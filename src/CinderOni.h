/*
 Copyright (c) 2012-2013, Gabor Papp, All rights reserved.

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

#include "cinder/Cinder.h"
#include "cinder/Exception.h"
#include "cinder/ImageIo.h"
#include "cinder/Thread.h"

#include "OpenNI.h"

#include "BufferManager.h"

namespace mndl { namespace oni {

//! Parent class for all OpenNI exceptions
class ExcOpenNI : public ci::Exception
{
	public:
		ExcOpenNI() throw();

		virtual const char* what() const throw()
		{
			return mMessage;
		}

	protected:
		char mMessage[ 1024 ];
};

typedef std::shared_ptr< class OniCapture > OniCaptureRef;

class OniCapture : public openni::VideoStream::NewFrameListener, public BufferObj
{
	public:
		//! Options for specifying OniCapture parameters
		struct Options
		{
			Options() : mEnableDepth( true ), mEnableColor( true ), mEnableIR( false ) {}

			bool mEnableDepth;
			bool mEnableColor;
			bool mEnableIR;
		};

		static OniCaptureRef create( const char *deviceUri, const Options &options = Options() )
		{ return OniCaptureRef( new OniCapture( deviceUri, options ) ); }

		~OniCapture();

		std::shared_ptr< openni::Device > getDeviceRef() { return mDeviceRef; }
		openni::VideoStream & getDepthStream() { return mDepthStream; }

		void start();
		void stop();

		bool checkNewDepthFrame();
		ci::ImageSourceRef getDepthImage();

		class ExcFailedOpenDevice : public ExcOpenNI {};
		class ExcFailedCreateDepthStream : public ExcOpenNI {};
		class ExcFailedStartDepthStream : public ExcOpenNI {};
		class ExcFailedReadStream : public ExcOpenNI {};
		class ExcUnknownDepthFrameFormat : public ExcOpenNI {};

	protected:
		OniCapture( const char *deviceUri, const Options &options );

		std::shared_ptr< openni::Device > mDeviceRef;

		openni::VideoStream mDepthStream;
		BufferManager< uint16_t > mDepthBuffers;
		int mDepthWidth, mDepthHeight;
		bool mNewDepthFrame;

		openni::VideoStream mColorStreamRef;
		BufferManager< uint8_t > mColorBuffers;

		void onNewFrame( openni::VideoStream &videoStream );

		friend class ImageSourceOniDepth;
};

} } // namespace mndl::oni

