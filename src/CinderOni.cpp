/*
 Copyright (c) 2012-2014, Gabor Papp, All rights reserved.

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

#include <algorithm>
#include <cstdio>
#include <limits>

#include "cinder/app/App.h"

#include "CinderOni.h"

namespace mndl { namespace oni {

class ImageSourceOniDepth : public ci::ImageSource
{
 public:
	ImageSourceOniDepth( uint16_t *buffer, int w, int h, std::shared_ptr< OniCapture::DepthListener > ownerObj )
		: ImageSource(), mOwnerObj( ownerObj ), mData( buffer )
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

		for ( int32_t row = 0; row < mHeight; ++row )
			((*this).*func)( target, row, mData + row * mWidth );
	}

 protected:
	std::shared_ptr< OniCapture::DepthListener > mOwnerObj;
	uint16_t *mData;
};

class ImageSourceOniColor : public ci::ImageSource
{
 public:
	ImageSourceOniColor( uint8_t *buffer, int w, int h, std::shared_ptr< OniCapture::ColorListener > ownerObj )
		: ImageSource(), mOwnerObj( ownerObj ), mData( buffer )
	{
		setSize( w, h );
		setColorModel( ci::ImageIo::CM_RGB );
		setChannelOrder( ci::ImageIo::RGB );
		setDataType( ci::ImageIo::UINT8 );
	}

	~ImageSourceOniColor()
	{
		// let the owner know we are done with the buffer
		mOwnerObj->mColorBuffers.derefBuffer( mData );
	}

	virtual void load( ci::ImageTargetRef target )
	{
		ci::ImageSource::RowFunc func = setupRowFunc( target );

		for ( int32_t row = 0; row < mHeight; ++row )
			((*this).*func)( target, row, mData + row * mWidth * 3 );
	}

 protected:
	std::shared_ptr< OniCapture::ColorListener > mOwnerObj;
	uint8_t *mData;
};

OniCapture::DepthListener::DepthListener( std::shared_ptr< openni::Device > deviceRef ) :
	mNewDepthFrame( false ), mDepthHistogramEnabled( false ), mDepthInverted( false )
{
	mDepthStreamRef = std::shared_ptr< openni::VideoStream >( new openni::VideoStream() );
	if ( mDepthStreamRef->create( *deviceRef.get(), openni::SENSOR_DEPTH ) )
	{
		throw ExcFailedCreateDepthStream();
	}
	mDepthHistogram = new float[ std::numeric_limits< uint16_t >::max() + 1 ];
}

OniCapture::DepthListener::~DepthListener()
{
	if ( mDepthStreamRef->isValid() )
	{
		mDepthStreamRef->destroy();
	}
	delete [] mDepthHistogram;
}

void OniCapture::DepthListener::start()
{
	if ( mDepthStreamRef->isValid() )
	{
		if ( mDepthStreamRef->start() != openni::STATUS_OK )
		{
			throw ExcFailedStartDepthStream();
		}

		openni::PixelFormat pf = mDepthStreamRef->getVideoMode().getPixelFormat();
		if ( pf != openni::PIXEL_FORMAT_DEPTH_1_MM &&
			 pf != openni::PIXEL_FORMAT_DEPTH_100_UM )
		{
			throw ExcUnknownDepthPixelFormat();
		}

		mDepthWidth = mDepthStreamRef->getVideoMode().getResolutionX();
		mDepthHeight = mDepthStreamRef->getVideoMode().getResolutionY();
		mDepthBuffers = BufferManager< uint16_t >( mDepthWidth * mDepthHeight, this );

		mDepthStreamRef->addNewFrameListener( this );
	}
}

void OniCapture::DepthListener::stop()
{
	if ( mDepthStreamRef->isValid() )
	{
		mDepthStreamRef->stop();
		mDepthStreamRef->removeNewFrameListener( this );
	}
}

void OniCapture::DepthListener::onNewFrame( openni::VideoStream &videoStream )
{
	openni::VideoFrameRef frame;
	openni::Status rc = videoStream.readFrame( &frame );
	if ( rc != openni::STATUS_OK )
	{
		throw ExcFailedReadStream();
	}

	std::lock_guard< std::recursive_mutex > lock( mMutex );
	const int minDepthValue = videoStream.getMinPixelValue();
	const int maxDepthValue = videoStream.getMaxPixelValue();
	const uint16_t *depth = reinterpret_cast< const uint16_t * >( frame.getData() );
	mDepthBuffers.derefActiveBuffer();
	uint16_t *destPixels = mDepthBuffers.getNewBuffer();

	if ( ! mDepthHistogramEnabled )
	{
		const uint32_t depthScale = 0xffff0000 / ( maxDepthValue - minDepthValue );
		for ( size_t p = 0; p < mDepthWidth * mDepthHeight; ++p )
		{
			uint32_t v = depth[ p ] - minDepthValue;
			destPixels[ p ] = ( depthScale * v ) >> 16;
			if ( mDepthInverted && ( v != 0 ) )
			{
				destPixels[ p ] = 0xffff - destPixels[ p ];
			}
		}
	}
	else
	{
		calcHistogram( depth );
		for ( size_t p = 0; p < mDepthWidth * mDepthHeight; ++p )
		{
			destPixels[ p ] = mDepthHistogram[ depth[ p ] ] * 0xffff;
		}
	}

	mDepthBuffers.setActiveBuffer( destPixels );
	mNewDepthFrame = true;
}

void OniCapture::DepthListener::calcHistogram( const uint16_t *depth )
{
	size_t histogramSize = std::numeric_limits< uint16_t >::max() + 1;
	std::fill( mDepthHistogram, mDepthHistogram + histogramSize,  0.0f );
	size_t pointNum = 0;
	for ( size_t p = 0; p < mDepthWidth * mDepthHeight; ++p )
	{
		uint16_t v = depth[ p ];
		if ( v != 0 )
		{
			mDepthHistogram[ v ]++;
			pointNum++;
		}
	}

	for ( size_t i = 1; i < histogramSize; i++ )
	{
		mDepthHistogram[ i ] += mDepthHistogram[ i - 1 ];
	}

	if ( mDepthInverted )
	{
		for ( size_t i = 1; i < histogramSize; i++ )
		{
			mDepthHistogram[ i ] = ( pointNum - mDepthHistogram[ i ] ) / static_cast< float >( pointNum );
		}
	}
	else
	{
		for ( size_t i = 1; i < histogramSize; i++ )
		{
			mDepthHistogram[ i ] = mDepthHistogram[ i ] / static_cast< float >( pointNum );
		}
	}
}

OniCapture::ColorListener::ColorListener( std::shared_ptr< openni::Device > deviceRef ) :
	mNewColorFrame( false )
{
	mColorStreamRef = std::shared_ptr< openni::VideoStream >( new openni::VideoStream() );
	if ( mColorStreamRef->create( *deviceRef.get(), openni::SENSOR_COLOR ) )
	{
		throw ExcFailedCreateColorStream();
	}
}

OniCapture::ColorListener::~ColorListener()
{
	if ( mColorStreamRef->isValid() )
	{
		mColorStreamRef->destroy();
	}
}

void OniCapture::ColorListener::start()
{
	if ( mColorStreamRef->isValid() )
	{
		if ( mColorStreamRef->start() != openni::STATUS_OK )
		{
			throw ExcFailedStartColorStream();
		}

		openni::PixelFormat pf = mColorStreamRef->getVideoMode().getPixelFormat();
		if ( pf != openni::PIXEL_FORMAT_RGB888 )
		{
			throw ExcUnknownColorPixelFormat();
		}

		mColorWidth = mColorStreamRef->getVideoMode().getResolutionX();
		mColorHeight = mColorStreamRef->getVideoMode().getResolutionY();
		mColorBuffers = BufferManager< uint8_t >( mColorWidth * mColorHeight * 3, this );

		mColorStreamRef->addNewFrameListener( this );
	}
}

void OniCapture::ColorListener::stop()
{
	if ( mColorStreamRef->isValid() )
	{
		mColorStreamRef->stop();
		mColorStreamRef->removeNewFrameListener( this );
	}
}

void OniCapture::ColorListener::onNewFrame( openni::VideoStream &videoStream )
{
	openni::VideoFrameRef frame;
	openni::Status rc = videoStream.readFrame( &frame );
	if ( rc != openni::STATUS_OK )
	{
		throw ExcFailedReadStream();
	}

	std::lock_guard< std::recursive_mutex > lock( mMutex );
	const uint8_t *srcPixels = reinterpret_cast< const uint8_t * >( frame.getData() );
	mColorBuffers.derefActiveBuffer();
	uint8_t *destPixels = mColorBuffers.getNewBuffer();
	int destStride = mColorWidth * 3;
	int srcStride = frame.getStrideInBytes();
	if ( destStride == srcStride )
	{
		memcpy( destPixels, srcPixels, destStride * mColorHeight );
	}
	else
	{
		uint8_t *dst = destPixels;
		for ( int y = 0; y < mColorHeight; y++ )
		{
			memcpy( dst, srcPixels, destStride );
			srcPixels += srcStride;
			dst += destStride;
		}
	}
	mColorBuffers.setActiveBuffer( destPixels );
	mNewColorFrame = true;
}

OniCapture::OniCapture( const char *deviceUri, const Options &options )
{
	mDeviceRef = std::shared_ptr< openni::Device >( new openni::Device() );

	openni::Status rc;
	rc = mDeviceRef->open( deviceUri );
	if ( rc != openni::STATUS_OK )
	{
		throw ExcFailedOpenDevice();
	}

	// TODO: check the file streams, getSensorInfo does not work
	/*
	if ( mDeviceRef->isFile() )
	{
		if ( mDeviceRef->getSensorInfo( openni::SENSOR_DEPTH ) == NULL )
		{
			mDepthListener = std::shared_ptr< DepthListener >( new DepthListener( mDeviceRef ) );
		}
		if ( mDeviceRef->getSensorInfo( openni::SENSOR_COLOR ) != NULL )
		{
			mColorListener = std::shared_ptr< ColorListener >( new ColorListener( mDeviceRef ) );
		}
	}
	*/

	if ( options.mEnableDepth )
	{
		mDepthListener = std::shared_ptr< DepthListener >( new DepthListener( mDeviceRef ) );
	}
	if ( options.mEnableColor )
	{
		mColorListener = std::shared_ptr< ColorListener >( new ColorListener( mDeviceRef ) );
	}
}

