#include <zap/actor/Flaptor.h>
#include <red/util/SpriteUtil.h>
#include <zap/Zap.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::Flaptor, Enemy)

CREATE_STATE_ID(zap::Flaptor, Patrol)
CREATE_STATE_ID(zap::Flaptor, Dive)
CREATE_STATE_ID(zap::Flaptor, Land)

CREATE_STATE_VIRTUAL_ID_OVERRIDE(zap::Flaptor, Enemy, DieOther)

static constexpr f32 cScaleFactor = 0.175f; // 3DW models are large
static constexpr f32 cInvScaleFactor = 1.0f / cScaleFactor;
static constexpr f32 cTurnRate = 4.0f; // degrees per frame
static constexpr f32 cAnimBlendTime = 10.0f;

// Spawning data
const ActorCreateInfo zap::Flaptor::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .spawn_range = {
        .offset_x = 0, .offset_y = 16,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_MapObj
};

// Hitbox data
using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData zap::Flaptor::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 8.5f, 8.5f },
    .shape_type = CC::cShapeType_Box,
    .kind = CC::cKind_Enemy,
    .attack = CC::cAttack_None,
    .vs_kind = CC::TargetKind( // Copied from Kuribo
        CC::cTargetKind_Player |
        CC::cTargetKind_Enemy |
        CC::cTargetKind_Item |
        CC::cTargetKind_Tama |
        CC::cTargetKind_ChibiYoshi |
        CC::cTargetKind_Unk10 |
        CC::cTargetKind_DrcTouch
    ),
    .vs_damage = CC::cDamageFrom_All,
    .status = CC::cStatus_None,
    .callback = &Enemy::normal_collcheck
};

Profile* zap::Flaptor::sProfile = zap::getRegistrar()->newProfile<zap::Flaptor>("flaptor")
    .resources<"mokinger">(ProfileInfo::cResType_Course)
    .createInfo(&cCreateInfo)
    .build();

zap::Flaptor::Flaptor(const ActorCreateParam& param)
    : Enemy(param)
    , mModel(nullptr)
    , mMoveType(-1)
    , mPatrolRange(0.0f)
    , mReturnSpeed(0.0f)
    , mJointBoneIdx(-1)
    , mYoshiEatData(mActorUniqueID)
    , mBabyYoshiEatData(mActorUniqueID)
{ }

ActorBase::Result zap::Flaptor::create() {
    mDirection = cDirType_Left;
    mStartPos = mPos;
    
    // Model
    mModel = JointBlendModel::create("mokinger", "mokinger", 4);
    mJointBoneIdx = mModel->getModel()->searchBoneIndex("JointRoot");
    mScale = sead::Vector3f(cScaleFactor, cScaleFactor, cScaleFactor);
    
    // Hitbox
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();
    
    // Terrain collision
    static const ActorBgCollisionCheck::Sensor foot = { -8.0f, 8.0f, -8.0f };
    mBgCheckObj.set(this, &foot, nullptr, nullptr);
    
    // Yoshi eat ability
    mEatDataPtr = &mYoshiEatData;
    mYoshiEatData.setEatType(EatData::cEatType_Drink);
    
    // Baby Yoshi eat ability
    mChibiYoshiEatDataPtr = &mBabyYoshiEatData;
    mBabyYoshiEatData.setEatType(ChibiYoshiEatData::cEatType_Drink);
    
    // Setting: Movement type
    mMoveType = red::SpriteUtil::getNybble5(this);
    
    // Setting: Patrol range
    mPatrolRange = red::SpriteUtil::getNybble6(this) * 16.0f;
    
    // Setting: Return speed
    mReturnSpeed = red::SpriteUtil::getNybble7(this) * 0.5f + 0.5f;
    
    // Let's go!
    changeState(StateID_Patrol);
    
    calcMdl_Normal();
    
    return cResult_Success;
}

bool zap::Flaptor::execute() {
    // Delete when offscreen
    screenOutCheck(0);
    
    executeState();
    
    calcMdl_Normal();
    
    // Accurately track the model with the hitbox via bone
    sead::Matrix34f mtx;
    mModel->getModel()->getBoneWorldMatrix(mJointBoneIdx, &mtx);

    const sead::Vector3f hitboxPos = mtx.getTranslation() - mPos;
    mCollisionCheck.setCenterOffsetX(hitboxPos.x);
    mCollisionCheck.setCenterOffsetY(hitboxPos.y);
    
    return true;
}

bool zap::Flaptor::draw() {
    mModel->draw();
    return true;
}

bool zap::Flaptor::createIceActor() {
    IceInfo info = { 
        IceInfo::makeParam(cIceType_Square),
        { mPos.x, mPos.y - 14.0f, mPos.z },
        mScale * cInvScaleFactor * 1.5f,
        nullptr
    };
    return mIceMgr.createIce(info);
}

void zap::Flaptor::calcMdl_Base() {
    mModel->update({ mPos.x, mPos.y - 8.0f, mPos.z }, mAngle, mScale, !isState(StateID_Ice));
}

