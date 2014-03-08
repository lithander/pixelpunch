#include "PixelPunch.h"
#include "Kernel.h"
#include "PixelScale.h"
#include <cassert>

using namespace cinder;
using namespace pp;

void _repeat(Surface& source, Surface& dest, int scaleFactor)
{
	Surface::ConstIter srcIt = source.getIter();
	Surface::Iter destIt = dest.getIter();
	while(srcIt.line())
	{
		//back to the start of source line
		Surface::ConstIter copy = srcIt;
		for(int i = 0; i < scaleFactor; i++)
		{
			destIt.line();
			while(srcIt.pixel()) 
			{
				for(int j = 0; j < scaleFactor; j++)
				{
					destIt.pixel();
					destIt.r() = srcIt.r(); 
					destIt.g() = srcIt.g();
					destIt.b() = srcIt.b();
				}
			}
			//reset srcIt
			srcIt = copy;
		}
	}
}

void _scale2x(Surface& source, Surface& dest)
{
	Kernel kSrc(source, 3, 3, 1, 1);
	Kernel kDst(dest, 2, 2);
	uint32_t** src = kSrc.pixels;
	uint32_t** dst = kDst.pixels;
	do
	{
		/*
		A B C
		D E F -> E0 E1 -> 00 10
		G H I    E2 E3    01 11

		if (B != H && D != F)
			E0 = D == B ? D : E;
			E1 = B == F ? F : E;
			E2 = D == H ? D : E;
			E3 = H == F ? F : E;
		*/
		kSrc.read();
		bool prereq = (src[1][0] != src[1][2]) && (src[0][1] != src[2][1]);
		dst[0][0] = prereq && (src[0][1] == src[1][0]) ? src[0][1] : src[1][1];
		dst[1][0] = prereq && (src[1][0] == src[2][1]) ? src[2][1] : src[1][1];
		dst[0][1] = prereq && (src[0][1] == src[1][2]) ? src[0][1] : src[1][1];
		dst[1][1] = prereq && (src[1][2] == src[2][1]) ? src[2][1] : src[1][1];
			
	}
	while(kDst.write(2));
}

void _scale3x(Surface& source, Surface& dest)
{
	Kernel kSrc(source, 3, 3, 1, 1);
	Kernel kDst(dest, 3, 3, 0, 0);
	uint32_t** src = kSrc.pixels;
	uint32_t** dst = kDst.pixels;
	do
	{
	/*
		A B C    E0 E1 E2     00 10 20
		D E F -> E3 E4 E5 ->  01 11 21
		G H I    E6 E7 E8     02 12 22

		if (B != H && D != F) {
			E0 = D == B										? D : E;
			E1 = (D == B && E != C) || (B == F && E != A)	? B : E;
			E2 = B == F										? F : E;
				
			E3 = (D == B && E != G) || (D == H && E != A)	? D : E;
			E4 = E;
			E5 = (B == F && E != I) || (H == F && E != C)	? F : E;
				
			E6 = D == H										? D : E;
			E7 = (D == H && E != I) || (H == F && E != G)	? H : E;
			E8 = H == F										? F : E;
	*/
		kSrc.read();
		bool prereq = (src[1][0] != src[1][2]) && (src[0][1] != src[2][1]);
		bool D_is_B = (src[0][1] == src[1][0]);
		bool B_is_F = (src[1][0] == src[2][1]);
		bool D_is_H = (src[0][1] == src[1][2]);
		bool H_is_F = (src[1][2] == src[2][1]);
		bool E_not_C = (src[1][1] != src[2][0]);
		bool E_not_G = (src[1][1] != src[0][2]);
		bool E_not_I = (src[1][1] != src[2][2]);
		bool E_not_A = (src[1][1] != src[0][0]);
			
		dst[0][0] = prereq && D_is_B										? src[0][1] : src[1][1];
		dst[1][0] = prereq && ((D_is_B && E_not_C) || (B_is_F && E_not_A))	? src[1][0] : src[1][1];
		dst[2][0] = prereq && B_is_F										? src[2][1] : src[1][1];
			
		dst[0][1] = prereq && ((D_is_B && E_not_G) || (D_is_H && E_not_A))	? src[0][1] : src[1][1];
		dst[1][1] = src[1][1];
		dst[2][1] = prereq && ((B_is_F && E_not_I) || (H_is_F && E_not_C))	? src[2][1] : src[1][1];
			
		dst[0][2] = prereq && D_is_H										? src[0][1] : src[1][1];
		dst[1][2] = prereq && ((D_is_H && E_not_I) || (H_is_F && E_not_G))	? src[1][2] : src[1][1];
		dst[2][2] = prereq && H_is_F										? src[2][1] : src[1][1];
	}
	while(kDst.write(3));
}

