#pragma once

#include <vector>
#include <list>

#include "cinder/Cinder.h"
#include "cinder/Vector.h"
#include "cinder/Exception.h"
#include "cinder/Surface.h"

#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <XnHash.h>
#include <XnLog.h>

#include "CiNIBufferManager.h"

namespace mndl { namespace ni {

class UserTracker
{
	public:
		UserTracker() {}
		UserTracker( xn::Context context );

		struct UserEvent
		{
			UserEvent( unsigned aId ) : id( aId ) {}

			unsigned id;
		};

		class Listener
		{
			public:
				virtual void newUser( UserEvent event ) {}
				virtual void lostUser( UserEvent event ) {}
				virtual void calibrationStart( UserEvent event ) {}
				virtual void calibrationEnd( UserEvent event ) {}
		};

		void start();
		void stop();

		size_t getNumUsers();
		std::vector< unsigned > getUsers();
		unsigned getClosestUserId();

		ci::Vec2f getJoint2d( XnUserID userId, XnSkeletonJoint jointId, float *conf = NULL );
		ci::Vec3f getJoint3d( XnUserID userId, XnSkeletonJoint jointId, float *conf = NULL );

		ci::Matrix33f getJointOrientation( XnUserID userId, XnSkeletonJoint jointId, float *conf = NULL );

		void setSmoothing( float s );

		ci::Vec3f getUserCenter( XnUserID userId );

		void addListener( Listener *listener );

		//! Returns mask for the given \a userId. Or a mask for all users if \a userId is 0 (the default). If \a fillWithUserId is set the user mask is filled with the userId instead of white color.
		ci::ImageSourceRef getUserMask( XnUserID userId = 0, bool fillWithUserId = false );

	protected:
		struct Obj : BufferObj {
			Obj( xn::Context context );
			~Obj();

			void start();
			void stop();

			xn::Context mContext;

			xn::UserGenerator mUserGenerator;
			xn::DepthGenerator mDepthGenerator;
			int mDepthWidth;
			int mDepthHeight;
			std::list< Listener * > mListeners;

			static bool sNeedPose;
			static XnChar sCalibrationPose[20];

			BufferManager<uint8_t> mUserBuffers;
		};
		std::shared_ptr<Obj> mObj;

		friend class ImageSourceOpenNIUserMask;

		static void XN_CALLBACK_TYPE newUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie );
		static void XN_CALLBACK_TYPE lostUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie );
		static void XN_CALLBACK_TYPE calibrationStartCB( xn::SkeletonCapability &capability, XnUserID nId, void *pCookie );
		static void XN_CALLBACK_TYPE calibrationEndCB( xn::SkeletonCapability &capability, XnUserID nId, XnBool bSuccess, void *pCookie );
		static void XN_CALLBACK_TYPE userPoseDetectedCB( xn::PoseDetectionCapability &capability, const XnChar *strPose, XnUserID nId, void *pCookie );

	public:
		//@{
		//! Emulates shared_ptr-like behavior
		typedef std::shared_ptr<Obj> UserTracker::*unspecified_bool_type;
		operator unspecified_bool_type() const { return ( mObj.get() == 0 ) ? 0 : &UserTracker::mObj; }
		void reset() { mObj.reset(); }
		//@}

		//! Parent class for all exceptions
		class Exc : cinder::Exception {};

		//! Exception thrown from a failure to create an user generator
		class ExcFailedUserGeneratorInit : public Exc {};

		//! Exception thrown if skeleton capability is not supported
		class ExcNoSkeletonCapability : public Exc {};

		//! Exception thrown if pose detection is required but not supported
		class ExcNoPoseDetection : public Exc {};
};

} } // namespace mndl::ni

