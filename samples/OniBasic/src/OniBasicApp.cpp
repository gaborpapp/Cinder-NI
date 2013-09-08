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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "CinderOni.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class OniBasicApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();
		void shutdown();

		void update();
		void draw();

		void mouseUp( MouseEvent event );

	private:
		mndl::oni::OniCaptureRef mOniCaptureRef;
		gl::Texture mColorTexture, mDepthTexture;
};

void OniBasicApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void OniBasicApp::setup()
{
	if ( openni::OpenNI::initialize() != openni::STATUS_OK )
	{
	    console() << openni::OpenNI::getExtendedError() << endl;
		quit();
	}

	mOniCaptureRef = mndl::oni::OniCapture::create( openni::ANY_DEVICE );
	openni::VideoMode depthMode;
	depthMode.setResolution( 640, 480 );
	depthMode.setFps( 30 );
	depthMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
	mOniCaptureRef->getDepthStream().setVideoMode( depthMode );

	mOniCaptureRef->start();
}

void OniBasicApp::shutdown()
{
	openni::OpenNI::shutdown();
}

void OniBasicApp::update()
{
	/*
	if ( mNI.checkNewVideoFrame() )
		mColorTexture = mNI.getVideoImage();
	*/
	if ( mOniCaptureRef->checkNewDepthFrame() )
		mDepthTexture = mOniCaptureRef->getDepthImage();
}

void OniBasicApp::draw()
{
	gl::clear();
	gl::setViewport( getWindowBounds() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	if ( mDepthTexture )
		gl::draw( mDepthTexture );
	/*
	if ( mColorTexture )
		gl::draw( mColorTexture, Vec2i( 640, 0 ) );
	*/
}

void OniBasicApp::mouseUp( MouseEvent event )
{
	/*
    // toggle infrared video
    mNI.setVideoInfrared( !mNI.isVideoInfrared() );
	*/
}

CINDER_APP_BASIC( OniBasicApp, RendererGl )

