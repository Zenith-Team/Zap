#include <zap/actor/FlyBones.h>
#include <zap/Zap.h>
#include <actor/ActorMgr.h>
#include <red/util/SpriteUtil.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::FlyBones, Enemy)

CREATE_STATE_ID(zap::FlyBones, Waiting)
CREATE_STATE_ID(zap::FlyBones, Flying)

// Spawning data
const ActorCreateInfo zap::FlyBones::cCreateInfo = {
    .offset_x = 8, .offset_y = -16, // how far away from the actual sprite (the blue square) the actor will spawn, useful for centering the actor
    .spawn_range = {
        .offset_x = 0, .offset_y = 16,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_MapObj
};

// Register it
Profile* zap::FlyBones::sProfile = zap::getRegistrar()->newProfile<zap::FlyBones>("flybones")
    .resources<"karon", "wing", "nokonokoB">(ProfileInfo::cResType_Course)
    .createInfo(&zap::FlyBones::cCreateInfo)
    .build();

// Hitbox data
using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData zap::FlyBones::cCollisionData = {
    .center_offset = { 0.0f, 8.0f },
    .half_size = { 8.0f, 12.0f },
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

zap::FlyBones::FlyBones(const ActorCreateParam& param)
    : Enemy(param)
    , mBodyModel(nullptr)
    , mWingsModel(nullptr)
    , mYoshiEatData(mActorUniqueID)
    , mChibiYoshiAwaData(this)
    , mFlyDistance(0)
    , mRising(false)
    , mWaitTimer(0)
    , mTimerTarget(0)
    , mStartY(param.position.y)
{ }

ActorBase::Result zap::FlyBones::create() {
    mAngle.y() = cBaseAngleY[cDirType_Left]; // TODO: Configurable or check player
    
    // Load models
    mBodyModel = AnimModel::create("nokonokoB", "nokonokoB", 1);
    mBodyModel->playSklAnim("flyA");
    mWingsModel = AnimModel::create("wing", "wing", 1);
    mWingsModel->playSklAnim("wing_kuri");
    
    // Hitbox
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();
    
    // Yoshi eat inability
    mEatDataPtr = &mYoshiEatData;
    mYoshiEatData.setEatType(EatData::cEatType_None);
    
    // Baby Yoshi bubble ability
    mChibiYoshiAwaDataPtr = &mChibiYoshiAwaData;
    mChibiYoshiAwaData.setAwaType(ChibiYoshiAwaData::cAwaType_Catch);
    
    // Setting: Fly Distance
    mFlyDistance = red::SpriteUtil::getNybble5(this) * 16;
    
    // Setting: Start Rising
    mRising = red::SpriteUtil::getNybble6(this);
    
    // Setting: Time between Flight
    mTimerTarget = red::SpriteUtil::getNybble7(this) * 60;
    
    changeState(StateID_Waiting);
    
    calcMdl_Base();
    
    return cResult_Success;
}

bool zap::FlyBones::execute() {
    // Delete when offscreen
    screenOutCheck(0);
    
    executeState();
    
    // Search for a player nearby
    if (sead::Vector2f distanceToPlayer; searchNearPlayer(distanceToPlayer) != -1) {
        // Found a player, face him
        mDirection = distanceToPlayer.x > 0 ? cDirType_Right : cDirType_Left;
    }
    mAngle.y().chaseRest(cBaseAngleY[mDirection], sead::Mathf::deg2idx(3.0f));
    
    calcMdl_Base();
    
    return true;
}

bool zap::FlyBones::draw() {
    mBodyModel->draw();
    mWingsModel->draw();
    return true;
}

void zap::FlyBones::calcMdl_Base() {
    mBodyModel->update(mPos, mAngle, mScale, !isState(StateID_Ice));
    mWingsModel->update(mPos + sead::Vector3f(0.0f, 8.0f, 0.0f), mAngle, mScale, !isState(StateID_Ice));
}

void zap::FlyBones::loseWings() {
    // delete self
    removeCollisionCheck();
    deleteActor(true);
    
    // spawn regular dry bones
    ActorCreateParam child;
    child.profile = Profile::get(0x288); // TODO: Enum
    child.position = mPos + sead::Vector3f(0.0f, -8.0f, 0.0f);
    ActorMgr::instance()->createImmediately(child);
}

bool zap::FlyBones::createIceActor() {
    const sead::Vector3f iceScale = {
        mScale.x * 1.1f,
        mScale.y * 0.95f,
        mScale.z * 1.3f
    };
    
    IceInfo info = {
        IceInfo::makeParam(cIceType_Tate),
        mPos + sead::Vector3f(0.0f, -8.0f, 0.0f),
        iceScale,
        nullptr
    };
    
    return mIceMgr.createIce(info);
}

// Player stomp
void zap::FlyBones::vsPlayerHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    /// IM ANTIFUMI -joe 2026
    // this just checks what type of hit just occurred
    switch (fumiCheck(cc_self, cc_other, cFumiSeType_Normal)) {
        case cFumiType_Fumi:
        case cFumiType_SpinFumi: {
            // stomp or spinjump
            loseWings();
            return;
        }
        
        case cFumiType_MameFumi: {
            // mini stomp, do nothing
            return;
        }
        
        default: {
            return Enemy::vsPlayerHitCheck_Normal(cc_self, cc_other);
        }
    }
}

// Same as above, but for Yoshi
void zap::FlyBones::vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    switch (fumiCheck(cc_self, cc_other, cFumiSeType_Normal)) {
        case cFumiType_Fumi: {
            loseWings();
            return;
        }
        
        default: {
            return Enemy::vsYoshiHitCheck_Normal(cc_self, cc_other);
        }
    }
}

bool zap::FlyBones::hitCallback_Fire(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    return false; // immune
}

/** STATE: Waiting */

void zap::FlyBones::initializeState_Waiting() {
    mWaitTimer = 0;
}

void zap::FlyBones::executeState_Waiting() {
    if (++mWaitTimer >= mTimerTarget) {
        changeState(StateID_Flying);
    }
}

void zap::FlyBones::finalizeState_Waiting() { }

/** STATE: Flying */

void zap::FlyBones::initializeState_Flying() {
    
}

void zap::FlyBones::executeState_Flying() {
    if (mRising) {
        mPos.y++;
        
        if (mPos.y - mStartY >= mFlyDistance) {
            changeState(StateID_Waiting);
        }
    } else {
        mPos.y--;
        
        if (mStartY - mPos.y >= mFlyDistance) {
            changeState(StateID_Waiting);
        }
    }
}

void zap::FlyBones::finalizeState_Flying() {
    mRising = !mRising;
}
