#pragma once

#include "cinder/Vector.h"
#include "cinder/Rect.h"
#include "cinder/Matrix.h"
#include "cinder/app/MouseEvent.h"

class TransformUI
{
public:
	TransformUI(void);
	~TransformUI(void);
	void setView(ci::Vec2i windowSize, float scale);
	void setShape(ci::Rectf rect);
	
	//inpute handlers return true if shape changed
	bool mouseMove(ci::app::MouseEvent evt);
	bool mouseDown(ci::app::MouseEvent evt);
	bool mouseUp(ci::app::MouseEvent evt);
	bool mouseDrag(ci::app::MouseEvent evt);

	void draw();
	void center();
	ci::Rectf getBounds();
	ci::Matrix44f getShapeToView() { return mShapeToView; };
	ci::Matrix44f getViewToShape() { return mViewToShape; };
	
	static const int NONE = -1;
	static const int TOP_LEFT = 0;
	static const int TOP_RIGHT = 1;
	static const int BOTTOM_RIGHT = 2;
	static const int BOTTOM_LEFT = 3;

	static const int TOP = 0;
	static const int RIGHT = 1;
	static const int BOTTOM = 2;
	static const int LEFT = 3;

	ci::Vec2f shape[4];
	
private:
	void updateMatrix();
	ci::Vec2f rotatePoint(const ci::Vec2f& point, const ci::Vec2f& rotBase, float angleRadian);
	void drawRotationCone();
	float getAngle(const ci::Vec2f& v1, const ci::Vec2f& v2);
	ci::Matrix44f mShapeToView;
	ci::Matrix44f mViewToShape;
	ci::Vec2f mShapeOffset;
	ci::Vec2f mViewSize;
	float mViewScale;
	bool mLeftMouseDown;
	bool mRightMouseDown;
	//dragging
	int mDraggedCorner;
	int mDraggedEdge;
	ci::Vec2f mEdgeStartToMouse;
	ci::Vec2f mMouseToEdgeEnd;
	ci::Vec2f mMouseDragStart;
	//rotation
	bool mIsRotating;
	ci::Vec2f mRotationStartDir;
	ci::Vec2f mRotationCurrentDir;
	float mRotationAngle;
	ci::Vec2f mPreRotShape[4];
	//config
	float mHandleRadius;
	float mRotationHandleLength;
};

