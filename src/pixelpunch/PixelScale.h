#pragma once

#include "cinder/Cinder.h"
#include "cinder/Surface.h"

namespace pp 
{
	enum ScaleMethod {
		SM_NONE,
		
		SM_SCALE2x, 
		SM_SCALE3x,
		SM_SCALE4x,

		SM_EAGLE2x,

		SM_SCALE2x_HQ,
		SM_SCALE3x_HQ,
		SM_SCALE4x_HQ
	};
	typedef enum ScaleMethod ScaleMethod;

	cinder::Surface scale(cinder::Surface& source, ScaleMethod method);
}