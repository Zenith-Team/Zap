#include <zap/actor/AngryGrrrol.h>
#include <audio/GameAudio.h>
#include <zap/Zap.h>
#include <red/utility/SpriteUtil.h>
#include <effect/EffectCreateUtil.h>

/*
    TODO:
    - DRCTouch
    - Land effect? (RP_Cmn_LandingSmoke_35)
*/

constexpr f32 cChaseAcceleration = 0.00025f;
constexpr f32 cBaseSpeed = 0.5f;

SEAD_RTTI_OVERRIDE_IMPL(zap::AngryGrrrol, Enemy);

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
        CC::cTargetKind_ChibiYoshi
    ),
    .vs_damage = CC::DamageFrom(
        CC::cDamageFrom_FireBall |
        CC::cDamageFrom_IceBall |
        CC::cDamageFrom_Star
    ),
    .status = CC::cStatus_MoveKill,
    .callback = &Enemy::normal_collcheck
};

Profile* zap::AngryGrrrol::sProfile = zap::getRegistrar()->newProfile<zap::AngryGrrrol>("angrygrrrol")
    .resources<"guruguru">(ProfileInfo::cResType_Course)
    .build();

zap::AngryGrrrol::AngryGrrrol(const ActorCreateParam& param)
    : Enemy(param)
    , mModel(nullptr)
    , mTouchingWall(false)
{ }

ActorBase::Result zap::AngryGrrrol::create() {
    // Model
    mModel = AnimModel::create("guruguru", "guruguru", 0, 0, 1);
    mModel->playTexSrtAnim("guruguru");
    
    // Hitbox
    mCollisionCheck.set(this, AngryGrrrol::cCollisionData);
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
    
    calcMdl_Normal();
    return cResult_Success;
}

bool zap::AngryGrrrol::execute() {
    // Delete when offscreen
    screenOutCheck(cScreenOutFlag_SkipNone);
    
    // Chase player
    
    sead::Vector2f distanceToPlayer;
    if (searchNearPlayer(distanceToPlayer) == -1)
        return true; // No player found
    
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
    }
    
    if (bgCheckWall_()) { // this function just tells you if you're currently touching a wall, but who knows for how long! (so track the state in mTouchingWall)
        mSpeed.x = 0;
        
        if (!mTouchingWall) {
            // We *just* hit a wall that we weren't touching already
            // spawn a particle effect
            sead::Vector3f effectPos = mPos + sead::Vector3f(mBgCheckObj.checkWall(cDirType_Left) ? -11.0f : 11.0f, 0.0f, 0.0f);
            EffectCreateUtil::createEffect(RP_Enm_Collision_5, &effectPos);
            // play a sound effect
            GameAudio::getAudioObjMap()->startSound("SE_OBJ_TEKKYU_CRASH", mPos);
        }
        
        mTouchingWall = true;
    } else {
        mTouchingWall = false; // reset every frame
    }
    
    // Sparks effect
    if (sead::Mathf::abs(mSpeed.x) > 0.7f) {
        const sead::Vector3f effectPos(mPos.x, mPos.y - 16.0f, mPos.z + 50.0f);
        mEffectSparks.createEffect(RP_Gorogoro_move_0, &effectPos);
    }
    
    // Roll sound
    // TODO: Only play when moving
    GameAudio::getAudioObjMap()->holdSound("SE_OBJ_TEKKYU_ROLL", mActorUniqueID.getValue(), mPos, 17);
    
    // Rotate model
    mAngle.z() -= sead::Mathf::deg2idx(3.1f * mSpeed.x);
    calcMdl_Normal();
    
    return true;
}

bool zap::AngryGrrrol::draw() {
    mModel->draw();
    return true;
}

void zap::AngryGrrrol::calcMdl_Base() {
    mModel->update(mPos, mAngle, mScale);
}
