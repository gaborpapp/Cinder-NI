#include "cinder/gl/gl.h"

#include "Node.h"

using namespace ci;

Node::Node()
{
	setPosition( vec3( 0, 0, 0 ) );
	setOrientation( quat( 1, 0, 0, 0 ) );
	setScale( 1 );
}

void Node::setPosition( const vec3 &p )
{
	mPosition = p;
	mLocalTransformMatrix[ 3 ] = vec4( p, 1.0f );
}

void Node::setOrientation( const quat &q )
{
	mOrientation = q;
	createMatrix();
}

void Node::setScale( float s )
{
	setScale( vec3( s, s, s ) );
}

void Node::setScale( const vec3 &s )
{
	mScale = s;
	createMatrix();
}

void Node::createMatrix()
{
	mLocalTransformMatrix = glm::scale( mScale );
	mLocalTransformMatrix *= glm::mat4_cast( mOrientation );
	mLocalTransformMatrix[ 3 ] = vec4( mPosition, 1.0f );
}

void Node::draw()
{
	gl::pushMatrices();
	gl::multModelMatrix( getGlobalTransformMatrix() );
	customDraw();
	gl::popMatrices();
}

mat4 Node::getGlobalTransformMatrix() const
{
	return mLocalTransformMatrix;
}
