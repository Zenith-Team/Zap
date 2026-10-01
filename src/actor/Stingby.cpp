#include <zap/actor/Stingby.h>
#include <zap/Zap.h>
#include <random/seadGlobalRandom.h>
#include <graphics/SkeletalAnimation.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::Stingby, Enemy)

CREATE_STATE_ID(zap::Stingby, Idle)
CREATE_STATE_ID(zap::Stingby, Notice)
CREATE_STATE_ID(zap::Stingby, Chase)
CREATE_STATE_ID(zap::Stingby, Return)

CREATE_STATE_VIRTUAL_ID_OVERRIDE(zap::Stingby, Enemy, DieOther)

// Configuration
//  Patrol:
static constexpr f32 cPatrolRange           = 4 * 16.0f; // tiles
static constexpr f32 cNoticeRange           = 5 * 16.0f; // tiles
static constexpr f32 cForgetRange           = 8 * 16.0f; // tiles
static constexpr f32 cPatrolCycleFrames     = 560.0f;    // 9.33 seconds
//  Visual:
static constexpr f32 cScaleFactor           = 0.17f;     // 3DW models are large
static constexpr f32 cAnimBlendTime         = 10.0f;
static constexpr f32 cAggroPuffCycleFrames  = 30.0f;     // 0.50 seconds
static constexpr f32 cTurnRate              = 3.0f;      // degrees/frame
//  Chase:
static constexpr f32 cChaseBaseSpeed        = 0.5f;
static constexpr f32 cChaseMaxSpeed         = 2.0f;
static constexpr f32 cChaseAccelFrames      = 156.0f;    // 2.6 seconds
static constexpr f32 cChaseInertia          = 0.04f;
//  Internal: (don't touch these)
static constexpr f32 cInvScaleFactor        = 1.0f / cScaleFactor;
static constexpr f32 cPatrolSpeed           = sead::Mathf::pi2() / cPatrolCycleFrames;
static constexpr f32 cChaseAccel            = (cChaseMaxSpeed - cChaseBaseSpeed) / cChaseAccelFrames;

// Register it
Profile* zap::Stingby::sProfile = zap::getRegistrar()->newProfile<zap::Stingby>("stingby")
    .resources<"hacchin000">(ProfileInfo::cResType_Course)
    .build();

