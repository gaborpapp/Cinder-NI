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
#include "cinder/Quaternion.h"
#include "cinder/Thread.h"
#include "cinder/Vector.h"

#include "OpenNI.h"
#include "NiTE.h"

#include "BufferManager.h"

namespace mndl { namespace oni {

inline ci::Vec3f fromOni( const nite::Point3f &v )
{
	return ci::Vec3f( v.x, v.y, v.z );
}

inline nite::Point3f toOni( const ci::Vec3f &v )
{
	return nite::Point3f( v.x, v.y, v.z );
}

inline ci::Quatf fromOni( const nite::Quaternion &q )
{
	return ci::Quatf( q.w, q.x, q.y, q.z );
}

inline nite::Quaternion toOni( const ci::Quatf &q )
{
	return nite::Quaternion( q.w, q.v.x, q.v.y, q.v.z );
}

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

class OniCapture
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
		std::shared_ptr< openni::VideoStream > getDepthStreamRef()
		{
			if ( mDepthListener ) return mDepthListener->mDepthStreamRef;
			else return std::shared_ptr< openni::VideoStream >();
		}
		std::shared_ptr< openni::VideoStream > getColorStreamRef()
		{
			if ( mColorListener ) return mColorListener->mColorStreamRef;
			else return std::shared_ptr< openni::VideoStream >();
		}

		void start();
		void stop();

		bool checkNewDepthFrame();
		bool checkNewColorFrame();
		ci::ImageSourceRef getDepthImage();
		ci::ImageSourceRef getColorImage();

		class ExcFailedOpenDevice : public ExcOpenNI {};
		class ExcFailedCreateDepthStream : public ExcOpenNI {};
		class ExcFailedStartDepthStream : public ExcOpenNI {};
		class ExcUnknownDepthPixelFormat : public ExcOpenNI {};
		class ExcFailedCreateColorStream : public ExcOpenNI {};
		class ExcFailedStartColorStream : public ExcOpenNI {};
		class ExcUnknownColorPixelFormat : public ExcOpenNI {};
		class ExcFailedReadStream : public ExcOpenNI {};

	protected:
		OniCapture( const char *deviceUri, const Options &options );

		struct DepthListener : public openni::VideoStream::NewFrameListener, public BufferObj
		{
			DepthListener( std::shared_ptr< openni::Device > deviceRef );
			~DepthListener();

			void start();
			void stop();
			void onNewFrame( openni::VideoStream &videoStream );

			std::shared_ptr< openni::VideoStream > mDepthStreamRef;
			BufferManager< uint16_t > mDepthBuffers;
			int mDepthWidth, mDepthHeight;
			bool mNewDepthFrame;

			friend class ImageSourceOniDepth;
		};

		struct ColorListener : public openni::VideoStream::NewFrameListener, public BufferObj
		{
			ColorListener( std::shared_ptr< openni::Device > deviceRef );
			~ColorListener();

			void start();
			void stop();
			void onNewFrame( openni::VideoStream &videoStream );

			std::shared_ptr< openni::VideoStream > mColorStreamRef;
			BufferManager< uint8_t > mColorBuffers;
			int mColorWidth, mColorHeight;
			bool mNewColorFrame;

			friend class ImageSourceOniColor;
		};

		std::shared_ptr< openni::Device > mDeviceRef;
		std::shared_ptr< DepthListener > mDepthListener;
		std::shared_ptr< ColorListener > mColorListener;

		friend class ImageSourceOniDepth;
		friend class ImageSourceOniColor;
};

} } // namespace mndl::oni

