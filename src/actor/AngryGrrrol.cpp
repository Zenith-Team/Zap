#include <zap/actor/AngryGrrrol.h>
#include <audio/GameAudio.h>
#include <zap/Zap.h>
#include <red/utility/SpriteUtil.h>
#include <effect/EffectCreateUtil.h>
#include <player/PlayerBase.h>
#include <map/Bg.h>

/*
    TODO:
    - Grrrols have slight bobbing during movement, figure out how they do it
*/

constexpr f32 cChaseAcceleration = 0.00025f;
constexpr f32 cBaseSpeed = 0.5f;

SEAD_RTTI_OVERRIDE_IMPL(zap::AngryGrrrol, Enemy);
CREATE_STATE_ID(zap::AngryGrrrol, Roll)

const ActorCreateInfo zap::AngryGrrrol::cCreateInfo = {
    .offset_x = 16, .offset_y = -16,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 16, .half_size_y = 16
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_NoCoinEdit
};

using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData zap::AngryGrrrol::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 12.0f, 12.0f },
    .shape_type = CC::cShapeType_Circle,
    .kind = CC::cKind_Enemy,
    .attack = CC::cAttack_Shell,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player |
        CC::cTargetKind_Yoshi |
        CC::cTargetKind_Enemy |
        CC::cTargetKind_Killer |
        CC::cTargetKind_Tama |
        CC::cTargetKind_ChibiYoshi |
        CC::cTargetKind_DrcTouch
    ),
    .vs_damage = CC::DamageFrom(
        CC::cDamageFrom_FireBall |
        CC::cDamageFrom_IceBall |
        CC::cDamageFrom_Star |
        CC::cDamageFrom_YoshiEat
    ),
    .status = CC::cStatus_MoveKill,
    .callback = &Enemy::normal_collcheck
};

// Gamepad touch callback
bool zap::AngryGrrrol::DrcTouchCB::ccSetTouchNormal(ActorCollisionCheck* cc, const sead::Vector2f& pos) {
    AngryGrrrol* grrrol = cc->getOwner<AngryGrrrol>();
    if (grrrol != nullptr) {
        grrrol->mBoyoMgr.begin(24, 0.04f);
        if (grrrol->bgCheckFoot_()) {
            grrrol->mSpeed.y = 2.0f;
        }
    }
    return true;
}

Profile* zap::AngryGrrrol::sProfile = zap::getRegistrar()->newProfile<zap::AngryGrrrol>("angrygrrrol")
    .resources<"guruguru">(ProfileInfo::cResType_Course)
    .createInfo(cCreateInfo)
    .build();

zap::AngryGrrrol::AngryGrrrol(const ActorCreateParam& param)
    : Enemy(param)
    , mModel(nullptr)
    , mYoshiEatData(mActorUniqueID)
    , mBoyoMgr(this)
    , mTouchingWall(false)
    , mTouchingFloor(true)
    , mTouchingWater(false)
{ }

ActorBase::Result zap::AngryGrrrol::create() {
    // Model
    mModel = AnimModel::create("guruguru", "guruguru", 0, 0, 1);
    mModel->playTexSrtAnim("guruguru");
    
    // Hitbox
    mCollisionCheck.set(this, AngryGrrrol::cCollisionData, &mDrcTouchCallback);
    reviveCollisionCheck();
    
    // Terrain collision
    static const ActorBgCollisionCheck::Sensor foot = { -8.0f, 8.0f, -16.0f };
    static const ActorBgCollisionCheck::Sensor headwall = { -8.0f, 8.0f, 16.0f };
    mBgCheckObj.set(this, &foot, &headwall, &headwall);
    
    // Flags copied from Grrrol, I don't know what all these do
    using F = ActorBgCollisionCheck::SensorFlag::Bit;
    mBgCheckObj.getSensorFlag(cDirType_Down).setBit(
        F::cBit_43
    );
    mBgCheckObj.getSensorFlag(cDirType_Right).setBit(
        F::cBit_6, F::cBit_9, F::cBit_10, F::cBit_15, F::cBit_26, F::cBit_27, F::cBit_36, F::cBit_43, F::cBit_54, F::cBit_56
    );
    mBgCheckObj.getSensorFlag(cDirType_Left).setBit(
        F::cBit_6, F::cBit_9, F::cBit_10, F::cBit_15, F::cBit_26, F::cBit_27, F::cBit_36, F::cBit_43, F::cBit_54, F::cBit_56
    );
    mBgCheckObj.getSensorFlag(cDirType_Up).setBit(
        F::cBit_6, F::cBit_15, F::cBit_43, F::cBit_54, F::cBit_56
    );
    
    // Setting: Break Blocks (Sides)
    if (red::SpriteUtil::getNybble7(this) != 0) {
        mBgCheckObj.getSensorFlag(cDirType_Up).setBit(F::cBit_BreakBlocks);
        mBgCheckObj.getSensorFlag(cDirType_Left).setBit(F::cBit_BreakBlocks);
        mBgCheckObj.getSensorFlag(cDirType_Right).setBit(F::cBit_BreakBlocks);
    }
    
    // Setting: Break Blocks (Down)
    if (red::SpriteUtil::getNybble8(this) != 0) {
        mBgCheckObj.getSensorFlag(cDirType_Down).setBit(F::cBit_BreakBlocks);
    }

    // Yoshi eat ability
    mEatDataPtr = &mYoshiEatData;
    mYoshiEatData.setEatType(EatData::cEatType_None);

    // Disable baby yoshi bubbles
    mChibiYoshiAwaDataPtr = &mChibiYoshiAwaData;
    mChibiYoshiAwaData.setAwaType(ChibiYoshiAwaData::cAwaType_Invalid);

    mPos.z = 432.0f;

    changeState(StateID_Roll);
    
    mBoyoMgr.initialize();

    calcMdl_Normal();
    return cResult_Success;
}

