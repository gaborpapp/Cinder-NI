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

#include "cinder/app/App.h"

#include "CiNI.h"

using namespace xn;
using namespace std;
using namespace ci;
using namespace ci::app;

namespace mndl { namespace ni {

class ImageSourceOpenNIColor : public ImageSource {
	public:
		ImageSourceOpenNIColor( uint8_t *buffer, int w, int h, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_RGB );
			setChannelOrder( ImageIo::RGB );
			setDataType( ImageIo::UINT8 );
		}

		~ImageSourceOpenNIColor()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mColorBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth * 3 );
		}

	protected:
		shared_ptr<OpenNI::Obj>     mOwnerObj;
		uint8_t                     *mData;
};

class ImageSourceOpenNIInfrared : public ImageSource {
	public:
		ImageSourceOpenNIInfrared( uint8_t *buffer, int w, int h, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_GRAY );
			setChannelOrder( ImageIo::Y );
			setDataType( ImageIo::UINT8 );
		}

		~ImageSourceOpenNIInfrared()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mColorBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth );
		}

	protected:
		shared_ptr<OpenNI::Obj>		mOwnerObj;
		uint8_t						*mData;
};

class ImageSourceOpenNIDepth : public ImageSource {
	public:
		ImageSourceOpenNIDepth( uint16_t *buffer, int w, int h, shared_ptr<OpenNI::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_GRAY );
			setChannelOrder( ImageIo::Y );
			setDataType( ImageIo::UINT16 );
		}

		~ImageSourceOpenNIDepth()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mDepthBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth );
		}

	protected:
		shared_ptr<OpenNI::Obj>     mOwnerObj;
		uint16_t                    *mData;
};

OpenNI::OpenNI( Device device )
	: mObj( new Obj( device.mIndex ) )
{
}

OpenNI::OpenNI( const fs::path &recording )
	: mObj( new Obj( recording ) )
{
}

void OpenNI::start()
{
	mObj->start();
}

OpenNI::Obj::Obj( int deviceIndex )
	: mShouldDie( false ),
	  mNewDepthFrame( false ),
	  mNewVideoFrame( false ),
	  mVideoInfrared( false ),
	  mRecording( false )
{
	XnStatus rc = mContext.Init();
	checkRc( rc, "context" );

	// depth
	rc = mDepthGenerator.Create(mContext);
	if ( rc != XN_STATUS_OK )
		throw ExcFailedDepthGeneratorInit();
	mDepthGenerator.GetMetaData( mDepthMD );
	mDepthWidth = mDepthMD.FullXRes();
	mDepthHeight = mDepthMD.FullYRes();
	mDepthMaxDepth = mDepthGenerator.GetDeviceMaxDepth();

	mDepthBuffers = BufferManager<uint16_t>( mDepthWidth * mDepthHeight, this );

	// image
	rc = mImageGenerator.Create(mContext);
	if ( rc != XN_STATUS_OK )
		throw ExcFailedImageGeneratorInit();
	mImageGenerator.GetMetaData( mImageMD );
	mImageWidth = mImageMD.FullXRes();
	mImageHeight = mImageMD.FullYRes();

	mColorBuffers = BufferManager<uint8_t>( mImageWidth * mImageHeight * 3, this );

	// IR
	rc = mIRGenerator.Create(mContext);
	if ( rc != XN_STATUS_OK )
		throw ExcFailedIRGeneratorInit();
	// make new map mode -> default to 640 x 480 @ 30fps
	XnMapOutputMode mapMode;
	mapMode.nXRes = mDepthWidth;
	mapMode.nYRes = mDepthHeight;
	mapMode.nFPS  = 30;
	mIRGenerator.SetMapOutputMode( mapMode );

	mIRGenerator.GetMetaData( mIRMD );
	mIRWidth = mIRMD.FullXRes();
	mIRHeight = mIRMD.FullYRes();

	mLastVideoFrameInfrared = mVideoInfrared;

	// user tracker
	mUserTracker = UserTracker(mContext);
}