// Hitbox data
using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData zap::Stingby::cCollisionData = {
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

// Main code
zap::Stingby::Stingby(const ActorCreateParam& param)
    : Enemy(param)
    , mModel(nullptr)
    , mJointBoneIdx(-1)
    , mYoshiEatData(mActorUniqueID)
    , mChibiYoshiEatData(mActorUniqueID)
    , mSpawnpoint(mPos)
    , mChaseSpeed(cChaseBaseSpeed)
    , mChaseVelX(0.0f)
    , mPatrolPhase(0.0f)
    , mPatrolSpeed(cPatrolSpeed)
    , mIdlePauseTimer(0)
    , mAggroPuffTimer(0)
{ }

ActorBase::Result zap::Stingby::create() {
    mDirection = DirType::cDirType_Right; // TODO: Configurable or check player
    
    // Load our model
    mModel = JointBlendModel::create("hacchin000", "hacchin000", 4);
    mJointBoneIdx = mModel->getModel()->searchBoneIndex("JointRoot");
    mScale = sead::Vector3f(cScaleFactor, cScaleFactor, cScaleFactor);
    
    // Hitbox
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();
    
    // Terrain collision
    static const ActorBgCollisionCheck::Sensor foot = { -4.25f, 4.25f,  0.0f };
    static const ActorBgCollisionCheck::Sensor head = {  0.0f, 0.0f, 8.5f };
    static const ActorBgCollisionCheck::Sensor wall = {  5.0f, 8.0f,  6.0f };
    mBgCheckObj.set(this, &foot, &head, &wall);
    
    // Yoshi eat ability
    mEatDataPtr = &mYoshiEatData;
    mYoshiEatData.setEatType(EatData::cEatType_Drink);
    
    // Baby Yoshi eat ability
    mChibiYoshiEatDataPtr = &mChibiYoshiEatData;
    mChibiYoshiEatData.setEatType(ChibiYoshiEatData::cEatType_Drink);

    // Seed the phase randomly
    mPatrolPhase = sead::GlobalRandom::instance()->getF32() * sead::Mathf::pi2();

    // Let's go!
    changeState(Stingby::StateID_Idle);

    calcMdl_Normal();
    return cResult_Success;
}

bool zap::Stingby::execute() {
    // Delete when offscreen
    screenOutCheck(cScreenOutFlag_SkipNone);
    
    executeState();
    
    // We don't want to do these things while dead or frozen etc
    if (isState(StateID_Idle) || isState(StateID_Notice) || isState(StateID_Chase) || isState(StateID_Return)) {
        // Turn towards the target direction
        mAngle.y().chaseRest(cBaseAngleY[mDirection], sead::Mathf::deg2idx(cTurnRate));

        // Aggro puff
        if (mAggroPuffTimer > 0) {
            const f32 t = static_cast<f32>(mAggroPuffTimer) / cAggroPuffCycleFrames;
            const f32 puff = sead::Mathf::sin(t * sead::Mathf::pi()) * 0.04f; // peaks at +0.04
            mScale = sead::Vector3f(cScaleFactor + puff, cScaleFactor + puff, cScaleFactor + puff);
            mAggroPuffTimer--;
            
            // Baseline reset on the final frame
            if (mAggroPuffTimer == 0) {
                mScale = sead::Vector3f(cScaleFactor, cScaleFactor, cScaleFactor);
            }
        }
        
    }

    calcMdl_Normal();
    
    // Accurately track the model with the hitbox via bone
    sead::Matrix34f mtx;
    mModel->getModel()->getBoneWorldMatrix(mJointBoneIdx, &mtx);

    const sead::Vector3f hitboxPos = mtx.getTranslation() - mPos;
    mCollisionCheck.setCenterOffsetX(hitboxPos.x);
    mCollisionCheck.setCenterOffsetY(hitboxPos.y);
    
    // TODO: Play a buzzing sound here
    
    // TODO: Emit some honey particles or something, maybe
    
    return true;
}

bool zap::Stingby::createIceActor() {
    IceInfo info = { 
        IceInfo::makeParam(cIceType_Square),
        { mPos.x, mPos.y - 14.0f, mPos.z },
        mScale * cInvScaleFactor * 1.5f,
        nullptr
    };
    return mIceMgr.createIce(info);
}

void zap::Stingby::setIceAnm() {
    // In the notice frame, sometimes the anim can move the model too far, resulting in clipping outside the ice
    if (isState(StateID_Notice)) {
        mModel->setAnm("fly_idle", 0.0f);
        
        // Random frame, since we're manually setting the anim we don't want all to look the same
        const f32 maxFrame = mModel->getCurSklAnim()->getFrameCtrl().getFrameMax();
        mModel->getCurSklAnim()->getFrameCtrl().setFrame(
            sead::GlobalRandom::instance()->getF32() * maxFrame
        );
    }
}

void zap::Stingby::calcMdl_Base() {
    mModel->update(mPos, mAngle, mScale, !isState(StateID_Ice)); // don't animate if we're frozen
}

bool zap::Stingby::draw() {
    mModel->draw();
    return true;
}

void zap::Stingby::vsPlayerHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
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

void zap::Stingby::vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
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

/** STATE: Idle */

void zap::Stingby::initializeState_Idle() {    
    // Re-derive the correct patrol phase
    f32 normalizedX = (mPos.x - mSpawnpoint.x) / cPatrolRange;
    normalizedX = sead::Mathf::clamp2(normalizedX, -1.0f, 1.0f);
    mPatrolPhase = sead::Mathf::asin(normalizedX);
    if (mChaseVelX < 0.0f) {
        // Mirror if we're moving left
        mPatrolPhase = sead::Mathf::pi() - mPatrolPhase;
    }

    // Avoid 1-frame desync (execute updates this)
    mPos.x = mSpawnpoint.x + sead::Mathf::sin(mPatrolPhase) * cPatrolRange;

    mIdlePauseTimer = 0;
    mPatrolSpeed = cPatrolSpeed;
    
    mModel->setAnm("fly_idle", cAnimBlendTime, FrameCtrl::cMode_Repeat);
}

void zap::Stingby::executeState_Idle() {
    sead::Vector2f player;
    if (searchNearPlayer(player) == -1)
        return; // No player found

    // Notice trigger
    if (sead::Mathf::abs(player.x) < cNoticeRange) {
        changeState(StateID_Notice);
    }
    
    // Idle pause (0.2% chance per frame)
    // Smoothly lerp patrol speed for inertia
    constexpr f32 lerpFactor = 0.18f;
    if (mIdlePauseTimer > 0) {
        mIdlePauseTimer--;
        mPatrolSpeed += (0.0f - mPatrolSpeed) * lerpFactor;
    } else {
        if (sead::GlobalRandom::instance()->getU32() % 500 == 0) {
            mIdlePauseTimer = 60;
        }
        mPatrolSpeed += (cPatrolSpeed - mPatrolSpeed) * lerpFactor;
    }

    mPatrolPhase += mPatrolSpeed;
    if (mPatrolPhase > sead::Mathf::pi2()) {
        mPatrolPhase -= sead::Mathf::pi2();
    }
    
    // Check for terrain collision and turn
    bgCheck_();
    if (mBgCheckObj.checkWall(mDirection))
        mPatrolPhase = sead::Mathf::pi() - mPatrolPhase;
    
    mPos.x = mSpawnpoint.x + sead::Mathf::sin(mPatrolPhase) * cPatrolRange;

    // Derive direction from the cosine (derivative of sine) so the model flips smoothly as it changes direction
    mDirection = sead::Mathf::cos(mPatrolPhase) >= 0 ? cDirType_Right : cDirType_Left;
}

void zap::Stingby::finalizeState_Idle() { }

/** STATE: Notice */

void zap::Stingby::initializeState_Notice() {
    mModel->setAnm("notice", cAnimBlendTime / 3, FrameCtrl::cMode_NoRepeat);
    
    // TODO: Play a notice sound effect here
}

void zap::Stingby::executeState_Notice() {
    // Wait for the little jump to finish before chasing
    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        changeState(StateID_Chase);
    }
}

