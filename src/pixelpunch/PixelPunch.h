#pragma once

#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include <list>

namespace pp 
{
	const float EPSILON = 0.01f;

	typedef std::list<cinder::Color8u> Palette;

	void genDest(cinder::Surface& source, int scaleFactor, cinder::Surface& result);
	void getColors(cinder::Surface& source, Palette& result);
	cinder::Surface compare(cinder::Surface& imageA, cinder::Surface& imageB);
	cinder::Surface choose(cinder::Surface& imageA, cinder::Surface& imageB, cinder::Surface& errorA, cinder::Surface& secondWeight, float threshold);
}