bool zap::AngryGrrrol::execute() {
    executeState();

    // Play splash effect if entering a liquid
    waterSplashEffect();

    // Delete when offscreen
    screenOutCheck(cScreenOutFlag_SkipNone);

    // Kill if crushed
    if (hasamareBgCheck_()) {
        setDeathInfo_Hasami();
    }

    calcMdl_Normal();

    mBoyoMgr.execute();
    
    return true;
}

bool zap::AngryGrrrol::draw() {
    mModel->draw();
    return true;
}

void zap::AngryGrrrol::waterSplashEffect(const sead::Vector3f& pos) {
    sead::Vector2f waterPos(pos.x, pos.y);
    Bg::instance()->setWaterInWave(waterPos, 6);
    GameAudio::getAudioObjEmy()->startSound("SE_OBJ_CMN_SPLASH", mPos);
}

void zap::AngryGrrrol::yoganSplashEffect(const sead::Vector3f& pos) {
    splashEffect_(pos, RP_Cmn_LavaSplash_42, 16, "SE_OBJ_CMN_SPLASH_LAVA");
}

void zap::AngryGrrrol::poisonSplashEffect(const sead::Vector3f& pos) {
    splashEffect_(pos, RP_Cmn_PoisonSplash_36, 4, "SE_OBJ_CMN_SPLASH_POISON");
}

void zap::AngryGrrrol::calcMdl_Base() {
    mModel->update(mPos, mAngle, mBoyoMgr.getScale());
}

void zap::AngryGrrrol::vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    FumiType fumiType = fumiCheck(cc_self, cc_other, cFumiSeType_Step);
    if (fumiType == cFumiType_Hit) {
        return Enemy::vsYoshiHitCheck_Normal(cc_self, cc_other);
    }
}

bool zap::AngryGrrrol::hitCallback_Shell(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    // Check for collisions with other Angry Grrrols
    AngryGrrrol* other_grrrol = cc_other->getOwner<AngryGrrrol>();
    if (other_grrrol) {
        // Get collision direction
        DirType collisionDirection = static_cast<DirType>(cc_self->getRevisionX(ActorCollisionCheck::cKind_Enemy) > 0.0f);
        if (mSpeed.x > 0.0f && collisionDirection == cDirType_Right) { // Moving right, should only bounce back if colliding with a left facing Grrrol
            sead::Vector3f effectPos = mPos + sead::Vector3f(11.0f, 0.0f, 0.0f);
            EffectCreateUtil::createEffect(RP_Enm_Collision_5, &effectPos);
            // play a sound effect
            GameAudio::getAudioObjMap()->startSound("SE_OBJ_TEKKYU_CRASH", mPos, calculateSoundVolume());
            // Invert speed
            mSpeed.x = -mSpeed.x;
        } else if (mSpeed.x < 0.0f && collisionDirection == cDirType_Left) { // Moving left, should only bounce back if colliding with a right facing Grrrol
            sead::Vector3f effectPos = mPos + sead::Vector3f(-11.0f, 0.0f, 0.0f);
            EffectCreateUtil::createEffect(RP_Enm_Collision_5, &effectPos);
            // play a sound effect
            GameAudio::getAudioObjMap()->startSound("SE_OBJ_TEKKYU_CRASH", mPos, calculateSoundVolume());
            // Invert speed
            mSpeed.x = -mSpeed.x;
        }
    }
    // Check for collisions with other normal Grrrols
    Enemy* other = cc_other->getOwner<Enemy>();
    if (other) {
        s32 profid = cc_other->getOwner()->getProfileID();
        // Collided with a Big Grrrol, kill ourselves
        if (profid == ProfileInfo::cProfileID_BigGrrrol || profid == ProfileInfo::cProfileID_BigGrrrolRDash) {
            return Enemy::hitCallback_Shell(cc_self, cc_other);
        }
        // Collided with a normal Grrrol, kill whichever has the faster speed
        if (profid == ProfileInfo::cProfileID_Grrrol) {
            if (sead::Mathf::abs(mSpeed.x) > sead::Mathf::abs(other->getSpeedVec().x)) {
                other->Enemy::hitCallback_Shell(cc_other, cc_self);
            } else {
                return Enemy::hitCallback_Shell(cc_self, cc_other);
            }
        }
    }
    return false;
}

bool zap::AngryGrrrol::hitCallback_Fire(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    fireballInvalid(cc_other);
    return true;
}

