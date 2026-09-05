#include <zap/actor/JumboRay.h>
#include <audio/GameAudio.h>
#include <collision/ActorBgCollisionMgr.h>
#include <map_obj/PlayerRideUtil.h>
#include <red/util/SpriteUtil.h>
#include <zap/Zap.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::JumboRay, Actor)

const ActorCreateInfo zap::JumboRay::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_None
};

Profile* zap::JumboRay::sProfile = zap::getRegistrar()->newProfile<zap::JumboRay>("jumboray")
    .resources<"manjirou">(ProfileInfo::cResType_Course)
    .createInfo(&cCreateInfo)
    .build();

zap::JumboRay::JumboRay(const ActorCreateParam& param)
    : Actor(param)
    , mModel(nullptr)
    , mTargetAltitude(0.0f)
    , mBaseY(0.0f)
    , mTime(0.0f)
    , mVerticalDirection(cDirType_Up)
    , mArcDuration(0)
    , mPlayerRideFlags(0)
{ }

// TODO: spawn an item and drag behind it (items, coin rings)
// position 0-5 where 0 is on back and 1-5 is a tail bone tail_N (+a Y offset)

ActorBase::Result zap::JumboRay::create() {
    // Model
    mModel = AnimModel::create("manjirou", "manjirou", 2, 0, 3);
    mModel->playSklAnim("wait", 0);
    mModel->playTexSrtAnim("glass", 0);
    mModel->playTexSrtAnim("wait", 1);
    
    // Save this for later
    mBaseY = mPos.y;
    
    // Setting: Horizontal Direction
    mDirection = static_cast<DirType>(red::SpriteUtil::getNybble7(this));
    mAngle.y() = sead::Mathf::deg2idx(mDirection == cDirType_Right ? 67.5f : -67.5f);
    
    // Setting: Vertical Direction
    mVerticalDirection = static_cast<DirType>(red::SpriteUtil::getNybble8(this));
    if (mVerticalDirection == 0) {
        mVerticalDirection = cDirType_Up;
    } else if (mVerticalDirection == 1) {
        mVerticalDirection = cDirType_Down;
    } else {
        tk::fatal("Invalid direction type for manta.");
    }
    
    // Setting: Altitude
    mTargetAltitude = (red::SpriteUtil::getNybble5(this) << 4) | red::SpriteUtil::getNybble6(this);
    mTargetAltitude *= 16.0f;
    
    // Setting: Arc Duration
    mArcDuration = (red::SpriteUtil::getNybble9(this) << 4) | red::SpriteUtil::getNybble10(this);
    if (mArcDuration == 0) {
        tk::fatal("Arc Duration was unset.");
    }
    
    updateModel();
    
    // Ride Collider
    static constexpr sead::Vector2f points[3] = {
        { -74.0f, 18.0f },
        {   0.0f, 24.0f },
        {  74.0f, 18.0f }
    };
    mCollider.set(this, {
        .pos_offset = { 0.0f, 0.0f },
        .rot_pivot_offset = { 0.0f, 0.0f },
        .points = points,
        .angle = mDirection == cDirType_Left ? mAngle.x() : -mAngle.x()
    });

    ActorBgCollisionMgr::instance()->entry(mCollider);
    
    return cResult_Success;
}

bool zap::JumboRay::execute() {
    // die if arc finished
    if (sead::Mathf::abs(waveDerivative(mTime)) < 0.01f) {
        // we are at a maxima
        
        if (mVerticalDirection == cDirType_Up && waveFunction(mTime) < 0.0f) {
            // crest
            deleteRequest();
        } else if (mVerticalDirection == cDirType_Down && waveFunction(mTime) > 0.0f) {
            // trough
            deleteRequest();
        }
    }
    
    mPos.x += mDirection == cDirType_Right ? 1.0 : -1.0f;
    
    mTime += 1.0f / (mArcDuration * 60.0f);
    mPos.y = mBaseY + waveFunction(mTime) * mTargetAltitude;
    
    // who is riding us now?
    u32 rideFlags = PlayerRideUtil::getRideFlag(mCollider);
    
    // it's someone new
    if (rideFlags > mPlayerRideFlags) {
        // whistle and dance
        GameAudio::getAudioObjEmy()->startSound("SE_EMY_MANTA_SING", mPos); //! <-- doesn't work
        GameAudio::getAudioObjEmy()->startSound("SE_SYS_CONTINUE_DONE", mPos); //! <-- works
        
        // play anim
        mModel->playSklAnim("ridden", 1);
        mModel->getSklAnim(1)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
        mModel->playTexSrtAnim("ridden", 1);
        mModel->getShuAnim(1)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
    }
    
    // save the current state to compare next frame
    mPlayerRideFlags = rideFlags;
    
    updateModel();
    
    mCollider.execute();
    mCollider.setAngle(mDirection == cDirType_Left ? mAngle.x() : -mAngle.x());
    
    return true;
}

bool zap::JumboRay::draw() {
    mModel->draw();
    return true;
}

void zap::JumboRay::updateModel() {
    mAngle.x() = sead::Mathf::deg2idx(waveDerivative(mTime) * -20.0f);
    
    mModel->update(mPos + sead::Vector3f((mDirection == cDirType_Right) ? 8.0f : -8.0f, 6.0f, 0.0f), mAngle, mScale);
}

f32 zap::JumboRay::waveFunction(const f32 x) const {
    if (mVerticalDirection == cDirType_Up) {
        return sead::Mathf::sin(x);
    } else {
        return -sead::Mathf::sin(x); 
    }
}

f32 zap::JumboRay::waveDerivative(const f32 x) const {
    if (mVerticalDirection == cDirType_Up) {
        return sead::Mathf::cos(x);
    } else {
        return -sead::Mathf::cos(x); 
    }
}