OpenNI::Obj::Obj( const fs::path &recording )
	: mShouldDie( false ),
	  mNewDepthFrame( false ),
	  mNewVideoFrame( false ),
	  mVideoInfrared( false ),
	  mRecording( false )
{
	XnStatus rc = mContext.Init();
	checkRc( rc, "context" );

	rc = mContext.OpenFileRecording( (const XnChar *)( recording.c_str()));
	if ( !checkRc( rc, "OpenFileRecording" ) )
	{
		throw ExcFailedOpenFileRecording();
	}

	NodeInfoList list;
	rc = mContext.EnumerateExistingNodes(list);
	if ( rc == XN_STATUS_OK )
	{
		for ( NodeInfoList::Iterator it = list.Begin(); it != list.End(); ++it )
		{
			switch ( (*it).GetDescription().Type )
			{
				case XN_NODE_TYPE_DEPTH:
					(*it).GetInstance( mDepthGenerator );
					break;

				case XN_NODE_TYPE_IMAGE:
					(*it).GetInstance( mImageGenerator );
					break;

				case XN_NODE_TYPE_IR:
					(*it).GetInstance( mIRGenerator );
					break;
			}
		}
	}

	// depth
	if ( mDepthGenerator.IsValid() )
	{
		mDepthGenerator.GetMetaData( mDepthMD );
		mDepthWidth = mDepthMD.FullXRes();
		mDepthHeight = mDepthMD.FullYRes();
		mDepthMaxDepth = mDepthGenerator.GetDeviceMaxDepth();

		mDepthBuffers = BufferManager<uint16_t>( mDepthWidth * mDepthHeight, this );
	}

	// image
	if ( mImageGenerator.IsValid() )
	{
		mImageGenerator.GetMetaData( mImageMD );
		mImageWidth = mImageMD.FullXRes();
		mImageHeight = mImageMD.FullYRes();

		mColorBuffers = BufferManager<uint8_t>( mImageWidth * mImageHeight * 3, this );
		mVideoInfrared = false;
	}

	// IR
	if ( mIRGenerator.IsValid() )
	{
		// make new map mode -> default to 640 x 480 @ 30fps
		XnMapOutputMode mapMode;
		mapMode.nXRes = mDepthWidth;
		mapMode.nYRes = mDepthHeight;
		mapMode.nFPS  = 30;
		mIRGenerator.SetMapOutputMode( mapMode );

		mIRGenerator.GetMetaData( mIRMD );
		mIRWidth = mIRMD.FullXRes();
		mIRHeight = mIRMD.FullYRes();

		mColorBuffers = BufferManager<uint8_t>( mIRWidth * mIRHeight * 3, this );
		mVideoInfrared = true;
	}

	// user tracker
	mUserTracker = UserTracker( mContext );

	mLastVideoFrameInfrared = mVideoInfrared;
}

OpenNI::Obj::~Obj()
{
	mShouldDie = true;
	if ( mThread )
		mThread->join();
	mContext.Shutdown();
}

void OpenNI::Obj::start()
{
	XnStatus rc = mDepthGenerator.StartGenerating();
	checkRc( rc, "DepthGenerator.StartGenerating" );

	if (!mVideoInfrared)
		rc = mImageGenerator.StartGenerating();
	else
		rc = mIRGenerator.StartGenerating();
	checkRc( rc, "Video.StartGenerating" );

	// TODO
	mUserTracker.start();

	mThread = shared_ptr< thread >( new thread( threadedFunc, this ) );
}

void OpenNI::Obj::threadedFunc( OpenNI::Obj *obj )
{
	while ( !obj->mShouldDie )
	{
		XnStatus status = obj->mContext.WaitOneUpdateAll( obj->mDepthGenerator );
		checkRc( status, "WaitOneUpdateAll" );

		obj->generateDepth();
		obj->generateImage();
		obj->generateIR();
	}
}

void OpenNI::Obj::generateDepth()
{
	if ( !mDepthGenerator.IsValid() )
		return;

	{
		lock_guard<recursive_mutex> lock( mMutex );
		mDepthGenerator.GetMetaData( mDepthMD );

		const uint16_t *depth = reinterpret_cast<const uint16_t*>( mDepthMD.Data() );
		mDepthBuffers.derefActiveBuffer(); // finished with current active buffer
		uint16_t *destPixels = mDepthBuffers.getNewBuffer(); // request a new buffer
		uint32_t depthScale = 0xffff0000 / mDepthMaxDepth;

		for (size_t p = 0; p < mDepthWidth * mDepthHeight; ++p )
		{
			uint32_t v = depth[p];
			destPixels[p] = ( depthScale * v ) >> 16;
		}
		mDepthBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
		mNewDepthFrame = true; // flag that there's a new depth frame
	}
}

