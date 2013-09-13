/*
 Copyright (c) 2012-2013, Gabor Papp

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

#include <limits>

#include "cinder/Cinder.h"
#include "cinder/Thread.h"
#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "CinderOni.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class OniUserTrackerApp : public AppBasic
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
};

void OniUserTrackerApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void OniUserTrackerApp::setup()
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

	mndl::oni::OniCapture::Options options;
	options.mEnableColor = false;
	mOniCaptureRef = mndl::oni::OniCapture::create( openni::ANY_DEVICE, options );
	openni::VideoMode depthMode;
	depthMode.setResolution( 640, 480 );
	depthMode.setFps( 30 );
	depthMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
	mOniCaptureRef->getDepthStreamRef()->setVideoMode( depthMode );

	/* FIXME: limitation of NiTE 2.2 on OS X, ini and data searchpath issue
	 * copy NiTE.ini next to OniUserTrackerApp.app and set
	 * [General]
	 * DataDir=./OniUserTracker.app/Contents/MacOS/NiTE2
	 * run the app as:
	 * ./OniUserTracker/Contents/MacOS/OniUserTracker
	 * see: http://community.openni.org/openni/topics/using_nite_from_os_x_app
	 */
	mUserTrackerRef = std::shared_ptr< nite::UserTracker >( new nite::UserTracker() );
	if ( mUserTrackerRef->create( mOniCaptureRef->getDeviceRef().get() ) != nite::STATUS_OK )
	{
		console() << "Error creating UserTracker." << endl;
		quit();
	}
	mUserTrackerRef->setSkeletonSmoothingFactor( .7f );

	mOniCaptureRef->start();
}

void OniUserTrackerApp::shutdown()
{
	if ( mUserTrackerRef )
	{
		mUserTrackerRef->destroy();
	}
	nite::NiTE::shutdown();
	openni::OpenNI::shutdown();
}

void OniUserTrackerApp::update()
{
	if ( mOniCaptureRef->checkNewDepthFrame() )
		mDepthTexture = mOniCaptureRef->getDepthImage();
}

void OniUserTrackerApp::draw()
{
	gl::clear();
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	gl::color( Color::white() );
	if ( mDepthTexture )
	{
		gl::draw( mDepthTexture );
	}

	const nite::JointType jointIds[] = {
		nite::JOINT_HEAD, nite::JOINT_NECK,
		nite::JOINT_LEFT_SHOULDER, nite::JOINT_RIGHT_SHOULDER,
		nite::JOINT_LEFT_ELBOW, nite::JOINT_RIGHT_ELBOW,
		nite::JOINT_LEFT_HAND, nite::JOINT_RIGHT_HAND,
		nite::JOINT_TORSO,
		nite::JOINT_LEFT_HIP, nite::JOINT_RIGHT_HIP,
		nite::JOINT_LEFT_KNEE, nite::JOINT_RIGHT_KNEE,
		nite::JOINT_LEFT_FOOT, nite::JOINT_RIGHT_FOOT };

	nite::UserTrackerFrameRef frame;
	if ( mUserTrackerRef->readFrame( &frame ) == nite::STATUS_OK )
	{
		gl::enableAlphaBlending();
		gl::color( ColorA( 1, 0, 0, .5 ) );

		const nite::Array< nite::UserData > &users = frame.getUsers();
		for ( int i = 0; i < users.getSize(); i++ )
		{
			if ( !users[ i ].isVisible() )
			{
				continue;
			}
			Vec3f centerOfMass = mndl::oni::fromOni( users[ i ].getCenterOfMass() );
			Vec2f scrPos;
			mUserTrackerRef->convertJointCoordinatesToDepth( centerOfMass.x,
					centerOfMass.y, centerOfMass.z,
					&scrPos.x, &scrPos.y );
			gl::drawStringCentered( toString( centerOfMass ), scrPos );

			if ( users[ i ].isNew() )
			{
				mUserTrackerRef->startSkeletonTracking( users[ i ].getId() );
			}
			nite::Skeleton skeleton = users[ i ].getSkeleton();
			if ( skeleton.getState() != nite::SKELETON_TRACKED )
			{
				continue;
			}

			for ( int j = 0; j < sizeof( jointIds ) / sizeof( jointIds[ 0 ] );
					++j )
			{
				const nite::SkeletonJoint &joint = skeleton.getJoint( jointIds[ j ] );
				Vec3f pos = mndl::oni::fromOni( joint.getPosition() );
				float posConf = joint.getPositionConfidence();

				if ( posConf < 0.5f )
					continue;

				Vec2f pos2d;
				mUserTrackerRef->convertJointCoordinatesToDepth( pos.x, pos.y, pos.z,
						&pos2d.x, &pos2d.y );
				gl::drawSolidCircle( pos2d, 7 );
			}
		}
		gl::disableAlphaBlending();
	}
}

CINDER_APP_BASIC( OniUserTrackerApp, RendererGl )

