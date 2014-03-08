#include "PixelPunch.h"
#include "Kernel.h"
#include <cassert>

using namespace cinder;

void pp::genDest(Surface& source, int scaleFactor, Surface& result)
{
	int w = scaleFactor * source.getWidth();
	int h = scaleFactor * source.getHeight();
	bool alpha = source.hasAlpha();
	result = Surface(w, h, alpha);
}

void pp::getColors(cinder::Surface& source, Palette& result)
{
	result.clear();
	Vec2i v(0,0);
	int width = source.getWidth();
	int height = source.getHeight();
	for(v.x = 0; v.x < width; v.x++)
		for(v.y = 0; v.y < height; v.y++)
		{
			Color8u pxl = source.getPixel(v);
			bool found = false;
			for(Palette::iterator it = result.begin(); it != result.end(); it++)
				if(it->distanceSquared(pxl) == 0)//found
					found = true;
			if(!found)
				result.push_back(pxl);
		}
}

Surface pp::compare(Surface& imageA, Surface& imageB)
{
	//for each target pixel find one in source!
	float width = std::min(imageA.getWidth(),imageB.getWidth());
	float height = std::min(imageB.getHeight(),imageB.getHeight());
	
	float kernel[3][3] = {{0.0625,0.125,0.0625},{0.125,0.25,0.125},{0.0625,0.125,0.0625}};
	Surface result(width, height, false);
	Vec2i v(0,0);
	for(v.x = 0; v.x < width; v.x++)
		for(v.y = 0; v.y < height; v.y++)
		{
			float c[3] = {0.5,0.5,0.5};
			for(int i = 0; i < 3; i++)
				for(int j = 0; j < 3; j++)
				{
					Vec2i offset(i-1, j-1);
					Color8u a = imageA.getPixel(v+offset);
					Color8u b = imageB.getPixel(v+offset);
					for(int k = 0; k < 3; k++)
						c[k] += kernel[i][j] * (a[k]-b[k])/255.0f;
				}
				result.setPixel(v,Color8u(c[0]*255,c[1]*255,c[2]*255));
		}
	/*
	Surface result(width, height, false);
	Vec2i v(0,0);
	for(v.x = 0; v.x < width; v.x++)
		for(v.y = 0; v.y < height; v.y++)
		{
			Color8u a = imageA.getPixel(v);
			Color8u b = imageB.getPixel(v);
			float c[3] = {0.5,0.5,0.5};
			for(int k = 0; k < 3; k++)
				c[k] += (a[k]-b[k])/255.0f;
			result.setPixel(v,Color8u(c[0]*127,c[1]*127,c[2]*127));
		}
	*/
	return result;
}

Surface pp::choose(cinder::Surface& imageA, cinder::Surface& imageB, cinder::Surface& errorA, cinder::Surface& secondWeight, float threshold)
{
	//for each target pixel find one in source!
	float width = std::min(imageA.getWidth(),imageB.getWidth());
	float height = std::min(imageB.getHeight(),imageB.getHeight());
	
	Surface result(width, height, false);
	Vec2i v(0,0);
	for(v.y = 0; v.y < height; v.y++)
		for(v.x = 0; v.x < width; v.x++)
		{
			//is errorA a local maximum?
			bool swap = true;
			Color8u eA = errorA.getPixel(v);
			float errA = (eA.r-127)*(eA.r-127) + (eA.g-127)*(eA.g-127) + (eA.b-127)*(eA.b-127);
			float alternative = secondWeight.getPixel(v).r;
			if(std::sqrt(errA)*alternative <= threshold*(3*127*127))
				swap = false;
			else
				for(int i = 0; i < 3 && swap; i++)
					for(int j = 0; j < 3 && swap; j++)
						if(i != 1 || j != 1)
						{
							Vec2i offset(i-1, j-1);
							Color8u eO = errorA.getPixel(v+offset);
							float errOther = (eO.r-127)*(eO.r-127) + (eO.g-127)*(eO.g-127) + (eO.b-127)*(eO.b-127);
							if(errOther >= errA)
								swap = false;
						}

			if(swap)
				result.setPixel(v,imageB.getPixel(v));
			else
				result.setPixel(v,imageA.getPixel(v));
		}

	return result;
}

/*
if(swap)
{
	//is b at this spot an improvement?	
	float ref[3] = {0.0, 0.0, 0.0};
	float a[3] = {0.0, 0.0, 0.0};
	float b[3] = {0.0, 0.0, 0.0};
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
		{
			Vec2i offset(i-1, j-1);
			Color8u cRef = reference.getPixel(v+offset);
			Color8u cA = imageA.getPixel(v+offset);
			Color8u cB = imageB.getPixel(v+offset);
			for(int k = 0; k < 3; k++)
			{
				ref[k] += kernel[i][j] * cRef[k];
				a[k] += kernel[i][j] * cA[k];
				if(i == 1 && k == 1)
					b[k] += kernel[i][j] * cB[k];
				else
					b[k] += kernel[i][j] * cA[k];
			}
		}

	float diffASquare = 0;
	float diffBSquare = 0;
	for(int k = 0; k < 3; k++)
	{
		diffASquare += (ref[k] - a[k]) * (ref[k] - a[k]);
		diffBSquare += (ref[k] - b[k]) * (ref[k] - b[k]);					
	}
	if(diffBSquare < diffASquare)
		result.setPixel(v,imageB.getPixel(v));
	else
		result.setPixel(v,imageA.getPixel(v));
}
else
	result.setPixel(v,imageA.getPixel(v));
	*/