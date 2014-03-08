#include "PixelPunch.h"
#include "PixelTransform.h"
#include "Kernel.h"
#include "cinder/Matrix.h"
#include <cassert>

using namespace cinder;
using namespace pp;


TransformMapping::TransformMapping(Vec2f* pts)
{
	bounds.set(pts[0].x, pts[0].y, 0, 0);
	
	for(int i = 0; i < 4; i++)
	{
		bounds.x1 = std::min(pts[i].x, bounds.x1);
		bounds.y1 = std::min(pts[i].y, bounds.y1);

		bounds.x2 = std::max(pts[i].x, bounds.x2);
		bounds.y2 = std::max(pts[i].y, bounds.y2);
	}

	Vec2f topLeft = bounds.getUpperLeft();
	for(int i = 0; i < 4; i++)
		localQuad[i] = pts[i] - topLeft;
}


TransformMapping::TransformMapping(const Rectf& rect)
{
	bounds = rect;
	Vec2f topLeft = bounds.getUpperLeft();
	//starting with TOPLEFT clockwise
	localQuad[0] = bounds.getUpperLeft() - topLeft;
	localQuad[1] = bounds.getUpperRight() - topLeft;
	localQuad[2] = bounds.getLowerRight() - topLeft;
	localQuad[3] = bounds.getLowerLeft() - topLeft;
}

Matrix33f _mapUnitSquareToQuad(ci::Vec2f* quad)
{
	Matrix33f result;
	result.m02 = quad[0].x;
	result.m12 = quad[0].y;
	result.m22 = 1;
	
	Vec2f p = quad[0] - quad[1] + quad[2] - quad[3];
	if(p.x == 0 && p.y == 0)
	{
		//Affine
		result.m00 = quad[1].x - quad[0].x;
		result.m01 = quad[2].x - quad[1].x;
		
		result.m10 = quad[1].y - quad[0].y;
		result.m11 = quad[2].y - quad[1].y;

		result.m20 = 0;
		result.m21 = 0;
	}
	else
	{
		//Projective
		Vec2f d1 = quad[1] - quad[2];
		Vec2f d2 = quad[3] - quad[2];
		float del = d1.cross(d2);
		assert(del != 0);

		result.m20 = p.cross(d2) / del;
		result.m21 = d1.cross(p) / del;
	
		result.m00 = quad[1].x - quad[0].x + result.m20 * quad[1].x;
		result.m01 = quad[3].x - quad[0].x + result.m21 * quad[3].x;
	
		result.m10 = quad[1].y - quad[0].y + result.m20 * quad[1].y;
		result.m11 = quad[3].y - quad[0].y + result.m21 * quad[3].y;
	}
	return result;

}

template<class Sampler>
void _drawProjective(Sampler& sampler, TransformMapping& srcMapping, Surface& dest, TransformMapping& destMapping)
{
	//calculate matrix mapping each pixel in target to a coordinate in source
	Matrix33f uvToTarget = _mapUnitSquareToQuad(destMapping.localQuad);
	Matrix33f targetToUV = uvToTarget.inverted();
	Matrix33f uvToSource = _mapUnitSquareToQuad(srcMapping.localQuad);
	Matrix33f targetToSource = uvToSource * targetToUV;

	//for each target pixel find one in source!
	float srcWidth = sampler.source.getWidth();
	float srcHeight = sampler.source.getHeight();
	Color8u blank = Color8u();
	for(int x = 0; x < dest.getWidth(); x++)
		for(int y = 0; y < dest.getHeight(); y++)
		{
			Vec3f vSrc = targetToSource.transformVec(Vec3f(x,y,1));
			vSrc /= vSrc.z;
			if(vSrc.x >= 0 && vSrc.y >= 0 && vSrc.x < srcWidth && vSrc.y < srcHeight)
				dest.setPixel(Vec2i(x,y), sampler(vSrc.x, vSrc.y) );
			else
				dest.setPixel(Vec2i(x,y), blank);
		}
}

