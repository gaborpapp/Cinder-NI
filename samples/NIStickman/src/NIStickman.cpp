/*
 Copyright (C) 2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Light.h"
#include "cinder/Quaternion.h"
#include "cinder/Camera.h"

#include "CiNI.h"

#include "Node.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class Bone : public Node
{
    public:
        Quatf mBindPoseOrientation;

        void customDraw()
        {
			gl::color( Color::white() );
			gl::drawCube( Vec3f( 0, -20, 0 ), Vec3f( 5, 30, 5 ) );
        }
};

class NIStickman : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void update();
		void draw();

	private:
		ni::OpenNI mNI;
		ni::UserTracker mNIUserTracker;
		gl::Texture mColorTexture;

		static const int sNumBones = 11;
        Bone mBones[ sNumBones ];

		void transformNode( unsigned userId, int nodeNum, XnSkeletonJoint skelJoint );

		CameraPersp mCam;
};

void NIStickman::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void NIStickman::setup()
{
	// openni
	try
	{
		//mNI = ni::OpenNI( ni::OpenNI::Device() );
		string oniFile = "form24.oni";
#ifdef CINDER_MAC
		oniFile = "../" + oniFile;
#endif
		mNI = ni::OpenNI( getAppPath() / oniFile );
	}
	catch ( ... )
	{
		console() << "Could not open Kinect" << endl;
		quit();
	}

	mNI.setDepthAligned();
	mNI.start();
	mNIUserTracker = mNI.getUserTracker();
	mNIUserTracker.setSmoothing( .7 );

	// stickman nodes
	for ( int i = 0; i < sNumBones; i++ )
	{
		mBones[ i ].setPosition( Vec3f( -1000, -1000, -1000 ) );
		mBones[ i ].setScale( 10 );

		// setup T pose
		// left arm
		if ( i == 2 || i == 4 )
		{
			mBones[i].mBindPoseOrientation.set( Vec3f( 0, 0, 1 ), toRadians( -90.f ) );
		}
		else // right arm
		if ( i == 3 || i == 5 )
		{
			mBones[ i ].mBindPoseOrientation.set( Vec3f( 0, 0, 1 ), toRadians( 90.f ) );
		}
		else // neck
		if ( i == 10 )
		{
			mBones[ i ].mBindPoseOrientation.set( Vec3f( 0, 0, 1 ), toRadians( 180.f ) );
		}
		else
		{
			mBones[ i ].mBindPoseOrientation.set( 1, 0, 0, 0 );
		}
	}

	mCam.lookAt( Vec3f( 0, 0, 0 ), Vec3f( 0, 0, 1500 ) );
	mCam.setPerspective( 60, getWindowAspectRatio(), 0.1f, 10000.0f );

	gl::enableDepthRead();
	gl::enableDepthWrite();
}

void NIStickman::update()
{
	if ( mNI.checkNewVideoFrame() )
		mColorTexture = mNI.getVideoImage();

	vector< unsigned > users = mNIUserTracker.getUsers();
	if ( !users.empty() )
	{
		unsigned userId = users[ 0 ];

		transformNode( userId, 0, XN_SKEL_TORSO );
		transformNode( userId, 1, XN_SKEL_WAIST );
		transformNode( userId, 2, XN_SKEL_LEFT_SHOULDER );
		transformNode( userId, 3, XN_SKEL_RIGHT_SHOULDER );
		transformNode( userId, 4, XN_SKEL_LEFT_ELBOW );
		transformNode( userId, 5, XN_SKEL_RIGHT_ELBOW );
		transformNode( userId, 6, XN_SKEL_LEFT_HIP );
		transformNode( userId, 7, XN_SKEL_RIGHT_HIP );
		transformNode( userId, 8, XN_SKEL_LEFT_KNEE );
		transformNode( userId, 9, XN_SKEL_RIGHT_KNEE );
		transformNode( userId, 10, XN_SKEL_NECK );
	}
}

void NIStickman::transformNode( unsigned userId, int nodeNum, XnSkeletonJoint skelJoint )
{
	float oriConf;
	Matrix44f ori = mNIUserTracker.getJointOrientation( userId, skelJoint, &oriConf );
	float posConf;
	Vec3f pos = mNIUserTracker.getJoint3d( userId, skelJoint, &posConf );

	if ( oriConf > 0 && posConf > 0 )
	{
		Quatf q( ori );
		mBones[ nodeNum ].setPosition( pos );

		// apply skeleton pose relatively to the bone bind pose
		mBones[ nodeNum ].setOrientation( mBones[ nodeNum ].mBindPoseOrientation * q );
	}
}

void NIStickman::draw()
{
	gl::clear( Color::black() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	gl::color( Color::white() );
	if ( mColorTexture )
		gl::draw( mColorTexture, Rectf( 0, 0, 160, 120 ) );

	gl::enable( GL_LIGHTING );
	gl::setMatrices( mCam );

	gl::Light light( gl::Light::DIRECTIONAL, 0 );
	light.setDirection( Vec3f( .5, 0, 1 ) );
    light.enable();
	light.update( mCam );

	// mirror
	gl::scale( Vec3f( -1, 1, 1 ) );

	for ( int i = 0; i < sNumBones; i++ )
	{
		mBones[ i ].draw();
	}
    light.disable();
	gl::disable( GL_LIGHTING );
}

CINDER_APP_BASIC( NIStickman, RendererGl() )

