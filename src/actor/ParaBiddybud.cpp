#include <zap/actor/ParaBiddybud.h>
#include <zap/Zap.h>
#include <red/utility/SpriteUtil.h>
#include <effect/EffectCreateUtil.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::ParaBiddybud, Enemy)

CREATE_STATE_ID(zap::ParaBiddybud, Idle)

CREATE_STATE_VIRTUAL_ID_OVERRIDE(zap::ParaBiddybud, Enemy, DieOther)

// Configuration
static constexpr f32 cScaleFactor = 0.17f; // 3DW models are large
static constexpr f32 cInvScaleFactor = 1.0f / cScaleFactor;

const ActorCreateInfo zap::ParaBiddybud::cCreateInfo = {
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

using CC = ActorCollisionCheck;
const CC::CollisionData zap::ParaBiddybud::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 8.0f, 8.0f },
    .shape_type = CC::ActorCollisionCheck::cShapeType_Box,
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

Profile* zap::ParaBiddybud::sProfile = zap::getRegistrar()->newProfile<zap::ParaBiddybud>("para_biddybud")
    .resources<"tenten_w">(ProfileInfo::cResType_Course)
    .createInfo(cCreateInfo)
    .build();

zap::ParaBiddybud::ParaBiddybud(const ActorCreateParam& param)
    : Enemy(param)
    , mModel(nullptr)
    , mYoshiEatData(mActorUniqueID)
    , mChibiYoshiEatData(mActorUniqueID)
{ }

ActorBase::Result zap::ParaBiddybud::create() {
    // Movement setup
    const u8 nybble20 = red::SpriteUtil::getNybble20(this);
    if (nybble20 > cPos_KinokoLift) {
        tk::fatal("Movement type was out of bounds");
    }
    const ParentMovementType movementType = static_cast<ParentMovementType>(nybble20);
    u32 movementMask = mMovementHandler.getTypeMask(movementType);

    setupMovement(mPos, movementMask, movementType, mParamEx.course.movement_id);

    // Model setup
    mScale = sead::Vector3f(cScaleFactor, cScaleFactor, cScaleFactor);
    mModel = JointBlendModel::create("tenten_w", "tenten_w", 3, 1, 1);
    mModel->playTexAnim("bud");
    mModel->playTexSrtAnim("FlyWait");
    mModel->playSklAnim("FlyWait");
    
    // Setting: Colour
    mModel->getTexAnim(0)->getFrameCtrl().setFrame(red::SpriteUtil::getNybble5(this));
    mModel->getTexAnim(0)->getFrameCtrl().setRate(0.0f);
    
    // Setup hitbox
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();
    
    // Yoshi eat ability
    mEatDataPtr = &mYoshiEatData;
    mYoshiEatData.setEatType(EatData::cEatType_Drink);
    
    // Baby Yoshi eat ability
    mChibiYoshiEatDataPtr = &mChibiYoshiEatData;
    mChibiYoshiEatData.setEatType(ChibiYoshiEatData::cEatType_Drink);

    changeState(StateID_Idle);

    calcMdl_Base();
    
    return cResult_Success;
}

void zap::ParaBiddybud::setupMovement(const sead::Vector3f& position, u32 movement_mask, ParentMovementType movement_type, u32 movement_id) {
    // use different link function if pivotal rotation, prevents glitches
    if (movement_type == ParentMovementType::cPos_CenterRotation) {
        mMovementHandler.linkPivotal(position, movement_mask, movement_id);
    } else {
        mMovementHandler.link(position, movement_mask, movement_id);
    }

    // set type-specific data
    setMovementParamaters(movement_type);

    mMovementHandler.execute();
}

void zap::ParaBiddybud::setMovementParamaters(ParentMovementType movement_type) {
    static sead::SafeArray<f32, 16> twoWayDistanceMultiplierArr {
        1.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f
    };
    static sead::SafeArray<f32, 16> boltMovementSpeedArr {
        1.0f, 0.25f, 0.5f, 0.75f, 0.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f
    };

    switch (movement_type) {
        case cPos_Screw: {
            mMovementHandler.setBoltSpeed(boltMovementSpeedArr[red::SpriteUtil::getNybble18(this)]);
            mMovementHandler.setBoltDirection(static_cast<DirType>(red::SpriteUtil::getNybble19(this)));
            break;
        }
        case cPos_GoAndCome: {
            mMovementHandler.setTwoWayDistanceMultiplier(twoWayDistanceMultiplierArr[red::SpriteUtil::getNybble18(this)] + (0.01f * red::SpriteUtil::getNybble19(this)));
            break;
        }
        case cPos_ShiftingPlatform: {
            mMovementHandler.setRectPlatformInfo(static_cast<RectPlatformInfo>(red::SpriteUtil::getNybble19(this)));
            break;
        }
        case cPos_FloorGyration: {
            mMovementHandler.setFloorGyrationAngle(0x1000000 * red::SpriteUtil::getNybbleRange(this, 17, 18));
            ParentMovementMgr::MovementProperties newproperty = mMovementHandler.getMovementProperties();
            newproperty.hill_distance_offset = -16.0f * red::SpriteUtil::getNybble19(this);
            mMovementHandler.setMovementProperties(newproperty);
            break;
        }
    }
}

bool zap::ParaBiddybud::execute() {
    // Movement
    if (isState(StateID_Idle)) {
        mMovementHandler.execute();
        mPos = mMovementHandler.getPosition();
        //mAngle.z() = mMovementHandler.getAngle();
    }

    // Delete when offscreen
    screenOutCheck(0);
    
    executeState();

    calcMdl_Base();
    
    // TODO: Track skeletal anim?
    
    return true; 
}

bool zap::ParaBiddybud::draw() {
    mModel->draw();
    return true;
}

void zap::ParaBiddybud::calcMdl_Base() {
    mModel->update(mPos + sead::Vector3f(0.0f, -8.0f, 0.0f), mAngle, mScale, !isState(StateID_Ice));
}

bool zap::ParaBiddybud::createIceActor() {
    IceInfo info = { 
        IceInfo::makeParam(cIceType_Square),
        { mPos.x, mPos.y - 8.0f, mPos.z },
        mScale * cInvScaleFactor * 1.5f,
        nullptr
    };
    return mIceMgr.createIce(info);
}

void zap::ParaBiddybud::vsPlayerHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
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

void zap::ParaBiddybud::vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
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

void zap::ParaBiddybud::initializeState_Idle() {
    
}

void zap::ParaBiddybud::executeState_Idle() {
    
}

void zap::ParaBiddybud::finalizeState_Idle() { }

/** STATE: DieOther */

void zap::ParaBiddybud::initializeState_DieOther() {
    mModel->setAnm("BlowDown", 0.0f, FrameCtrl::cMode_NoRepeat);
    removeCollisionCheck();
}

void zap::ParaBiddybud::executeState_DieOther() {
    // Wait for squish to finish before deleting
    if (mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        deleteRequest();
        removeCollisionCheck();
        
        // Spawn a puff effect
        // TODO: Change this effect to the correct one from Yoshi stomp?
        EffectCreateUtil::createEffect(RP_Cmn_EnemyBurst_00, &mPos);
    }
}

void zap::ParaBiddybud::finalizeState_DieOther() { }