Vec2f _transformInvBilinear(Vec2f p, Vec2f* q)
{	
	//non-inverse is easy: 
	//p = (1-u)*(1-v)*q[0] + (1-u)*v*q[3] + u*(1-v)*q[1] + u*v*q[2]
	//solve p.x and p.y to v and get
	//v = ( (1-u)*(x0-x) + u*(x1-x) ) / ( (1-u)*(x0-x3) + u*(x1-x2) )
	//v = ( (1-u)*(y0-y) + u*(y1-y) ) / ( (1-u)*(y0-y3) + u*(y1-y2) )

	//A*(1-u)^2 + B*2u(1-u) + C*u^2 = 0
	
	double A = (q[0]-p).cross(q[0]-q[3]);
	double B = ((q[0]-p).cross(q[1]-q[2]) + (q[1]-p).cross(q[0]-q[3]) ) / 2;
	double C = (q[1]-p).cross(q[1]-q[2]);

	//FIND U
	double u = 0;
	double div = ( A - 2*B + C );
	if(std::abs(div) < ci::EPSILON_VALUE)
		u = ((A-C) != 0) ? A / (A-C) : 0;//U
	else
	{
		u = ( (A-B) + sqrt(B*B - A*C) ) / div;
		if(u < 0 || u > 1)
			u = ( (A-B) - sqrt(B*B - A*C) ) / div;
	}

	//FIND V
	double v = 0;
	Vec2f vDiv = (1-u)*(q[0]-q[3]) + u*(q[1]-q[2]);
	if(abs(vDiv.x) > abs(vDiv.y))
		v = ( (1-u)*(q[0].x-p.x) + u*(q[1].x-p.x) ) / vDiv.x;
	else if(vDiv.y != 0)
		v = ( (1-u)*(q[0].y-p.y) + u*(q[1].y-p.y) ) / vDiv.y;
	
	return Vec2f((float)u,(float)v);
}

template<class Sampler>
void _drawBilinear(Sampler& sampler, TransformMapping& srcMapping, Surface& dest, TransformMapping& destMapping)
{
	Matrix33f uvToSource = _mapUnitSquareToQuad(srcMapping.localQuad);

	//for each target pixel find one in source!
	float srcWidth = sampler.source.getWidth();
	float srcHeight = sampler.source.getHeight();
	Color8u blank = Color8u();
	for(int x = 0; x < dest.getWidth(); x++)
		for(int y = 0; y < dest.getHeight(); y++)
		{
			float u = (x+0.5) / (float)dest.getWidth();
			float v = (y+0.5) / (float)dest.getHeight();
			Vec2f uv = _transformInvBilinear(Vec2f(x,y), destMapping.localQuad);
			if(u >= 0 && u <= 1 && v >= 0 && v <= 1)
			{
				Vec3f vSrc = uvToSource.transformVec(Vec3f(uv,1));
				vSrc /= vSrc.z;
				if(vSrc.x >= 0 && vSrc.y >= 0 && vSrc.x < srcWidth && vSrc.y < srcHeight)
					dest.setPixel(Vec2i(x,y), sampler(vSrc.x, vSrc.y) );
				else
					dest.setPixel(Vec2i(x,y), blank);
			};
		}	
}

template<class Sampler>
Surface pp::transform(Sampler& sampler, TransformMapping& targetMapping, TransformMethod method)
{
	if(method == TM_IDENTITY)
		return sampler.source;

	Surface result(targetMapping.bounds.getWidth(), targetMapping.bounds.getHeight(), sampler.source.hasAlpha());
	TransformMapping srcMapping(sampler.source.getBounds());
	switch(method)
	{
	case TM_PROJECTIVE:
		_drawProjective(sampler, srcMapping, result, targetMapping);
		break;
	case TM_BILINEAR:
		_drawBilinear(sampler, srcMapping, result, targetMapping);
		break;
	}	
	return result;
}

//****** SAMPLER ******

//NEAREST NEIGHBOUR
template Surface pp::transform<NearestNeighbourSampler>(NearestNeighbourSampler& source, TransformMapping& targetMapping, TransformMethod method);

NearestNeighbourSampler::NearestNeighbourSampler(Surface& src)
{
	source = src;
}

ColorA8u NearestNeighbourSampler::operator()(float x, float y)
{
	Vec2i srcPxl;
	srcPxl.x = (int)(x + 0.5);
	srcPxl.y = (int)(y + 0.5);
	return source.getPixel(srcPxl);
}

//BILINEAR

template Surface pp::transform<BilinearSampler>(BilinearSampler& source, TransformMapping& targetMapping, TransformMethod method);

BilinearSampler::BilinearSampler(cinder::Surface& src)
{
	source = src;
}

ColorA8u BilinearSampler::operator()(float x, float y)
{
	/*
		a b
		c d
	*/
	int x1 = floor(x);
	int y1 = floor(y);
	int x2 = ceil(x);
	int y2 = ceil(y);
	ColorAf a = source.getPixel(Vec2i(x1, y1));
	ColorAf b = source.getPixel(Vec2i(x2, y1));
	ColorAf c = source.getPixel(Vec2i(x1, y2));
	ColorAf d = source.getPixel(Vec2i(x2, y2));
	float subx = x - x1;
	float suby = y - y1;
	return a*( (1-subx)	* (1-suby) )
		 + b*( subx		* (1-suby) )
		 + c*( (1-subx)	* suby )
		 + d*( subx		* suby );
}

template Surface pp::transform<BicubicSampler>(BicubicSampler& source, TransformMapping& targetMapping, TransformMethod method);