void _eagle2x(Surface& source, Surface& dest)
{
	Kernel kSrc(source, 3, 3, 1, 1);
	Kernel kDst(dest, 2, 2);
	uint32_t** src = kSrc.pixels;
	uint32_t** dst = kDst.pixels;
	do
	{
		/*
		first:        |Then 
		. . . --\ CC  |00 10 20		S T U  --\ 1 2
		. C . --/ CC  |01 11 21		V C W  --/ 3 4
		. . .         |02 12 22		X Y Z
					  | IF V==S==T => 1=S
					  | IF T==U==W => 2=U
					  | IF V==X==Y => 3=X
					  | IF W==Z==Y => 4=Z
		*/
		kSrc.read();
		dst[0][0] = (src[0][1] == src[0][0] && src[1][0] == src[0][0]) ? src[0][0] : src[1][1];
		dst[1][0] = (src[1][0] == src[2][0] && src[2][1] == src[2][0]) ? src[2][0] : src[1][1];
		dst[0][1] = (src[0][1] == src[0][2] && src[1][2] == src[0][2]) ? src[0][2] : src[1][1];
		dst[1][1] = (src[2][1] == src[2][2] && src[1][2] == src[2][2]) ? src[2][2] : src[1][1];
			
	}
	while(kDst.write(2));
}


void _fillFissure(Surface& surf)
{
	/* 
	The artefact we want to remove consists of a cluster of 3 pixels sourrounded by pixels of the same other color.
	Fill the artefact witht he sourrounding color.

		B a B	B a B	B B .	B B . 
		a a B	B a a	B a a	a a B
		B B .	. B B	B a B	B a B
	*/

	Kernel k(surf, 3, 3, 1, 1);
	uint32_t** p = k.pixels;
	do
	{
		k.read(0);
		uint32_t cA = p[1][1];
		for(int i = -1; i < 2; i += 2)
			for(int j = -1; j < 2; j += 2)
			{
				uint32_t cB = p[1+j][1+i];
				if(cA == cB)
					continue;
				//crease exists?
				if((p[1+j][1] != cA) || (p[1][1+i] != cA))
					continue;
				//sourrounded? (edge)
				if((p[1-j][1] != cB) || (p[1][1-i] != cB))
					continue;
				//sourrounded? (corners)
				if((p[1-j][1+i] != cB) || (p[1+j][1-i] != cB))
					continue;

				p[1][1] = p[1+j][1] = p[1][1+i] = cB;
			}
	}
	while(k.write(1));
}

void _fillSingle(Surface& surf)
{
	/* 
	The artefact we want to remove consists of a single pixel flanked by pixels of the same other color.
	Fill the artefact witht he sourrounding color.

		. x .
		x A x
		. x .
	*/
	Kernel k(surf, 3, 3, 1, 1);
	uint32_t** p = k.pixels;
	do
	{
		k.read(0);
		uint32_t cA = p[1][1];
		uint32_t ref = p[0][1];
		if(cA != ref && ref == p[1][0] && ref == p[2][1] && ref == p[1][2])
			p[1][1] = ref;
	}
	while(k.write(1));
}

void _buffDouble(Surface& surf)
{
	/* 
	We want to buff two individual pixels of the same color touching corners.
		. . . x		x . . .
		. x	A .		. A x .
		. A	x .		. x A .
		x . . .		. . . x
	*/
	Kernel k(surf, 4, 4, 1, 1);
	uint32_t** p = k.pixels;
	do
	{
		k.read(0);
		uint32_t ref = p[2][1];
		if(ref == p[1][2] && ref != p[0][3] && ref != p[3][0] && ref != p[1][1] && ref != p[2][2])
			p[1][1] = p[2][2] = ref;

		ref = p[1][1];
		if(ref == p[2][2] && ref != p[0][0] && ref != p[3][3] && ref != p[2][1] && ref != p[1][2])
			p[2][1] = p[1][2] = ref;
	}
	while(k.write(1));
}