void zap::Stingby::finalizeState_Notice() { }

/** STATE: Chase */

void zap::Stingby::initializeState_Chase() {
    // Reset
    mChaseSpeed = cChaseBaseSpeed;
    mChaseVelX = 0.0f;

    // Trigger aggro puff
    mAggroPuffTimer = cAggroPuffCycleFrames;

    mModel->setAnm("fly_dash", cAnimBlendTime, FrameCtrl::cMode_Repeat);
}

void zap::Stingby::executeState_Chase() {
    // Check for terrain collision
    bgCheck_();
    
    // give up and turn if we hit a wall
    const bool hitWall = mBgCheckObj.checkWall(mDirection);
    
    sead::Vector2f player;
    if (searchNearPlayer(player) == -1 || sead::Mathf::abs(player.x) > cForgetRange || hitWall) {
        if (hitWall) {
            mDirection = InvDirX(mDirection);
            mChaseSpeed = 0.0f;
            mChaseVelX = 0.0f;
        }
        
        // Are we inside the patrol bound?
        const f32 leftBound = mSpawnpoint.x - cPatrolRange;
        const f32 rightBound = mSpawnpoint.x + cPatrolRange;
        if (mPos.x >= leftBound && mPos.x <= rightBound) {
            // Yes, chill
            changeState(StateID_Idle);
        } else {
            // No, fly back there
            changeState(StateID_Return);
        }
        
        return;
    }
    
    // Accelerate
    mChaseSpeed = sead::Mathf::min(mChaseSpeed + cChaseAccel, cChaseMaxSpeed);

    // x-inertia
    f32 targetVelX = player.x > 0 ? mChaseSpeed : -mChaseSpeed;
    mChaseVelX += (targetVelX - mChaseVelX) * cChaseInertia;
    mPos.x += mChaseVelX;
    
    mDirection = mChaseVelX > 0 ? cDirType_Right : cDirType_Left;
}

void zap::Stingby::finalizeState_Chase() { }

/** STATE: Return */

void zap::Stingby::initializeState_Return() {
    mModel->setAnm("fly_idle", cAnimBlendTime, FrameCtrl::cMode_Repeat);
}

void zap::Stingby::executeState_Return() {
    // Check for terrain collision, if there's something in the way here then just keep trying idk
    bgCheck_();
    
    const f32 leftBound = mSpawnpoint.x - cPatrolRange;
    const f32 rightBound = mSpawnpoint.x + cPatrolRange;

    // If we reached the patrol zone, go back to Idle
    if (mPos.x >= leftBound && mPos.x <= rightBound) {
        changeState(StateID_Idle);
        return;
    }

    // Determine which boundary is closest to fly towards
    const f32 targetX = (mPos.x < leftBound) ? leftBound : rightBound;

    constexpr f32 returnSpeed = 1.0f;
    const f32 targetVelX = (mPos.x < targetX) ? returnSpeed : -returnSpeed;

    // x-inertia
    mChaseVelX += (targetVelX - mChaseVelX) * cChaseInertia;
    mPos.x += mChaseVelX;
    
    mDirection = mChaseVelX > 0 ? cDirType_Right : cDirType_Left;
}

void zap::Stingby::finalizeState_Return() { }

/** STATE: DieOther */

void zap::Stingby::initializeState_DieOther() {
    mModel->setAnm("die_squish", 0.0f, FrameCtrl::cMode_NoRepeat);
    removeCollisionCheck();
}

void zap::Stingby::executeState_DieOther() {
    // Wait for squish to finish before deleting
    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        deleteRequest();
    }
}

void zap::Stingby::finalizeState_DieOther() { }
