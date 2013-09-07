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
#include "cinder/Thread.h"

#include "OpenNI.h"

#include "CiNIBufferManager.h"

namespace mndl { namespace ni {

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

typedef std::shared_ptr< class DeviceStream > DeviceStreamRef;

class DeviceStream : public openni::VideoStream::NewFrameListener
{
	public:
		//! Options for specifying DeviceStream parameters
		struct Options
		{
			Options() : mEnableDepth( true ), mEnableColor( true ), mEnableIR( false ) {}

			bool mEnableDepth;
			bool mEnableColor;
			bool mEnableIR;
		};

		static DeviceStreamRef create( const char *deviceUri, const Options &options = Options() )
		{ return DeviceStreamRef( new DeviceStream( deviceUri, options ) ); }

		~DeviceStream();

		void start();
		void stop();

		class ExcFailedOpenDevice : public ExcOpenNI {};
		class ExcFailedCreateDepthStream : public ExcOpenNI {};
		class ExcFailedStartDepthStream : public ExcOpenNI {};

	protected:
		DeviceStream( const char *deviceUri, const Options &options );

		std::shared_ptr< openni::Device > mDeviceRef;

		std::mutex mMutex;

		std::shared_ptr< openni::VideoStream > mDepthStreamRef;
		std::shared_ptr< openni::VideoStream > mColorStreamRef;

		void onNewFrame( openni::VideoStream &videoStream );
};

} } // namespace mndl::ni

