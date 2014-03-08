#include "Kernel.h"

using namespace pp;
using namespace cinder;

Kernel::Kernel(Surface& source, int width, int height, int centerX, int centerY)
:	mIter(source.getIter()),
	mWidth(width),
	mHeight(height),
	mOffsetX(-centerX),
	mOffsetY(-centerY)
{
	mHasAlpha = source.hasAlpha();
	Area range = source.getBounds();

	//range.x2 -= (mWidth + paddingX);
	//range.y2 -= (mHeight + paddingY);
	//prepare data storage
	pixels = (uint32_t**)malloc(mWidth * mHeight * sizeof(*pixels));    
	for(int i = 0; i < mWidth; ++i)    
	{        
		pixels[i] = (uint32_t*)malloc(mHeight * sizeof(*pixels[0]));    
	}
	mIter = source.getIter(range);
	mValid = mIter.line() & mIter.pixel();
}

Kernel::~Kernel()
{
	for(int i = 0; i < mWidth; ++i)    
		free(pixels[i]);
	free(pixels);
}

bool Kernel::copy(const Kernel& from)
{
	if(mWidth > from.mWidth || mHeight > from.mHeight)
		return false;

	for(int x = 0; x < mWidth; x++)
		for(int y = 0; y < mHeight; y++)
			pixels[x][y] = from.pixels[x][y];
	
	return true;
}

bool Kernel::step(int stepsH, int stepsV)
{
	//step right
	for(int i = 0; mIter.mX < mIter.mEndX && i < stepsH; i++)
	{
		++mIter.mX;
		mIter.mPtr += mIter.mInc;
	}
	//end of line? step down!
	if(mIter.mX == mIter.mEndX)
		for(int i = 0;  mIter.mY < mIter.mEndY && i < stepsV; i++)
		{
			++mIter.mY;
			mIter.mLinePtr += mIter.mRowInc;
			mIter.mPtr = reinterpret_cast<uint8_t*>( mIter.mLinePtr );
			mIter.mX = mIter.mStartX;
		}

	return (mIter.mY < mIter.mEndY);
}

bool Kernel::write(int steps)
{
	for(int x = 0, ox = mOffsetX; x < mWidth; x++, ox++)
		for(int y = 0, oy = mOffsetY; y < mHeight; y++, oy++)
		{
			uint32_t r = 0xFF & (pixels[x][y] >> 16);
			uint32_t g = 0xFF & (pixels[x][y] >> 8);
			uint32_t b = 0xFF &  pixels[x][y];
			mIter.rClamped(ox,oy) = r;
			mIter.gClamped(ox,oy) = g;
			mIter.bClamped(ox,oy) = b;
		}	

	return step(steps, steps);
}

bool Kernel::read(int steps)
{		
	if(!mValid)
		return false;

	for(int x = 0, ox = mOffsetX; x < mWidth; x++, ox++)
		for(int y = 0, oy = mOffsetY; y < mHeight; y++, oy++)
		{
			uint32_t r = mIter.rClamped(ox,oy);
			uint32_t g = mIter.gClamped(ox,oy);
			uint32_t b = mIter.bClamped(ox,oy);
			uint32_t c = ((r << 16) & 0x00FF0000) + ((g << 8) & 0x0000FF00) + (b & 0x000000FF);
			pixels[x][y] = c; //TODO: Consider Alpha << mIter.a(x,y);
		}

	return step(steps, steps);
}