OniCapture::~OniCapture()
{
	stop();

	if ( mDepthListener )
	{
		mDepthListener.reset();
	}
	if ( mColorListener )
	{
		mColorListener.reset();
	}

	if ( mDeviceRef )
	{
		mDeviceRef->close();
	}
}

void OniCapture::start()
{
	if ( mDepthListener )
	{
		mDepthListener->start();
	}
	if ( mColorListener )
	{
		mColorListener->start();
	}
}

void OniCapture::stop()
{
	if ( mDepthListener )
	{
		mDepthListener->stop();
	}
	if ( mColorListener )
	{
		mColorListener->stop();
	}
}

bool OniCapture::checkNewDepthFrame()
{
	if ( ! mDepthListener )
	{
		return false;
	}

	std::lock_guard< std::recursive_mutex > lock( mDepthListener->mMutex );
	bool oldValue = mDepthListener->mNewDepthFrame;
	mDepthListener->mNewDepthFrame = false;
	return oldValue;
}

bool OniCapture::checkNewColorFrame()
{
	if ( ! mColorListener )
	{
		return false;
	}

	std::lock_guard< std::recursive_mutex > lock( mColorListener->mMutex );
	bool oldValue = mColorListener->mNewColorFrame;
	mColorListener->mNewColorFrame = false;
	return oldValue;
}

