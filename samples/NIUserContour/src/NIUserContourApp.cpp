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

#include "cinder/Cinder.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

#include "CinderOpenCV.h"
#include "opencv2/imgproc/imgproc.hpp"

#include "CiNI.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mndl;

class NIUserMaskApp : public AppBasic
{
	public:
		void prepareSettings( Settings *settings );
		void setup();

		void update();
		void draw();

	private:
		ni::OpenNI mNI;
		ni::UserTracker mNIUserTracker;

		float mBlurAmt;
		float mErodeAmt;
		float mDilateAmt;
		int mThres;

		Shape2d mShape;

		params::InterfaceGl mParams;
};

void NIUserMaskApp::prepareSettings(Settings *settings)
{
	settings->setWindowSize( 640, 480 );
}

void NIUserMaskApp::setup()
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

	mNIUserTracker = mNI.getUserTracker();

	mNI.setMirrored();
	mNI.start();

	mParams = params::InterfaceGl( "Parameters", Vec2i( 200, 100 ) );
	mBlurAmt = 15.0;
	mParams.addParam( "blur", &mBlurAmt, "min=1 max=15 step=.5" );
	mErodeAmt = 3.0;
	mParams.addParam( "erode", &mErodeAmt, "min=1 max=15 step=.5" );
	mDilateAmt = 7.0;
	mParams.addParam( "dilate", &mDilateAmt, "min=1 max=15 step=.5" );
	mThres = 128;
	mParams.addParam( "threshold", &mThres, "min=1 max=255" );
}

void NIUserMaskApp::update()
{
	if ( mNI.checkNewVideoFrame() )
	{
		Surface8u maskSurface = mNIUserTracker.getUserMask();

		cv::Mat cvMask, cvMaskFiltered;
		cvMask = toOcv( Channel8u( maskSurface ) );
		cv::blur( cvMask, cvMaskFiltered, cv::Size( mBlurAmt, mBlurAmt ) );

		cv::Mat dilateElm = cv::getStructuringElement( cv::MORPH_RECT,
				cv::Size( mDilateAmt, mDilateAmt ) );
		cv::Mat erodeElm = cv::getStructuringElement( cv::MORPH_RECT,
				cv::Size( mErodeAmt, mErodeAmt ) );
		cv::erode( cvMaskFiltered, cvMaskFiltered, erodeElm, cv::Point( -1, -1 ), 1 );
		cv::dilate( cvMaskFiltered, cvMaskFiltered, dilateElm, cv::Point( -1, -1 ), 3 );
		cv::erode( cvMaskFiltered, cvMaskFiltered, erodeElm, cv::Point( -1, -1 ), 1 );
		cv::blur( cvMaskFiltered, cvMaskFiltered, cv::Size( mBlurAmt, mBlurAmt ) );

		cv::threshold( cvMaskFiltered, cvMaskFiltered, mThres, 255, CV_THRESH_BINARY);

		vector< vector< cv::Point > > contours;
		cv::findContours( cvMaskFiltered, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE );

		mShape.clear();
		for ( vector< vector< cv::Point > >::const_iterator it = contours.begin();
				it != contours.end(); ++it )
		{
			vector< cv::Point >::const_iterator pit = it->begin();

			if ( it->empty() )
				continue;

			mShape.moveTo( fromOcv( *pit ) );

			++pit;
			for ( ; pit != it->end(); ++pit )
			{
				mShape.lineTo( fromOcv( *pit ) );
			}
			mShape.close();
		}
	}
}

void NIUserMaskApp::draw()
{
	gl::clear( Color::black() );
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );

	glLineWidth( 2.0 );
	gl::draw( mShape );

	params::InterfaceGl::draw();
}

CINDER_APP_BASIC( NIUserMaskApp, RendererGl() )