void zap::Flaptor::vsPlayerHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    Actor* other = cc_other->getOwner();
    
    switch (fumiCheck(cc_self, cc_other, cFumiSeType_Normal)) {
        case cFumiType_Fumi: {
            return setDeathInfo_FumiOther(other, mSpeed); // changes state to DieOther
        }
        
        case cFumiType_MameFumi: {
            return;
        }
        
        case cFumiType_SpinFumi: {
            return setDeathInfo_SpinFumi(other); // changes state to DieFall
        }
        
        default: {
            return Enemy::vsPlayerHitCheck_Normal(cc_self, cc_other);
        }
    }
}

void zap::Flaptor::vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    Actor* other = cc_other->getOwner();
    
    switch (fumiCheck(cc_self, cc_other, cFumiSeType_Normal)) {
        case cFumiType_Fumi: {
            return setDeathInfo_YoshiFumi(other); // changes state to DieYoshiFumi
        }
        
        default: {
            return Enemy::vsYoshiHitCheck_Normal(cc_self, cc_other);
        }
    }
}

/** STATE: Patrol */

void zap::Flaptor::initializeState_Patrol() {
    mModel->setAnm("Fly", cAnimBlendTime);
}

void zap::Flaptor::executeState_Patrol() {
    mAngle.y().chaseRest(cBaseAngleY[mDirection], sead::Mathf::deg2idx(cTurnRate));

    switch (mMoveType) {
        case MoveType::Stationary: {
            sead::Vector2f delta = { mStartPos.x - mPos.x, mStartPos.y - mPos.y };
            const f32 dist = delta.length();

            if (dist > mReturnSpeed) {
                delta *= (mReturnSpeed / dist);
                mPos.x += delta.x;
                mPos.y += delta.y;
            } else {
                mPos = mStartPos;
            }
            break;
        }

        case MoveType::Horizontal: {
            const f32 offsetX = mPos.x - mStartPos.x;

            // Flip direction at the threshold walls
            if      (offsetX < -mPatrolRange) mDirection = cDirType_Right;
            else if (offsetX >  mPatrolRange) mDirection = cDirType_Left;

            mPos.x += (mDirection == cDirType_Right) ? 1.0f : -1.0f;

            if (!sead::Mathf::chase(&mPos.y, mStartPos.y, mReturnSpeed))
                return;

            break;
        }

        case MoveType::Vertical: {
            // Mirror of the Horizontal case with axes swapped.
            const f32 offsetY = mPos.y - mStartPos.y;

            if      (offsetY < -mPatrolRange) mDirection = cDirType_Up;
            else if (offsetY >  mPatrolRange) mDirection = cDirType_Down;

            mPos.y += (mDirection == cDirType_Up) ? 1.0f : -1.0f;

            if (!sead::Mathf::chase(&mPos.x, mStartPos.x, mReturnSpeed))
                return;

            break;
        }
    }
    
    // Check for terrain collision (don't fly into walls)
    bgCheck_();
    
    // Don't check for dives until we are (almost) back up
    if (sead::Mathf::abs(mPos.y - mStartPos.y) > 2.0f * 16.0f)
        return;

    sead::Vector2f distanceToPlayer;
    if (searchNearPlayer(distanceToPlayer) == -1)
        return;

    if (distanceToPlayer.length() <= 5.0f * 16.0f && distanceToPlayer.y < 4.0f)
        changeState(StateID_Dive);
}

void zap::Flaptor::finalizeState_Patrol() { }

/** STATE: Dive */

void zap::Flaptor::initializeState_Dive() {
    mGravity = cDefaultGravity;
    mSpeedMax.y = -4.0f;
    
    mModel->setAnm("Attack", cAnimBlendTime);
    mModel->getCurSklAnim()->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void zap::Flaptor::executeState_Dive() {
    calcSpeedY_();
    posMove_();
    bgCheck_();
    
    if (bgCheckFoot_()) {
        changeState(StateID_Land);
    }
}

void zap::Flaptor::finalizeState_Dive() { }

/** STATE: Land */

void zap::Flaptor::initializeState_Land() {
    mModel->setAnm("AttackLand", cAnimBlendTime);
    mModel->getCurSklAnim()->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void zap::Flaptor::executeState_Land() {
    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        changeState(StateID_Patrol);
    }
}

void zap::Flaptor::finalizeState_Land() {
    mModel->getCurSklAnim()->getFrameCtrl().setPlayMode(FrameCtrl::cMode_Repeat);
    mPos.y += 8.0f; // TODO: Fix this root-cause. He teleports down after completing this anim for some reason, so revert that.
}

/** STATE: DieOther */

void zap::Flaptor::initializeState_DieOther() {
    mModel->setAnm("PressDown", cAnimBlendTime);
    mModel->getCurSklAnim()->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
    
    removeCollisionCheck();
}

void zap::Flaptor::executeState_DieOther() {
    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        deleteRequest();
    }
}

void zap::Flaptor::finalizeState_DieOther() { }
