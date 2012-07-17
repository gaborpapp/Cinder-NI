#pragma once

#include "cinder/Vector.h"
#include "cinder/Quaternion.h"
#include "cinder/Matrix.h"

class Node
{
	public:
		Node();
		virtual ~Node() {}

		void setPosition( const ci::Vec3f& p );
		void setOrientation( const ci::Quatf& q );
		void setScale( float s );
		void setScale( const ci::Vec3f& s );

		void draw();
		virtual void customDraw() {}

		ci::Matrix44f getGlobalTransformMatrix() const;

	private:
		void createMatrix();

		ci::Vec3f mPosition;
		ci::Quatf mOrientation;
		ci::Vec3f mScale;

		ci::Matrix44f mLocalTransformMatrix;
};

