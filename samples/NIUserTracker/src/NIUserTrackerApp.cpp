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

#include "CiNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class NIUserTrackerApp : public AppBasic
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
};

void NIUserTrackerApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void NIUserTrackerApp::setup()
{
	try
	{
		mNI = ni::OpenNI( ni::OpenNI::Device() );
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
}

void NIUserTrackerApp::update()
{
	if ( mNI.checkNewVideoFrame() )
		mColorTexture = mNI.getVideoImage();
}

void NIUserTrackerApp::draw()
{
	gl::clear( Color::black() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	gl::color( Color::white() );
	if ( mColorTexture )
		gl::draw( mColorTexture );

	gl::enableAlphaBlending();
	gl::color( ColorA( 1, 0, 0, .5 ) );

	const XnSkeletonJoint jointIds[] = {
		XN_SKEL_HEAD, XN_SKEL_NECK, XN_SKEL_TORSO, XN_SKEL_WAIST,
		XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW, XN_SKEL_LEFT_HAND,
		XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW, XN_SKEL_RIGHT_HAND,
		XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_FOOT,
		XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE, XN_SKEL_RIGHT_FOOT };

	vector< unsigned > users = mNIUserTracker.getUsers();
	for ( vector< unsigned >::const_iterator it = users.begin();
			it < users.end(); ++it )
	{
		unsigned userId = *it;
		for ( int i = 0; i < sizeof( jointIds ) / sizeof( jointIds[0] );
				++i )
		{
			float conf;
			Vec2f joint = mNIUserTracker.getJoint2d( userId, jointIds[i], &conf );

			if ( conf > .9 )
			{
				gl::drawSolidCircle( joint, 7 );
			}
		}
	}
	gl::disableAlphaBlending();
}

CINDER_APP_BASIC( NIUserTrackerApp, RendererGl() )