void OpenNI::Obj::generateImage()
{
	if (!mImageGenerator.IsValid() || !mImageGenerator.IsGenerating())
		return;

	{
		lock_guard<recursive_mutex> lock(mMutex);
		mImageGenerator.GetMetaData( mImageMD );

		mColorBuffers.derefActiveBuffer(); // finished with current active buffer
		uint8_t *destPixels = mColorBuffers.getNewBuffer();  // request a new buffer

		const XnRGB24Pixel *src = mImageMD.RGB24Data();
		uint8_t *dst = destPixels + ( mImageMD.XOffset() + mImageMD.YOffset() * mImageWidth ) * 3;
		for (int y = 0; y < mImageMD.YRes(); ++y)
		{
			memcpy(dst, src, mImageMD.XRes() * 3);

			src += mImageMD.XRes(); // FIXME: FullXRes
			dst += mImageWidth * 3;
		}

		mColorBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
		mNewVideoFrame = true; // flag that there's a new color frame
		mLastVideoFrameInfrared = false;
	}
}

void OpenNI::Obj::generateIR()
{
	if ( !mIRGenerator.IsValid() || !mIRGenerator.IsGenerating() )
		return;

	{
		lock_guard<recursive_mutex> lock( mMutex );

		mColorBuffers.derefActiveBuffer(); // finished with current active buffer
		uint8_t *destPixels = mColorBuffers.getNewBuffer();  // request a new buffer
		const XnIRPixel *src = reinterpret_cast<const XnIRPixel *>( mIRGenerator.GetData() );

		for (size_t p = 0; p < mIRWidth * mIRHeight; ++p )
		{
			destPixels[p] = src[p] / 4;
		}

		mColorBuffers.setActiveBuffer( destPixels ); // set this new buffer to be the current active buffer
		mNewVideoFrame = true; // flag that there's a new color frame
		mLastVideoFrameInfrared = true;
	}
}

bool OpenNI::checkNewDepthFrame()
{
	lock_guard<recursive_mutex> lock( mObj->mMutex );
	bool oldValue = mObj->mNewDepthFrame;
	mObj->mNewDepthFrame = false;
	return oldValue;
}

bool OpenNI::checkNewVideoFrame()
{
	lock_guard<recursive_mutex> lock( mObj->mMutex );
	bool oldValue = mObj->mNewVideoFrame;
	mObj->mNewVideoFrame = false;
	return oldValue;
}

ImageSourceRef OpenNI::getDepthImage()
{
	// register a reference to the active buffer
	uint16_t *activeDepth = mObj->mDepthBuffers.refActiveBuffer();
	return ImageSourceRef( new ImageSourceOpenNIDepth( activeDepth, mObj->mDepthWidth, mObj->mDepthHeight, this->mObj ) );
}

ImageSourceRef OpenNI::getVideoImage()
{
	uint8_t *activeColor = mObj->mColorBuffers.refActiveBuffer();
	if (mObj->mLastVideoFrameInfrared)
	{
		return ImageSourceRef( new ImageSourceOpenNIInfrared( activeColor, mObj->mIRWidth, mObj->mIRHeight, this->mObj ) );
	}
	else
	{
		return ImageSourceRef( new ImageSourceOpenNIColor( activeColor, mObj->mImageWidth, mObj->mImageHeight, this->mObj ) );
	}
}

std::shared_ptr<uint8_t> OpenNI::getVideoData()
{
	// register a reference to the active buffer
	uint8_t *activeColor = mObj->mColorBuffers.refActiveBuffer();
	return shared_ptr<uint8_t>( activeColor, DataDeleter<uint8_t>( &mObj->mColorBuffers, mObj ) );
}

std::shared_ptr<uint16_t> OpenNI::getDepthData()
{
	// register a reference to the active buffer
	uint16_t *activeDepth = mObj->mDepthBuffers.refActiveBuffer();
	return shared_ptr<uint16_t>( activeDepth, DataDeleter<uint16_t>( &mObj->mDepthBuffers, mObj ) );
}