bool zap::AngryGrrrol::hitCallback_Ice(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
    iceballInvalid(cc_other);
    return true;
}

void zap::AngryGrrrol::yoshiFumiScoreSet(Actor* player) { }

s16 zap::AngryGrrrol::calculateSoundVolume() {
    // Calculate volume based on speed
    s16 soundVolume = 0;
    float absSpeedX = sead::Mathf::abs(mSpeed.x);
    float absSpeedY = sead::Mathf::abs(mSpeed.y);
    if (absSpeedY <= absSpeedX) {
        soundVolume = absSpeedX * 10.0f;
    } else {
        soundVolume = absSpeedY * 10.0f;
    }
    return soundVolume;
}

void zap::AngryGrrrol::waterSplashEffect() {
    // Check for any collisions with liquids
    f32 waterPosY;
    WaterType waterType = checkWater_(&waterPosY);
    if (waterType == cWaterType_None) {
        // Reset every frame we're not in liquid
        mTouchingWater = false;
    } else if (mTouchingWater == false) {
        // First frame touching liquid
        mTouchingWater = true;
        sead::Vector3f splashPos(mPos.x, waterPosY, EFFECT_Z_POS_DEFAULT);
        switch (waterType) {
            case cWaterType_Water:
            case cWaterType_AirWater:
                waterSplashEffect(splashPos);
                break;
            case cWaterType_Lava:
                yoganSplashEffect(splashPos);
                break;
            case cWaterType_LavaWave:
                yoganWaveSplashEffect(splashPos);
                break;
            case cWaterType_Poison:
                poisonSplashEffect(splashPos);
                break;
        }
    }
}

void zap::AngryGrrrol::initializeState_Roll() { }

void zap::AngryGrrrol::executeState_Roll() {
    // Chase player
    
    sead::Vector2f distanceToPlayer;
    if (searchNearPlayer(distanceToPlayer) == -1)
        return; // No player found
    
    // Setting: Acceleration
    const f32 accel = cChaseAcceleration * ((red::SpriteUtil::getNybble6(this) + 4) / 4.0f);
    mSpeed.x += distanceToPlayer.x * accel;
    
    // Setting: Maximum Speed
    const f32 maxSpeed = cBaseSpeed * (red::SpriteUtil::getNybble5(this) + 1);
    mSpeed.x = sead::Mathf::clamp2(-maxSpeed, mSpeed.x, maxSpeed);

    calcSpeedY_(); // apply gravity
    posMove_();    // apply velocity
    
    // Terrain collision
    bgCheck_();
    if (bgCheckFoot_()) {
        mSpeed.y = 0; // stop accelerating downwards if we hit the ground

        // Sparks effect
        if (sead::Mathf::abs(mSpeed.x) > 0.7f) {
            const sead::Vector3f effectPos(mPos.x, mPos.y - 16.0f, mPos.z + 50.0f);
            mEffectSparks.createEffect(RP_Gorogoro_move_0, &effectPos);
        }

        if (!mTouchingFloor) {
            // We *just* hit a floor that we weren't touching already
            // spawn the landing effect
            sead::Vector3f effectPos = mPos + sead::Vector3f(0.0f, -16.0f, 0.0f);
            EffectCreateUtil::createEffect(RP_Cmn_LandingSmoke_35, &effectPos);
            // play a sound effect
            GameAudio::getAudioObjMap()->startSound("SE_OBJ_TEKKYU_CRASH", mPos, calculateSoundVolume());
        }

        mTouchingFloor = true;
    } else {
        mTouchingFloor = false; // reset every frame
    }
    
    if (bgCheckWall_()) { // this function just tells you if you're currently touching a wall, but who knows for how long! (so track the state in mTouchingWall)
        // If the speed we touch the wall with is less than 0.1f, just set the X speed to 0 to prevent spinning against walls
        if (sead::Mathf::abs(mSpeed.x) < 0.1f) {
            mSpeed.x = 0.0f;
        }
        if (!mTouchingWall) {
            // We *just* hit a wall that we weren't touching already
            // spawn a particle effect
            sead::Vector3f effectPos = mPos + sead::Vector3f(mBgCheckObj.checkWall(cDirType_Left) ? -11.0f : 11.0f, 0.0f, 0.0f);
            EffectCreateUtil::createEffect(RP_Enm_Collision_5, &effectPos);
            // play a sound effect
            GameAudio::getAudioObjMap()->startSound("SE_OBJ_TEKKYU_CRASH", mPos, calculateSoundVolume());
            // Invert speed
            mSpeed.x = -mSpeed.x * 0.5f;
        }
        
        mTouchingWall = true;
    } else {
        mTouchingWall = false; // reset every frame
    }
    
    // Roll sound
    GameAudio::getAudioObjMap()->holdSound("SE_OBJ_TEKKYU_ROLL", mActorUniqueID.getValue(), mPos, calculateSoundVolume());
    
    // Rotate model
    mAngle.z() -= sead::Mathf::deg2idx(3.1f * mSpeed.x);
}

void zap::AngryGrrrol::finalizeState_Roll() { }
