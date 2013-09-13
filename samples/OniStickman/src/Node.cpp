#include "cinder/gl/gl.h"

#include "Node.h"

using namespace ci;

Node::Node()
{
	setPosition( Vec3f( 0, 0, 0 ) );
	setOrientation( Quatf( 1, 0, 0, 0 ) );
	setScale( 1 );
}

void Node::setPosition( const Vec3f& p )
{
	mPosition = p;
	mLocalTransformMatrix.setTranslate( p );
}

void Node::setOrientation( const Quatf& q )
{
	mOrientation = q;
	createMatrix();
}

void Node::setScale( float s )
{
    setScale( Vec3f( s, s, s ) );
}

void Node::setScale( const Vec3f& s )
{
    mScale = s;
    createMatrix();
}

void Node::createMatrix()
{
    mLocalTransformMatrix = Matrix44f::createScale( mScale );
    mLocalTransformMatrix *= mOrientation.toMatrix44();
    mLocalTransformMatrix.setTranslate( mPosition );
}

void Node::draw()
{
    gl::pushMatrices();
    glMultMatrixf( &getGlobalTransformMatrix().m[ 0 ] );
	customDraw();
	gl::popMatrices();
}

Matrix44f Node::getGlobalTransformMatrix() const
{
	return mLocalTransformMatrix;
}