double _cubicInterpolate (double p[4], double x) 
{
	return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

double _bicubicInterpolate (double p[4][4], double x, double y) 
{
	double arr[4];
	arr[0] = _cubicInterpolate(p[0], y);
	arr[1] = _cubicInterpolate(p[1], y);
	arr[2] = _cubicInterpolate(p[2], y);
	arr[3] = _cubicInterpolate(p[3], y);
	return constrain(_cubicInterpolate(arr, x), 0.0, 1.0);
}

BicubicSampler::BicubicSampler(cinder::Surface& src)
{
	source = src;
}

ci::ColorA8u BicubicSampler::operator()(float x, float y)
{
	/*
		4x4		
	*/
	int x1 = floor(x)-1;
	int y1 = floor(y)-1;
	double p[3][4][4];
	for(int ox = 0; ox < 4; ox++)
		for(int oy = 0; oy < 4; oy++)
		{
			ColorAf c = source.getPixel(Vec2i(x1+ox, y1+oy));
			p[0][ox][oy] = c.r;
			p[1][ox][oy] = c.g;
			p[2][ox][oy] = c.b;
		}
	
	float subx = x - floor(x);
	float suby = y - floor(y);
	ColorA8u result(0,0,0,1);
	result.r = 255*_bicubicInterpolate(p[0],subx,suby);
	result.g = 255*_bicubicInterpolate(p[1],subx,suby);
	result.b = 255*_bicubicInterpolate(p[2],subx,suby);
	return result;
}

template Surface pp::transform<BilinearDominanceSampler>(BilinearDominanceSampler& source, TransformMapping& targetMapping, TransformMethod method);

BilinearDominanceSampler::BilinearDominanceSampler(cinder::Surface& src, int sampleOrder)
{
	source = src;
	order = sampleOrder;
}

ColorA8u BilinearDominanceSampler::operator()(float x, float y)
{
	/*
		a b
		c d
	*/
	int i = 0;
	int k = 0;
	ColorA8u colors[4];
	float weights[4];
	int x1 = floor(x);
	int y1 = floor(y);
	int x2 = ceil(x);
	int y2 = ceil(y);
	float subx = x - x1;
	float suby = y - y1;
	//A
	colors[i] = source.getPixel(Vec2i(x1, y1));
	weights[i] =  (1-subx)	* (1-suby);
	i++;
	//B
	ColorA8u c = source.getPixel(Vec2i(x2, y1));
	for(k = 0; k < i; k++)
		if(colors[k].r == c.r && colors[k].g == c.g && colors[k].b == c.b)
		{
			weights[k] += subx * (1-suby);
			break;
		}
	if(k == i)
	{
		colors[i] = c;
		weights[i] = subx * (1-suby);
		i++;
	}
	//C
	c = source.getPixel(Vec2i(x1, y2));
	for(k = 0; k < i; k++)
		if(colors[k].r == c.r && colors[k].g == c.g && colors[k].b == c.b)
		{
			weights[k] += (1-subx) * suby;
			break;
		}
	if(k == i)
	{
		colors[i] = c;
		weights[i] = (1-subx) * suby;
		i++;
	}
	//D
	c = source.getPixel(Vec2i(x2, y2));
	for(k = 0; k < i; k++)
		if(colors[k].r == c.r && colors[k].g == c.g && colors[k].b == c.b)
		{
			weights[k] += (1-subx) * suby;
			break;
		}
	if(k == i)
	{
		colors[i] = c;
		weights[i] = subx * suby;
		i++;
	}
	/**
	int best = 0;
	for(k = 0; k < i; k++)
	{
		if(weights[k] >= weights[best])
			best = k;
	}
	return colors[best];
	**/
	if(i == 1)
		return colors[0];

	//make sure that 
	int max = 0;
	int a = -1;
	while(true)
	{
		//assume [a] is max
		int max = ++a;
		//find the real max
		for(k = max + 1; k < i; k++)
			if(weights[k] > weights[max])
				max = k; //new max
		
		if(a == order)
			return colors[max];
		//if we're not done and colors[a] is not max it might still be our result 
		//so we replace colors[max] (which we don't care for) with colors[a]
		if(max != a)
		{
			weights[max] = weights[a];
			colors[max] = colors[a];
		}
	}
	return colors[max];
}

template Surface pp::transform<BicubicBestFitSampler>(BicubicBestFitSampler& source, TransformMapping& targetMapping, TransformMethod method);

BicubicBestFitSampler::BicubicBestFitSampler(cinder::Surface& src, bool allowOuterPixels) : palette(NULL)
{
	source = src;
	mode = allowOuterPixels ? LOCAL_4x4 : LOCAL_2x2;
}

BicubicBestFitSampler::BicubicBestFitSampler(cinder::Surface& src, Palette& colors) : palette(&colors)
{
	source = src;
	mode = PALETTE;
}

ci::ColorA8u BicubicBestFitSampler::operator()(float x, float y)
{
	/*
		4x4		
	*/
	int x1 = floor(x)-1;
	int y1 = floor(y)-1;
	double p[3][4][4];
	for(int ox = 0; ox < 4; ox++)
		for(int oy = 0; oy < 4; oy++)
		{
			ColorAf c = source.getPixel(Vec2i(x1+ox, y1+oy));
			p[0][ox][oy] = c.r;
			p[1][ox][oy] = c.g;
			p[2][ox][oy] = c.b;
		}
	
	float subx = x - floor(x);
	float suby = y - floor(y);
	float r = _bicubicInterpolate(p[0],subx,suby);
	float g = _bicubicInterpolate(p[1],subx,suby);
	float b = _bicubicInterpolate(p[2],subx,suby);

	//return the best fitting of the 4 center pixels (least squares)
	ColorA8u result(0,0,0,1);
	float best = std::numeric_limits<float>::max();
	if(mode == PALETTE && palette)
	{
		ColorA8u pxl(255 * r, 255 * g, 255 * b);
		for(Palette::iterator it = palette->begin(); it != palette->end(); it++)
		{
			float error = it->distanceSquared(pxl);
			if(error < best)//found
			{
				result = *it;
				best = error;
			}
			if(error == 0)
				break;
		}
	}
	else
	{
		int from = (mode == LOCAL_4x4) ? 0 : 1;
		int to = (mode == LOCAL_4x4) ? 3 : 2;
		for(int i = from; i <= to; i++)
			for(int j = from; j <= to; j++)
			{
				float r_ij = p[0][i][j];
				float g_ij = p[1][i][j];
				float b_ij = p[2][i][j];
				float error = (r_ij-r)*(r_ij-r) + (g_ij-g)*(g_ij-g) + (b_ij-b)*(b_ij-b);
				if(error < best)
				{
					best = error;
					result.r = r_ij * 255;
					result.g = g_ij * 255;
					result.b = b_ij * 255;
				}
			}
	}
	return result;
}

//***
//***
//***

template Surface pp::transform<WeightSampler>(WeightSampler& source, TransformMapping& targetMapping, TransformMethod method);

WeightSampler::WeightSampler(cinder::Surface& src, int sampleOrder)
{
	source = src;
	order = sampleOrder;
}

ColorA8u WeightSampler::operator()(float x, float y)
{
	/*
		a b
		c d
	*/
	int i = 0;
	int k = 0;
	ColorA8u colors[4];
	float weights[4];
	int x1 = floor(x);
	int y1 = floor(y);
	int x2 = ceil(x);
	int y2 = ceil(y);
	float subx = x - x1;
	float suby = y - y1;
	//A
	colors[i] = source.getPixel(Vec2i(x1, y1));
	weights[i] =  (1-subx)	* (1-suby);
	i++;
	//B
	ColorA8u c = source.getPixel(Vec2i(x2, y1));
	for(k = 0; k < i; k++)
		if(colors[k].r == c.r && colors[k].g == c.g && colors[k].b == c.b)
		{
			weights[k] += subx * (1-suby);
			break;
		}
	if(k == i)
	{
		colors[i] = c;
		weights[i] = subx * (1-suby);
		i++;
	}
	//C
	c = source.getPixel(Vec2i(x1, y2));
	for(k = 0; k < i; k++)
		if(colors[k].r == c.r && colors[k].g == c.g && colors[k].b == c.b)
		{
			weights[k] += (1-subx) * suby;
			break;
		}
	if(k == i)
	{
		colors[i] = c;
		weights[i] = (1-subx) * suby;
		i++;
	}
	//D
	c = source.getPixel(Vec2i(x2, y2));
	for(k = 0; k < i; k++)
		if(colors[k].r == c.r && colors[k].g == c.g && colors[k].b == c.b)
		{
			weights[k] += (1-subx) * suby;
			break;
		}
	if(k == i)
	{
		colors[i] = c;
		weights[i] = subx * suby;
		i++;
	}

	if(i < (order+1))//order doesn't exists
		return ColorA8u(0,0,0);

	if(i == 1 && order == 0)//only one value
		return ColorA8u(255,0,0);


	//make sure that 
	int max = 0;
	int a = -1;
	while(true)
	{
		//assume [a] is max
		int max = ++a;
		//find the real max
		for(k = max + 1; k < i; k++)
			if(weights[k] > weights[max])
				max = k; //new max
		
		if(a == order)
			return ColorA8u(weights[max]*255,0,0);
		//if we're not done and colors[a] is not max it might still be our result 
		//so we replace colors[max] (which we don't care for) with colors[a]
		if(max != a)
			weights[max] = weights[a];
	}
	return ColorA8u(0,255,0);
}