ci::ImageSourceRef OniCapture::getDepthImage()
{
	// register a reference to the active buffer
	uint16_t *activeDepth = mDepthListener->mDepthBuffers.refActiveBuffer();
	return ci::ImageSourceRef( new ImageSourceOniDepth( activeDepth,
				mDepthListener->mDepthWidth, mDepthListener->mDepthHeight, mDepthListener ) );
}

ci::ImageSourceRef OniCapture::getColorImage()
{
	// register a reference to the active buffer
	uint8_t *activeColor = mColorListener->mColorBuffers.refActiveBuffer();
	return ci::ImageSourceRef( new ImageSourceOniColor( activeColor,
				mColorListener->mColorWidth, mColorListener->mColorHeight, mColorListener ) );
}

void OniCapture::enableDepthHistogram( bool enable )
{
	if ( mDepthListener )
	{
		std::lock_guard< std::recursive_mutex > lock( mDepthListener->mMutex );
		mDepthListener->mDepthHistogramEnabled = enable;
	}
}

bool OniCapture::isDepthHistogramEnabled() const
{
	if ( mDepthListener )
	{
		std::lock_guard< std::recursive_mutex > lock( mDepthListener->mMutex );
		return mDepthListener->mDepthHistogramEnabled;
	}
	return false;
}

void OniCapture::invertDepth( bool invert )
{
	if ( mDepthListener )
	{
		std::lock_guard< std::recursive_mutex > lock( mDepthListener->mMutex );
		mDepthListener->mDepthInverted = invert;
	}
}

bool OniCapture::isDepthInverted() const
{
	if ( mDepthListener )
	{
		std::lock_guard< std::recursive_mutex > lock( mDepthListener->mMutex );
		return mDepthListener->mDepthInverted;
	}
	return false;
}

ExcOpenNI::ExcOpenNI() throw()
{
	sprintf( mMessage, "%s", openni::OpenNI::getExtendedError() );
}

} } // namespace mndl::oni
