/*
 Copyright (c) 2012, Gabor Papp, All rights reserved.

 This code is intended for use with the Cinder C++ library:
 http://libcinder.org

 Partially based on the Cinder-Kinect block:
 https://github.com/cinder/Cinder-Kinect

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <exception>
#include <iostream>
#include <map>
#include <string>

#include "cinder/Cinder.h"
#include "cinder/Thread.h"
#include "cinder/Vector.h"
#include "cinder/Area.h"
#include "cinder/Exception.h"
#include "cinder/ImageIo.h"
#include "cinder/app/App.h"

#include "cinder/Surface.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Function.h"

#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnHash.h>
#include <XnLog.h>

#include "CiNIBufferManager.h"
#include "CiNIUserTracker.h"

namespace mndl { namespace ni {

inline bool checkRc( XnStatus rc, std::string what )
{
	if ( rc != XN_STATUS_OK )
	{
		ci::app::console() << "OpenNI Error - " << what << " : " << xnGetStatusString( rc ) << std::endl;
		return false;
	}
	else
		return true;
}

class OpenNI
{
	public:
		//! Options for specifying OpenNI generators
		class Options
		{
			public:
				Options( bool enableDepth = true, bool enableImage = true,
						 bool enableIR = true, bool enableUserTracker = true )
					: mDepthEnabled( enableDepth ), mImageEnabled( enableImage ),
					  mIREnabled( enableIR ), mUserTrackerEnabled( enableUserTracker )
				{}

				Options &enableDepth( bool enable = true ) { mDepthEnabled = enable; return *this; }
				bool getDepthEnabled() const { return mDepthEnabled; }
				void setDepthEnabled( bool enable = true ) { mDepthEnabled = enable; }

				Options &enableImage( bool enable = true ) { mImageEnabled = enable; return *this; }
				bool getImageEnabled() const { return mImageEnabled; }
				void setImageEnabled( bool enable = true ) { mImageEnabled = enable; }

				Options &enableIR( bool enable = true ) { mIREnabled = enable; return *this; }
				bool getIREnabled() const { return mIREnabled; }
				void setIREnabled( bool enable = true ) { mIREnabled = enable; }

				Options &enableUserTracker( bool enable = true ) { mUserTrackerEnabled = enable; return *this; }
				bool getUserTrackerEnabled() const { return mUserTrackerEnabled; }
				void setUserTrackerEnabled( bool enable = true ) { mUserTrackerEnabled = enable; }

			private:
				bool mDepthEnabled;
				bool mImageEnabled;
				bool mIREnabled;
				bool mUserTrackerEnabled;
		};

		//! Represents the identifier for a particular OpenNI device
		struct Device {
			Device( int index = 0 )
				: mIndex( index )
			{}

			int     mIndex;
		};

		//! Default constructor - creates an uninitialized instance
		OpenNI() {}

		//! Creates a new OpenNI based on Device # \a device. 0 is the typical value for \a deviceIndex.
		OpenNI( Device device, const Options &options = Options() );

		//! Creates a new OpenNI based on the OpenNI recording from the file path \a path.
		OpenNI( const ci::fs::path &recording, const Options &options = Options() );

		void start();
		void stop();

		//! Returns whether there is a new depth frame available since the last call to checkNewDepthFrame(). Call getDepthImage() to retrieve it.
		bool			checkNewDepthFrame();

		//! Returns whether there is a new video frame available since the last call to checkNewVideoFrame(). Call getVideoImage() to retrieve it.
		bool			checkNewVideoFrame();

		//! Returns latest depth frame.
		ci::ImageSourceRef	getDepthImage();

		//! Returns latest video frame.
		ci::ImageSourceRef	getVideoImage();

		std::shared_ptr<uint8_t> getVideoData();
		std::shared_ptr<uint16_t> getDepthData();

		//! Sets the video image returned by getVideoImage() and getVideoData() to be infrared when \a infrared is true, color when it's false (the default)
		void			setVideoInfrared( bool infrared = true );

		//! Returns whether the video image returned by getVideoImage() and getVideoData() is infrared when \c true, or color when it's \c false (the default)
		bool			isVideoInfrared() const { return mObj->mVideoInfrared; }

		//! Calibrates depth to video frame.
		void			setDepthAligned( bool aligned = true );
		bool			isDepthAligned() const { return mObj->mDepthAligned; }

		void			setMirrored( bool mirror = true );
		bool			isMirrored() const { return mObj->mMirrored; }

		void			startRecording( const ci::fs::path &filename );
		void			stopRecording();
		bool			isRecording() const { return mObj->mRecording; }

		UserTracker		getUserTracker() { return mObj->mUserTracker; }

	protected:
		class Obj : public BufferObj {
			public:
				Obj( int deviceIndex, const Options &options );
				Obj( const ci::fs::path &recording, const Options &options );
				~Obj();

				void start();
				void stop();

				BufferManager<uint8_t> mColorBuffers;
				BufferManager<uint16_t> mDepthBuffers;

				static void threadedFunc( struct OpenNI::Obj *arg );

				xn::Context mContext;

				xn::DepthGenerator mDepthGenerator;
				xn::DepthMetaData mDepthMD;
				int mDepthWidth;
				int mDepthHeight;
				int mDepthMaxDepth;

				xn::ImageGenerator mImageGenerator;
				xn::ImageMetaData mImageMD;
				int mImageWidth;
				int mImageHeight;

				xn::IRGenerator mIRGenerator;
				xn::IRMetaData mIRMD;
				int mIRWidth;
				int mIRHeight;

				UserTracker mUserTracker;

				xn::Recorder mRecorder;
				bool mRecording;

				Options mOptions;

				void generateDepth();
				void generateImage();
				void generateIR();

				std::shared_ptr<std::thread> mThread;

				volatile bool mShouldDie;
				volatile bool mNewDepthFrame, mNewVideoFrame;
				volatile bool mVideoInfrared;
				volatile bool mLastVideoFrameInfrared;

				volatile bool mDepthAligned;
				volatile bool mMirrored;
		};

		friend class ImageSourceOpenNIColor;
		friend class ImageSourceOpenNIInfrared;
		friend class ImageSourceOpenNIDepth;

		//friend class UserTracker;

		std::shared_ptr<Obj> mObj;

	public:
		//@{
		//! Emulates shared_ptr-like behavior
		typedef std::shared_ptr<Obj> OpenNI::*unspecified_bool_type;
		operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &OpenNI::mObj; }
		void reset() { mObj.reset(); }
		//@}
};

//! Parent class for all OpenNI exceptions
class OpenNIExc : std::exception {};

//! Exception thrown from a failure to open a file recording
class ExcFailedOpenFileRecording : public OpenNIExc {};

//! Exception thrown from a failure to create a depth generator
class ExcFailedDepthGeneratorInit : public OpenNIExc {};

//! Exception thrown from a failure to create an image generator
class ExcFailedImageGeneratorInit : public OpenNIExc {};

//! Exception thrown from a failure to create an IR generator
class ExcFailedIRGeneratorInit : public OpenNIExc {};

} } // namespace mndl::ni

