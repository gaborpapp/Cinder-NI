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

#include "CinderOni.h"

namespace mndl { namespace oni {

class ImageSourceOniDepth : public ci::ImageSource
{
	public:
		ImageSourceOniDepth( uint16_t *buffer, int w, int h, OniCapture *ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ci::ImageIo::CM_GRAY );
			setChannelOrder( ci::ImageIo::Y );
			setDataType( ci::ImageIo::UINT16 );
		}

		~ImageSourceOniDepth()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mDepthBuffers.derefBuffer( mData );
		}

		virtual void load( ci::ImageTargetRef target )
		{
			ci::ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth );
		}

	protected:
		OniCapture *mOwnerObj;
		uint16_t *mData;
};

OniCapture::OniCapture( const char *deviceUri, const Options &options ) :
	mNewDepthFrame( false )
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
		if ( mDepthStream.create( *mDeviceRef.get(), openni::SENSOR_DEPTH ) )
		{
			throw ExcFailedCreateDepthStream();
		}
	}
}

OniCapture::~OniCapture()
{
	stop();

	if ( mDepthStream.isValid() )
	{
		mDepthStream.destroy();
	}

	if ( mDeviceRef )
	{
		mDeviceRef->close();
	}
}

void OniCapture::start()
{
	if ( mDepthStream.isValid() )
	{
		if ( mDepthStream.start() != openni::STATUS_OK )
		{
			throw ExcFailedStartDepthStream();
		}

		openni::PixelFormat pf = mDepthStream.getVideoMode().getPixelFormat();
		if ( pf != openni::PIXEL_FORMAT_DEPTH_1_MM &&
			 pf != openni::PIXEL_FORMAT_DEPTH_100_UM )
		{
			throw ExcUnknownDepthFrameFormat();
		}

		mDepthWidth = mDepthStream.getVideoMode().getResolutionX();
		mDepthHeight = mDepthStream.getVideoMode().getResolutionY();
		mDepthBuffers = BufferManager< uint16_t >( mDepthWidth * mDepthHeight, this );

		mDepthStream.addNewFrameListener( this );
	}
}

void OniCapture::stop()
{
	if ( mDepthStream.isValid() )
	{
		mDepthStream.stop();
		mDepthStream.removeNewFrameListener( this );
	}
}

void OniCapture::onNewFrame( openni::VideoStream &videoStream )
{
	openni::VideoFrameRef frame;
	openni::Status rc = videoStream.readFrame( &frame );
	if ( rc != openni::STATUS_OK )
	{
		throw ExcFailedReadStream();
	}

	switch ( frame.getSensorType() )
	{
		case openni::SENSOR_DEPTH:
		{
			std::lock_guard< std::recursive_mutex > lock( mMutex );
			const int minDepthValue = videoStream.getMinPixelValue();
			const int maxDepthValue = videoStream.getMaxPixelValue();
			const uint16_t *depth = reinterpret_cast< const uint16_t * >( frame.getData() );
			mDepthBuffers.derefActiveBuffer(); // finished with current active buffer
			uint16_t *destPixels = mDepthBuffers.getNewBuffer(); // request a new buffer
			const uint32_t depthScale = 0xffff0000 / ( maxDepthValue - minDepthValue );
			for ( size_t p = 0; p < mDepthWidth * mDepthHeight; ++p )
			{
				uint32_t v = depth[ p ] - minDepthValue;
				destPixels[ p ] = ( depthScale * v ) >> 16;
			}
			mDepthBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
			mNewDepthFrame = true; // flag that there's a new depth frame
			break;
		}

		default:
			break;
	}
}

bool OniCapture::checkNewDepthFrame()
{
	std::lock_guard< std::recursive_mutex > lock( mMutex );
	bool oldValue = mNewDepthFrame;
	mNewDepthFrame = false;
	return oldValue;
}

ci::ImageSourceRef OniCapture::getDepthImage()
{
	// register a reference to the active buffer
	uint16_t *activeDepth = mDepthBuffers.refActiveBuffer();
	return ci::ImageSourceRef( new ImageSourceOniDepth( activeDepth,
				mDepthWidth, mDepthHeight, this ) );
}

ExcOpenNI::ExcOpenNI() throw()
{
	sprintf( mMessage, "%s", openni::OpenNI::getExtendedError() );
}

} } // namespace mndl::oni

