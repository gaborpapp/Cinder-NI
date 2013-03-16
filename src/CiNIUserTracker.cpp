#include "cinder/app/App.h"

#include "CiNi.h"
#include "CiNIUserTracker.h"

using namespace ci;
using namespace std;

namespace mndl { namespace ni {

void XN_CALLBACK_TYPE UserTracker::newUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie )
{
	app::console() << "new user " << nId << endl;
	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);

	if (Obj::sNeedPose)
	{
		obj->mUserGenerator.GetPoseDetectionCap().StartPoseDetection(Obj::sCalibrationPose, nId);
	}
	else
	{
		obj->mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}

	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->newUser( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::lostUserCB( xn::UserGenerator &generator, XnUserID nId, void *pCookie )
{
	app::console() << "lost user " << nId << endl;
	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);
	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->lostUser( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::calibrationStartCB( xn::SkeletonCapability &capability, XnUserID nId, void *pCookie )
{
	app::console() << "calibration start " << nId << endl;

	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);
	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->calibrationStart( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::calibrationEndCB( xn::SkeletonCapability &capability, XnUserID nId, XnBool bSuccess, void *pCookie )
{
	app::console() << "calibration end " << nId << " status " << bSuccess << endl;
	UserTracker::Obj *obj = static_cast<UserTracker::Obj *>(pCookie);

	if (bSuccess)
	{
		// Calibration succeeded
		app::console() << "calibration complete for user " << nId << endl;
		obj->mUserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		app::console() << "calibration failed for user " << nId << endl;
		if (Obj::sNeedPose)
		{
			obj->mUserGenerator.GetPoseDetectionCap().StartPoseDetection(Obj::sCalibrationPose, nId);
		}
		else
		{
			obj->mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}

	for ( list< Listener *>::const_iterator i = obj->mListeners.begin();
			i != obj->mListeners.end(); ++i )
	{
		(*i)->calibrationEnd( UserEvent( nId ) );
	}
}

void XN_CALLBACK_TYPE UserTracker::userPoseDetectedCB( xn::PoseDetectionCapability &capability, const XnChar *strPose, XnUserID nId, void *pCookie )
{
	app::console() << "pose " << strPose << " detected for user " << nId << endl;

	/*
	mUserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	*/
}


UserTracker::UserTracker( xn::Context context )
	: mObj( new Obj( context ) )
{
}


bool UserTracker::Obj::sNeedPose = false;
XnChar UserTracker::Obj::sCalibrationPose[20] = "";

UserTracker::Obj::Obj( xn::Context context )
	: mContext( context )
{
	XnStatus rc;

	rc = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH, mDepthGenerator);
	checkRc( rc, "Retrieving depth generator" );

	xn::DepthMetaData depthMD;
	mDepthGenerator.GetMetaData( depthMD );
	mDepthWidth = depthMD.FullXRes();
	mDepthHeight = depthMD.FullYRes();
	mUserBuffers = BufferManager<uint8_t>( mDepthWidth * mDepthHeight, this );

	rc = mContext.FindExistingNode(XN_NODE_TYPE_USER, mUserGenerator);
	if (rc != XN_STATUS_OK)
	{
		rc = mUserGenerator.Create(mContext);
		if (rc != XN_STATUS_OK)
			throw ExcFailedUserGeneratorInit();
	}

	if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
		throw ExcNoSkeletonCapability();

	// user callbacks
	XnCallbackHandle userCallbacks;
	rc = mUserGenerator.RegisterUserCallbacks(newUserCB, lostUserCB,
			this, userCallbacks);
	checkRc( rc, "Register user callbacks" );

	// calibration callbacks
	XnCallbackHandle calibrationCallbacks;
	rc = mUserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(
		calibrationStartCB, calibrationEndCB,
		this, calibrationCallbacks);
	checkRc( rc, "Register calibration callbacks" );

	if (mUserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		sNeedPose = true;
		if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
			throw ExcNoPoseDetection();

		XnCallbackHandle poseDetected;
		rc = mUserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(
				userPoseDetectedCB, this, poseDetected);
		checkRc( rc, "Register pose detected callback" );
		mUserGenerator.GetSkeletonCap().GetCalibrationPose(sCalibrationPose);
	}

	mUserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
}

void UserTracker::start()
{
	XnStatus rc;
	rc = mObj->mUserGenerator.StartGenerating();
	checkRc( rc, "UserGenerator.StartGenerating" );
}

void UserTracker::stop()
{
	XnStatus rc;
	rc = mObj->mUserGenerator.StopGenerating();
	checkRc( rc, "UserGenerator.StopGenerating" );
}

void UserTracker::addListener( Listener *listener )
{
	mObj->mListeners.push_back( listener );
}

size_t UserTracker::getNumUsers()
{
	XnUserID aUsers[20];
	XnUInt16 nUsers = 20;
	mObj->mUserGenerator.GetUsers( aUsers, nUsers );

	return nUsers;
}

vector< unsigned > UserTracker::getUsers()
{
	XnUserID aUsers[20];
	XnUInt16 nUsers = 20;
	mObj->mUserGenerator.GetUsers( aUsers, nUsers );
	vector< unsigned > users;
	for ( unsigned i = 0; i < nUsers; i++)
		users.push_back( aUsers[i] );

	return users;
}

unsigned UserTracker::getClosestUserId()
{
	XnUserID aUsers[20];
	XnUInt16 nUsers = 20;
	mObj->mUserGenerator.GetUsers( aUsers, nUsers );
	float minZ = 99999;
	unsigned closestId = 0;
	for ( unsigned i = 0; i < nUsers; i++)
	{
		XnPoint3D center;
		mObj->mUserGenerator.GetCoM( aUsers[i], center );
		if (center.Z < minZ)
		{
			closestId = aUsers[i];
			minZ = center.Z;
		}
	}
	return closestId;
}

Vec2f UserTracker::getJoint2d( XnUserID userId, XnSkeletonJoint jointId, float *conf /* = NULL */ )
{
	if (mObj->mUserGenerator.GetSkeletonCap().IsTracking( userId ))
	{
		XnSkeletonJointPosition joint;
		mObj->mUserGenerator.GetSkeletonCap().GetSkeletonJointPosition( userId, jointId, joint );

		if ( conf != NULL )
			*conf = joint.fConfidence;

		XnPoint3D pt[1];
		pt[0] = joint.position;

		mObj->mDepthGenerator.ConvertRealWorldToProjective( 1, pt, pt );

		return Vec2f( pt[0].X, pt[0].Y );
	}
	else
	{
		if ( conf != NULL )
			*conf = 0;

		return Vec2f();
	}
}

Vec3f UserTracker::getJoint3d( XnUserID userId, XnSkeletonJoint jointId, float *conf /* = NULL */ )
{
	if (mObj->mUserGenerator.GetSkeletonCap().IsTracking( userId ))
	{
		XnSkeletonJointPosition joint;
		mObj->mUserGenerator.GetSkeletonCap().GetSkeletonJointPosition( userId, jointId, joint );

		if ( conf != NULL )
			*conf = joint.fConfidence;

		return Vec3f( joint.position.X, joint.position.Y, joint.position.Z );
	}
	else
	{
		if ( conf != NULL )
			*conf = 0;

		return Vec3f();
	}
}

Matrix33f UserTracker::getJointOrientation( XnUserID userId, XnSkeletonJoint jointId, float *conf /* = NULL */ )
{
	if (mObj->mUserGenerator.GetSkeletonCap().IsTracking( userId ))
	{
		XnSkeletonJointOrientation jointOri;
		mObj->mUserGenerator.GetSkeletonCap().GetSkeletonJointOrientation( userId, jointId, jointOri );

		if ( conf != NULL )
			*conf = jointOri.fConfidence;

		float *oriM = jointOri.orientation.elements;
		return Matrix33f( oriM[ 0 ], oriM[ 3 ], oriM[ 6 ],
						  oriM[ 1 ], oriM[ 4 ], oriM[ 7 ],
						  oriM[ 2 ], oriM[ 5 ], oriM[ 8 ] );
	}
	else
	{
		if ( conf != NULL )
			*conf = 0;

		return Matrix33f();
	}
}

void UserTracker::setSmoothing( float s )
{
	mObj->mUserGenerator.GetSkeletonCap().SetSmoothing( s );
}

Vec3f UserTracker::getUserCenter( XnUserID userId )
{
	XnPoint3D center;
	mObj->mUserGenerator.GetCoM( userId, center );
	return Vec3f( center.X, center.Y, center.Z );
}

class ImageSourceOpenNIUserMask : public ImageSource {
	public:
		ImageSourceOpenNIUserMask( uint8_t *buffer, int w, int h, shared_ptr<UserTracker::Obj> ownerObj )
			: ImageSource(), mData( buffer ), mOwnerObj( ownerObj )
		{
			setSize( w, h );
			setColorModel( ImageIo::CM_GRAY );
			setChannelOrder( ImageIo::Y );
			setDataType( ImageIo::UINT8 );
		}

		~ImageSourceOpenNIUserMask()
		{
			// let the owner know we are done with the buffer
			mOwnerObj->mUserBuffers.derefBuffer( mData );
		}

		virtual void load( ImageTargetRef target )
		{
			ImageSource::RowFunc func = setupRowFunc( target );

			for( int32_t row = 0; row < mHeight; ++row )
				((*this).*func)( target, row, mData + row * mWidth );
		}

	protected:
		shared_ptr<UserTracker::Obj>	mOwnerObj;
		uint8_t							*mData;
};

ImageSourceRef UserTracker::getUserMask( XnUserID userId )
{
	mObj->mUserBuffers.derefActiveBuffer(); // finished with current active buffer
	uint8_t *destPixels = mObj->mUserBuffers.getNewBuffer();  // request a new buffer

	xn::SceneMetaData sceneMD;
    XnStatus rc = mObj->mUserGenerator.GetUserPixels( userId, sceneMD);
	checkRc( rc, "UserGenerator GetUserPixels" );
	if ( rc == XN_STATUS_OK )
	{
		const uint16_t *userPixels = reinterpret_cast< const uint16_t * >( sceneMD.Data() );

		if ( userId == 0 )
		{
			// mask for all users
			for ( size_t i = 0 ; i < mObj->mDepthWidth * mObj->mDepthHeight; i++ )
			{
				if ( userPixels[i] != 0 )
					destPixels[i] = 255;
				else
					destPixels[i] = 0;
			}
		}
		else
		{
			// mask for specified user id
			for ( size_t i = 0 ; i < mObj->mDepthWidth * mObj->mDepthHeight; i++ )
			{
				if ( userPixels[i] == userId )
					destPixels[i] = 255;
				else
					destPixels[i] = 0;
			}
		}
	}

	// set this new buffer to be the current active buffer
	mObj->mUserBuffers.setActiveBuffer( destPixels );

	uint8_t *activeMask = mObj->mUserBuffers.refActiveBuffer();
	return ImageSourceRef( new ImageSourceOpenNIUserMask( activeMask, mObj->mDepthWidth, mObj->mDepthHeight, mObj ) );
}

} } // namespace mndl::ni