void OpenNI::setVideoInfrared( bool infrared )
{
	if (mObj->mVideoInfrared == infrared)
		return;

	{
		lock_guard<recursive_mutex> lock( mObj->mMutex );

		mObj->mVideoInfrared = infrared;
		if (mObj->mVideoInfrared)
		{
			XnStatus rc = mObj->mImageGenerator.StopGenerating();
			checkRc( rc, "ImageGenerater.StopGenerating" );
			rc = mObj->mIRGenerator.StartGenerating();
			checkRc( rc, "IRGenerater.StartGenerating" );
		}
		else
		{
			XnStatus rc = mObj->mIRGenerator.StopGenerating();
			checkRc( rc, "IRGenerater.StopGenerating" );
			rc = mObj->mImageGenerator.StartGenerating();
			checkRc( rc, "ImageGenerater.StartGenerating" );
		}
	}
}

void OpenNI::setDepthAligned( bool aligned )
{
	if ( mObj->mDepthAligned == aligned )
		return;

    if ( !mObj->mImageGenerator.IsValid() )
        return;

	{
		lock_guard<recursive_mutex> lock( mObj->mMutex );
		if ( mObj->mDepthGenerator.IsCapabilitySupported( XN_CAPABILITY_ALTERNATIVE_VIEW_POINT ) )
		{
			XnStatus rc = XN_STATUS_OK;

			if ( aligned )
				rc = mObj->mDepthGenerator.GetAlternativeViewPointCap().SetViewPoint( mObj->mImageGenerator );
			else
				rc = mObj->mDepthGenerator.GetAlternativeViewPointCap().ResetViewPoint();
			if ( checkRc( rc, "DepthGenerator.GetAlternativeViewPointCap" ) )
				mObj->mDepthAligned = aligned;
		}
		else
		{
			console() << "DepthGenerator alternative view point not supported. " << endl;
		}
	}
}

void OpenNI::setMirrored( bool mirror )
{
	if ( mObj->mMirrored == mirror )
		return;
	XnStatus rc = mObj->mContext.SetGlobalMirror(mirror);
	if ( checkRc( rc, "Context.SetGlobalMirror" ) )
		mObj->mMirrored = mirror;
}

void OpenNI::startRecording( const fs::path &filename )
{
	if ( !mObj->mRecording )
	{
		XnStatus rc;
		rc = mObj->mContext.FindExistingNode( XN_NODE_TYPE_RECORDER, mObj->mRecorder );
		if ( !checkRc( rc, "Context.FindExistingNode Recorder " ) )
		{
			rc = mObj->mRecorder.Create( mObj->mContext );
			if ( !checkRc( rc, "Recorder.Create" ) )
				return;
		}
		rc = mObj->mRecorder.SetDestination( XN_RECORD_MEDIUM_FILE, (const XnChar *)( filename.c_str()));
		checkRc( rc, "Recorder.SetDestination" );

		if ( mObj->mDepthGenerator.IsValid() )
		{
			rc = mObj->mRecorder.AddNodeToRecording( mObj->mDepthGenerator, XN_CODEC_16Z_EMB_TABLES );
			checkRc( rc, "Recorder.AddNodeToRecording depth" );
		}

		if ( mObj->mImageGenerator.IsValid() && !mObj->mVideoInfrared )
		{
			rc = mObj->mRecorder.AddNodeToRecording( mObj->mImageGenerator, XN_CODEC_JPEG );
			checkRc( rc, "Recorder.AddNodeToRecording image" );
		}

		if ( mObj->mIRGenerator.IsValid() && mObj->mVideoInfrared )
		{
			rc = mObj->mRecorder.AddNodeToRecording( mObj->mIRGenerator, XN_CODEC_NULL );
			checkRc( rc, "Recorder.AddNodeToRecording image" );
		}
		mObj->mRecording = true;
	}
}

void OpenNI::stopRecording()
{
	if ( mObj->mRecording )
	{
		mObj->mRecorder.Release();
		mObj->mRecording = false;
	}
}

} } // namespace mndl::ni

