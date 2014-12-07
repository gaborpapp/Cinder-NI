#pragma once

#include "cinder/Vector.h"
#include "cinder/Quaternion.h"
#include "cinder/Matrix.h"

class Node
{
	public:
		Node();
		virtual ~Node() {}

		void setPosition( const ci::vec3 &p );
		void setOrientation( const ci::quat &q );
		void setScale( float s );
		void setScale( const ci::vec3 &s );

		void draw();
		virtual void customDraw() {}

		ci::mat4 getGlobalTransformMatrix() const;

	private:
		void createMatrix();

		ci::vec3 mPosition;
		ci::quat mOrientation;
		ci::vec3 mScale;

		ci::mat4 mLocalTransformMatrix;
};

