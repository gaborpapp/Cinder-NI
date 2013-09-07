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

#include <cstdio>

#include "cinder/app/App.h"

#include "CiNI.h"

namespace mndl { namespace ni {

DeviceStream::DeviceStream( const char *deviceUri, const Options &options )
{
	mDeviceRef = std::shared_ptr< openni::Device >( new openni::Device() );

	openni::Status rc;
	rc = mDeviceRef->open( deviceUri );
	if ( rc != openni::STATUS_OK )
	{
		throw ExcFailedOpenDevice();
	}

	if ( options.mEnableDepth )
	{
		mDepthStreamRef = std::shared_ptr< openni::VideoStream >( new openni::VideoStream() );
		if ( mDepthStreamRef->create( *mDeviceRef.get(), openni::SENSOR_DEPTH ) )
		{
			throw ExcFailedCreateDepthStream();
		}
		mDepthStreamRef->addNewFrameListener( this );
	}
}

DeviceStream::~DeviceStream()
{
	if ( mDepthStreamRef )
	{
		mDepthStreamRef->stop();
		mDepthStreamRef->removeNewFrameListener( this );
		mDepthStreamRef->destroy();
	}

	if ( mDeviceRef )
	{
		mDeviceRef->close();
	}
}

void DeviceStream::start()
{
	if ( mDepthStreamRef )
	{
		if ( mDepthStreamRef->start() != openni::STATUS_OK )
		{
			throw ExcFailedStartDepthStream();
		}
	}
}

void DeviceStream::stop()
{
	if ( mDepthStreamRef )
	{
		mDepthStreamRef->stop();
	}
}

void DeviceStream::onNewFrame( openni::VideoStream &videoStream )
{
	openni::VideoFrameRef frame;
	videoStream.readFrame( &frame );
	switch ( frame.getSensorType() )
	{
		case openni::SENSOR_DEPTH:
		{
			break;
		}
		default:
			break;
	}
}

ExcOpenNI::ExcOpenNI() throw()
{
	sprintf( mMessage, "%s", openni::OpenNI::getExtendedError() );
}

} } // namespace mndl::ni

