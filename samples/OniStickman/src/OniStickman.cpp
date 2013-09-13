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

#include "cinder/Camera.h"
#include "cinder/Cinder.h"
#include "cinder/Quaternion.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Texture.h"

#include "CinderOni.h"

#include "Node.h"

using namespace ci;
using namespace ci::app;
using namespace std;

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

class OniStickmanApp : public AppBasic,
	public nite::UserTracker::NewFrameListener
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void update();
		void draw();

	private:
		mndl::oni::OniCaptureRef mOniCaptureRef;
		gl::Texture mDepthTexture;

		std::shared_ptr< nite::UserTracker > mUserTrackerRef;
		void onNewFrame( nite::UserTracker &userTracker );
		recursive_mutex mMutex;

		static const int sNumBones = 10;
		Bone mBones[ sNumBones ];

		void transformNode( const nite::Skeleton &skeleton, int nodeNum, nite::JointType jointType );

		CameraPersp mCam;
};

void OniStickmanApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void OniStickmanApp::setup()
{
	if ( openni::OpenNI::initialize() != openni::STATUS_OK )
	{
		console() << openni::OpenNI::getExtendedError() << endl;
		quit();
	}
	if ( nite::NiTE::initialize() != nite::STATUS_OK )
	{
		console() << "NiTE initialization error." << endl;
		quit();
	}

	try
	{
		mndl::oni::OniCapture::Options options;
		options.mEnableColor = false;
		mOniCaptureRef = mndl::oni::OniCapture::create( openni::ANY_DEVICE, options );
	}
	catch ( const mndl::oni::ExcOpenNI &exc )
	{
		console() << exc.what() << endl;
		quit();
	}

	openni::VideoMode depthMode;
	depthMode.setResolution( 640, 480 );
	depthMode.setFps( 30 );
	depthMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
	mOniCaptureRef->getDepthStreamRef()->setVideoMode( depthMode );

	mUserTrackerRef = std::shared_ptr< nite::UserTracker >( new nite::UserTracker() );
	if ( mUserTrackerRef->create( mOniCaptureRef->getDeviceRef().get() ) != nite::STATUS_OK )
	{
		console() << "Error creating UserTracker." << endl;
		quit();
	}
	mUserTrackerRef->addNewFrameListener( this );
	mUserTrackerRef->setSkeletonSmoothingFactor( .7f );

	mOniCaptureRef->start();

	// stickman nodes
	for ( int i = 0; i < sNumBones; i++ )
	{
		mBones[ i ].setPosition( Vec3f( -1000, -1000, -1000 ) );
		mBones[ i ].setScale( 10 );

		// setup T pose
		// left arm
		if ( i == 2 || i == 4 )
		{
			mBones[ i ].mBindPoseOrientation.set( Vec3f( 0, 0, 1 ), toRadians( -90.f ) );
		}
		else // right arm
		if ( i == 3 || i == 5 )
		{
			mBones[ i ].mBindPoseOrientation.set( Vec3f( 0, 0, 1 ), toRadians( 90.f ) );
		}
		else // neck
		if ( i == 1 )
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

void OniStickmanApp::shutdown()
{
	if ( mUserTrackerRef )
	{
		mUserTrackerRef->removeNewFrameListener( this );
		mUserTrackerRef->destroy();
	}
	nite::NiTE::shutdown();
	openni::OpenNI::shutdown();
}

void OniStickmanApp::onNewFrame( nite::UserTracker &userTracker )
{
	nite::UserTrackerFrameRef frame;
	if ( mUserTrackerRef->readFrame( &frame ) != nite::STATUS_OK )
		return;

	const nite::Array< nite::UserData > &users = frame.getUsers();
	if ( users.isEmpty() )
	{
		return;
	}
	if ( users[ 0 ].isNew() )
	{
		mUserTrackerRef->startSkeletonTracking( users[ 0 ].getId() );
	}
	nite::Skeleton skeleton = users[ 0 ].getSkeleton();
	if ( skeleton.getState() != nite::SKELETON_TRACKED )
	{
		return;
	}

	transformNode( skeleton, 0, nite::JOINT_TORSO );
	transformNode( skeleton, 1, nite::JOINT_NECK );
	transformNode( skeleton, 2, nite::JOINT_LEFT_SHOULDER );
	transformNode( skeleton, 3, nite::JOINT_RIGHT_SHOULDER );
	transformNode( skeleton, 4, nite::JOINT_LEFT_ELBOW );
	transformNode( skeleton, 5, nite::JOINT_RIGHT_ELBOW );
	transformNode( skeleton, 6, nite::JOINT_LEFT_HIP );
	transformNode( skeleton, 7, nite::JOINT_RIGHT_HIP );
	transformNode( skeleton, 8, nite::JOINT_LEFT_KNEE );
	transformNode( skeleton, 9, nite::JOINT_RIGHT_KNEE );
}

void OniStickmanApp::update()
{
	if ( mOniCaptureRef->checkNewDepthFrame() )
		mDepthTexture = mOniCaptureRef->getDepthImage();
}

void OniStickmanApp::transformNode( const nite::Skeleton &skeleton, int nodeNum, nite::JointType jointType )
{
	lock_guard< recursive_mutex > lock( mMutex );
	const nite::SkeletonJoint &joint = skeleton.getJoint( jointType );
	Quatf ori = mndl::oni::fromOni( joint.getOrientation() );
	float oriConf = joint.getOrientationConfidence();
	Vec3f pos = mndl::oni::fromOni( joint.getPosition() );
	float posConf = joint.getPositionConfidence();

	if ( oriConf > 0 && posConf > 0 )
	{
		mBones[ nodeNum ].setPosition( pos );

		// apply skeleton pose relatively to the bone bind pose
		mBones[ nodeNum ].setOrientation( mBones[ nodeNum ].mBindPoseOrientation * ori );
	}
}

void OniStickmanApp::draw()
{
	gl::clear();
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	gl::color( Color::white() );
	if ( mDepthTexture )
		gl::draw( mDepthTexture, Rectf( 0, 0, 160, 120 ) );

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
		lock_guard< recursive_mutex > lock( mMutex );
		mBones[ i ].draw();
	}
	light.disable();
	gl::disable( GL_LIGHTING );
}

CINDER_APP_BASIC( OniStickmanApp, RendererGl )

