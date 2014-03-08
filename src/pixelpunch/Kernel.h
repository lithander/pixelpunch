#pragma once

#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include "cinder/Area.h"

namespace pp 
{
	class Kernel
	{
	public:
		Kernel(cinder::Surface& source, int width, int height, int centerX = 0, int centerY = 0);
		~Kernel();
		bool step(int stepsH, int stepsV);
		bool read(int steps = 1);
		bool write(int steps = 1);
		bool copy(const Kernel& from);
		cinder::Surface::Iter& iter() { return mIter; }
		uint32_t** pixels;

	private:
		cinder::Surface::Iter mIter;
		bool mHasAlpha;
		bool mValid;
		int mWidth;
		int mHeight;
		int mOffsetX;
		int mOffsetY;
	};
}
