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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Thread.h"

#include <map>

namespace mndl { namespace oni {

struct BufferObj
{
 public:
	std::recursive_mutex mMutex;
};

template<typename T>
struct BufferManager
{
	BufferManager() {};
	BufferManager( size_t allocationSize, BufferObj *bufferObj )
		: mAllocationSize( allocationSize ), mBufferObj( bufferObj ), mActiveBuffer( 0 )
	{}

	~BufferManager();

	T*			getNewBuffer();
	void		setActiveBuffer( T *buffer );
	void		derefActiveBuffer();
	T*			refActiveBuffer();
	void		derefBuffer( T *buffer );

	BufferObj				*mBufferObj;
	size_t					mAllocationSize;
	// map from pointer to reference count
	std::map< T*, size_t >	mBuffers;
	T						*mActiveBuffer;
};

template<typename T>
BufferManager<T>::~BufferManager()
{
	for ( typename std::map<T*,size_t>::iterator bufIt = mBuffers.begin(); bufIt != mBuffers.end(); ++bufIt ) {
		delete [] bufIt->first;
	}
}

template<typename T>
T* BufferManager<T>::getNewBuffer()
{
	std::lock_guard<std::recursive_mutex> lock( mBufferObj->mMutex );

	typename std::map<T*,size_t>::iterator bufIt;
	for ( bufIt = mBuffers.begin(); bufIt != mBuffers.end(); ++bufIt ) {
		if( bufIt->second == 0 ) // 0 means free buffer
			break;
	}
	if ( bufIt != mBuffers.end() ) {
		bufIt->second = 1;
		return bufIt->first;
	}
	else { // there were no available buffers - add a new one and return it
		T *newBuffer = new T[ mAllocationSize ];
		mBuffers[ newBuffer ] = 1;
		return newBuffer;
	}
}

template<typename T>
void BufferManager<T>::setActiveBuffer( T *buffer )
{
	std::lock_guard<std::recursive_mutex> lock( mBufferObj->mMutex );
	// assign new active buffer
	mActiveBuffer = buffer;
}

template<typename T>
T* BufferManager<T>::refActiveBuffer()
{
	std::lock_guard<std::recursive_mutex> lock( mBufferObj->mMutex );
	mBuffers[ mActiveBuffer ]++;
	return mActiveBuffer;
}

template<typename T>
void BufferManager<T>::derefActiveBuffer()
{
	std::lock_guard<std::recursive_mutex> lock( mBufferObj->mMutex );
	if ( mActiveBuffer )	// decrement use count on current active buffer
		mBuffers[ mActiveBuffer ]--;
}

template<typename T>
void BufferManager<T>::derefBuffer( T *buffer )
{
	std::lock_guard<std::recursive_mutex> lock( mBufferObj->mMutex );
	mBuffers[ buffer ]--;
}

// Used as the deleter for the shared_ptr returned by getImageData()
// and getDepthData()
template<typename T>
class DataDeleter
{
 public:
	DataDeleter( BufferManager<T> *bufferMgr, std::shared_ptr<BufferObj> ownerObj )
		: mBufferMgr( bufferMgr ), mOwnerObj( ownerObj )
	{}

	void operator()( T *data ) {
		mBufferMgr->derefBuffer( data );
	}

	std::shared_ptr<BufferObj>	mOwnerObj; // to prevent deletion of our parent Obj
	BufferManager<T> *mBufferMgr;
};

} } // namespace mndl::oni
