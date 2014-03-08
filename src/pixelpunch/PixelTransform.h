#pragma once

#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include "cinder/Rect.h"

namespace pp 
{
	enum TransformMethod {
		TM_IDENTITY,
		TM_PROJECTIVE,
		TM_BILINEAR
	};
	typedef enum TransformMethod TransformMethod;

	struct TransformMapping
	{
		TransformMapping(cinder::Vec2f* pts);
		TransformMapping(const cinder::Rectf& rect);
		ci::Vec2f localQuad[4]; //starting with TOPLEFT clockwise, local to bounds
		ci::Rectf bounds;
	};

	enum SamplingMethod {
		SAMPLE_NEAREST,
		SAMPLE_BILINEAR,
		SAMPLE_BICUBIC,
		SAMPLE_FIRST_BILINEAR,
		SAMPLE_SECOND_BILINEAR,
		SAMPLE_BEST_FIT_NARROW,
		SAMPLE_BEST_FIT_WIDE,
		SAMPLE_BEST_FIT_ANY,
		SAMPLE_FIRST_WEIGHT,
		SAMPLE_SECOND_WEIGHT,
		SAMPLE_MINIMIZE_ERROR
	};
	typedef enum SamplingMethod SamplingMethod;

	struct NearestNeighbourSampler
	{
		NearestNeighbourSampler(cinder::Surface& src);
		ci::Surface source;
		ci::ColorA8u operator()(float x, float y);
	};

	struct BilinearSampler
	{
		BilinearSampler(cinder::Surface& src);
		ci::Surface source;
		ci::ColorA8u operator()(float x, float y);
	};

	struct BicubicSampler
	{
		BicubicSampler(cinder::Surface& src);
		ci::Surface source;
		ci::ColorA8u operator()(float x, float y);
	};
	
	struct BilinearDominanceSampler
	{
		BilinearDominanceSampler(cinder::Surface& src, int sampleOrder);
		ci::Surface source;
		int order; //0 = most dominant, 1 = 2nd most dominant...
		ci::ColorA8u operator()(float x, float y);
	};

	struct BicubicBestFitSampler
	{
		enum ColorSelectMode { LOCAL_2x2, LOCAL_4x4, PALETTE };
		
		BicubicBestFitSampler(cinder::Surface& src, bool allowOuterPixels);
		BicubicBestFitSampler(cinder::Surface& src, Palette& colors);
		ci::Surface source;
		ColorSelectMode mode;
		Palette* palette;
		ci::ColorA8u operator()(float x, float y);
	};

	struct WeightSampler
	{
		WeightSampler(cinder::Surface& src, int sampleOrder);
		ci::Surface source;
		int order; //0 = most dominant, 1 = 2nd most dominant...
		ci::ColorA8u operator()(float x, float y);
	};


	template<class Sampler>
	cinder::Surface transform(Sampler& source, TransformMapping& targetMapping, TransformMethod method);


}