void _buffTripleStrict(Surface& surf)
{
	/* 
	We want to connect individual pixels to larger clusters
	
		A x .	. x A 
		x A x	x A x 
		. x A	A x .
	*/

	Kernel k(surf, 3, 3, 1, 1);
	uint32_t** p = k.pixels;
	do
	{
		k.read(0);
		uint32_t ref = p[0][0];
		if( ref == p[1][1] && ref == p[2][2] && //line exists
			ref != p[0][1] && ref != p[1][2] && ref != p[1][0] && ref != p[2][1]) //neighbours differ
				p[0][1] = p[1][2] = p[1][0] = p[2][1] = ref;

		ref = p[2][0];
		if( ref == p[1][1] && ref == p[0][2] && //line exists
			ref != p[0][1] && ref != p[1][2] && ref != p[1][0] && ref != p[2][1]) //neighbours differ
				p[0][1] = p[1][2] = p[1][0] = p[2][1] = ref;
	}
	while(k.write(1));
}


void _buffTripleLoose(Surface& surf)
{
	/* 
	We want to connect individual pixels to larger clusters. X and Y will be judged
	sperately.
	
		A x .	. y A 
		x A y	x A y 
		. y A	A x .
	*/

	Kernel k(surf, 3, 3, 1, 1);
	uint32_t** p = k.pixels;
	do
	{
		k.read(0);
		uint32_t ref = p[0][0];
		if( ref == p[1][1] && ref == p[2][2]) //line exists
		{
			if(ref != p[0][1] && ref != p[1][0]) 
				p[0][1] = p[1][0] = ref;
			if(ref != p[1][2] && ref != p[2][1]) //neighbours differ
				p[1][2] = p[2][1] = ref;
		}
		ref = p[2][0];
		if( ref == p[1][1] && ref == p[0][2]) //line exists
		{
			if(ref != p[0][1] && ref != p[1][2] ) //neighbours differ
				p[0][1] = p[1][2] = ref;
			if(ref != p[1][0] && ref != p[2][1]) //neighbours differ
				p[1][0] = p[2][1] = ref;
		}
	}
	while(k.write(1));
}

void genDest(Surface& source, int scaleFactor, Surface& result)
{
	int w = scaleFactor * source.getWidth();
	int h = scaleFactor * source.getHeight();
	bool alpha = source.hasAlpha();
	result = Surface(w, h, alpha);
}

Surface pp::scale(Surface& source, ScaleMethod method)
{
	Surface result;
	Surface temp;
	//migrate data
	switch(method)
	{
	case SM_NONE:
		genDest(source, 1, result);
		_repeat(source, result, 1);
		break;
	case SM_SCALE2x:
		genDest(source, 2, result);
		_scale2x(source, result);
		break;
	case SM_SCALE3x:
		genDest(source, 3, result);
		_scale3x(source, result);
		break;
	case SM_SCALE4x:
		genDest(source, 2, temp);
		_scale2x(source, temp);
		genDest(temp, 2, result);
		_scale2x(temp, result);
		break;
	case SM_EAGLE2x:
		genDest(source, 2, result);
		_eagle2x(source, result);
		break;
	case SM_SCALE2x_HQ:
		genDest(source, 2, result);
		_scale2x(source, result);
		_fillSingle(result);
		_buffDouble(result);
		break;
	case SM_SCALE3x_HQ:
		genDest(source, 3, result);
		_scale3x(source, result);
		_fillFissure(result);
		_buffTripleStrict(result);
		break;
	case SM_SCALE4x_HQ:
		genDest(source, 2, temp);
		_scale2x(source, temp);
		_fillSingle(temp);
		_buffDouble(temp);
		genDest(temp, 2, result);
		_eagle2x(temp, result);
	break;

	}
	return result;
}
