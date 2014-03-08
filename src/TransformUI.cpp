#include "TransformUI.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace	cinder::app;

TransformUI::TransformUI(void) :	
	mHandleRadius(4.0f),
	mRotationHandleLength(40.0f),
	mViewScale(1.0f),
	mShapeOffset(0,0),
	mDraggedEdge(NONE),
	mDraggedCorner(NONE),
	mLeftMouseDown(false),
	mRightMouseDown(false),
	mIsRotating(false)
{
}

TransformUI::~TransformUI(void)
{

}

void TransformUI::center()
{
	mShapeOffset.set(0,0);
	for(int i = 0; i < 4; i++)
		mShapeOffset += shape[i];
	mShapeOffset /= 4;
	updateMatrix();
}

void TransformUI::updateMatrix()
{
	mShapeToView.setToIdentity();
	mShapeToView.translate(Vec3f(0.5 * mViewSize));
	mShapeToView.scale(Vec3f(mViewScale, mViewScale, 1.0f));
	mShapeToView.translate(Vec3f(-mShapeOffset));

	mViewToShape = mShapeToView.affineInverted();
}

void TransformUI::setView(ci::Vec2i windowSize, float scale)
{
	mViewSize = windowSize;
	mViewScale = scale;
	updateMatrix();
}

void TransformUI::setShape(ci::Rectf rect)
{
	shape[TOP_LEFT] = rect.getUpperLeft();
	shape[TOP_RIGHT] = rect.getUpperRight();
	shape[BOTTOM_LEFT] = rect.getLowerLeft();
	shape[BOTTOM_RIGHT] = rect.getLowerRight();
}

ci::Rectf TransformUI::getBounds()
{
	Rectf result(shape[0], Vec2f());
	for(int i = 0; i < 4; i++)
	{
		result.x1 = std::min(shape[i].x, result.x1);
		result.y1 = std::min(shape[i].y, result.y1);

		result.x2 = std::max(shape[i].x, result.x2);
		result.y2 = std::max(shape[i].y, result.y2);
	}
	return result;
}	

bool TransformUI::mouseMove(MouseEvent evt)
{
	Vec2f pos = evt.getPos();
	Vec2f posShape = mViewToShape.transformPoint(Vec3f(pos)).xy();
	mDraggedCorner = NONE;
	mDraggedEdge = NONE;
	mLeftMouseDown = false;
	mRightMouseDown = false;

	//A handle to drag?
	for(int i = 0; i < 4; i++)
		if(shape[i].distance(posShape) < mHandleRadius)
		{
			mDraggedCorner = i;
			return true;
		}
	//An edge to drag?
	for(int i = 0; i < 4; i++)
	{
		Vec2f edgeDir = (shape[(i+1)%4] - shape[i]).normalized();
		Vec2f cornerToMouse = posShape - shape[i];
		Vec2f edgeToMouse = cornerToMouse - edgeDir * cornerToMouse.dot(edgeDir); 
		if(edgeToMouse.length() < mHandleRadius)
		{
			Vec2f posSnapped = posShape - edgeToMouse;
			mDraggedEdge = i;
			mEdgeStartToMouse = posSnapped - shape[i];
			mMouseToEdgeEnd = shape[(i+1)%4] - posSnapped;
			return true;
		}
	}
	return false;
}

bool TransformUI::mouseDown(MouseEvent evt)
{
	mMouseDragStart = evt.getPos();
	mRightMouseDown = evt.isRightDown();
	mLeftMouseDown = evt.isLeftDown();
	return false;
}

bool TransformUI::mouseUp(MouseEvent evt)
{
	mIsRotating = false;
	mLeftMouseDown = evt.isLeftDown();
	mRightMouseDown = evt.isRightDown();
	if(!mRightMouseDown)
		mIsRotating = false;
	return false;
}

bool TransformUI::mouseDrag(MouseEvent evt)
{
	Vec2f pos = evt.getPos();
	Vec2f posShape = mViewToShape.transformPoint(Vec3f(pos)).xy();
	if(mLeftMouseDown)
	{
		//DRAG EDGE
		if(mDraggedEdge >= 0)
		{
			shape[mDraggedEdge] = posShape - mEdgeStartToMouse;
			shape[(mDraggedEdge+1)%4] = posShape + mMouseToEdgeEnd;
			return true;
		}
		//DRAG CORNER
		if(mDraggedCorner >= 0)
		{
			shape[mDraggedCorner] = posShape;
			return true;
		}
	}
	//ROTATE SHAPE
	if(mRightMouseDown)
	{
		Vec2f pos = evt.getPos();
		if(!mIsRotating)
		{
			mRotationStartDir = pos - mMouseDragStart;
			if(mRotationStartDir.length() > mRotationHandleLength)
			{
				mRotationAngle = 0;
				mIsRotating = true;
				for(int i = 0; i < 4; i++)
					mPreRotShape[i] = shape[i];
			}
		}
		if(mIsRotating)
		{
			mRotationCurrentDir =  pos - mMouseDragStart;
			mRotationAngle = getAngle(mRotationStartDir, mRotationCurrentDir);
			if(mRotationAngle > 0.1)
				int foo = 0;
			Vec3f rotBase = mViewToShape * Vec3f(mMouseDragStart,1);
			for(int i = 0; i < 4; i++)
				shape[i] = rotatePoint(mPreRotShape[i], rotBase.xy(), mRotationAngle);
			return true;
		}
	}
	return false;
}

float TransformUI::getAngle(const Vec2f& v1, const Vec2f& v2)
{
	float angle = acosf(v1.normalized().dot(v2.normalized()));
	if(v2.cross(v1) > 0)
		angle = 2 * M_PI - angle;
	return angle;
}

Vec2f TransformUI::rotatePoint(const Vec2f& point, const Vec2f& rotBase, float angleRadian)
{
	Vec2f v = point - rotBase;
	v.rotate(angleRadian);
	return rotBase + v;
}

void TransformUI::draw()
{
	glLineWidth( 2.0f );
	
	gl::pushMatrices();
	gl::multModelView(mShapeToView.m);
	for(int i = 0; i < 4; i++)
	{
		glColor3f( 0.4f, 0.4f, 0.4f );
		//draw rect
		if(mDraggedEdge == i)
			glColor3f( 0.4f, 0.8f, 0.4f );
		gl::drawLine(shape[i], shape[(i+1)%4]);		
		//draw handles
		glColor3f( 0.4f, 0.4f, 0.4f );
		if(mDraggedCorner == i)
			glColor3f( 0.4f, 0.8f, 0.4f );
		gl::drawSolidCircle(shape[i], mHandleRadius/mViewScale, 30);
	}
	gl::popMatrices();
	drawRotationCone();
}

void TransformUI::drawRotationCone()
{
	//draw rotation helper
	if(mIsRotating)
	{
		Path2d path;
		path.moveTo(mMouseDragStart);
		float startAngle = getAngle(Vec2f(1,0), mRotationStartDir);
		if(mRotationAngle < 0 || mRotationAngle > M_PI)
			path.arc(mMouseDragStart, mRotationCurrentDir.length(), startAngle+mRotationAngle, startAngle);
		else
			path.arc(mMouseDragStart, mRotationCurrentDir.length(), startAngle, startAngle+mRotationAngle);
		path.close();
		
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE );
		glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		gl::drawSolid(path);
		glColor3f( 0.4f, 0.8f, 0.4f );
		gl::draw(path);
		glDisable( GL_BLEND );
	}
}

