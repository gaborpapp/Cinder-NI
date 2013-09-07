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

class OpenNI
{
	public:
		//! Options for specifying OpenNI parameters
		class Options
		{
			public:
				/*
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
				*/
		};

		/*
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
		*/

		//! Parent class for all OpenNI exceptions
		class Exception : public ci::Exception
		{
			public:
				virtual const char* what() const throw()
				{
					return mMessage;
				}

			protected:
				char mMessage[ 1024 ];
		};



};

/*
class OpenNIExc : std::exception {};

//! Exception thrown from a failure to open a file recording
class ExcFailedOpenFileRecording : public OpenNIExc {};

//! Exception thrown from a failure to create a depth generator
class ExcFailedDepthGeneratorInit : public OpenNIExc {};

//! Exception thrown from a failure to create an image generator
class ExcFailedImageGeneratorInit : public OpenNIExc {};

//! Exception thrown from a failure to create an IR generator
class ExcFailedIRGeneratorInit : public OpenNIExc {};
*/

} } // namespace mndl::ni

