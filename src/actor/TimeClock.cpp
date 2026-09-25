#include <zap/actor/TimeClock.h>
#include <zap/Zap.h>
#include <audio/GameAudio.h>
#include <effect/EffectCreateUtil.h>
#include <game/CourseTimer.h>
#include <red/utility/SpriteUtil.h>
#include <map/SwitchFlagMgr.h>
#include <game/CourseTask.h>
#include <map/CoinOrigin.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::TimeClock, ActorState);

CREATE_STATE_ID(zap::TimeClock, Active)
CREATE_STATE_ID(zap::TimeClock, Collecting)

static constexpr f32 cCollectAnimDuration = 9.0f; // frames
static constexpr f32 cCollectAnimTiles = 1.5f;

static constexpr sead::SafeArray<u32, 6> cTimes = {10, 1, 5, 30, 50, 100};

// Register it
Profile* zap::TimeClock::sProfile = zap::getRegistrar()->newProfile<zap::TimeClock>("timeclock")
    .resources<"timeclock">(ProfileInfo::cResType_Course)
    .createInfo(cCreateInfo)
    .build();

const ActorCreateInfo zap::TimeClock::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_MapObj
};

// Hitbox data
using CC = ActorCollisionCheck;
const ActorCollisionCheck::CollisionData zap::TimeClock::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 12.0f, 12.0f },
    .shape_type = CC::cShapeType_Box,
    .kind = CC::cKind_Item,
    .attack = CC::cAttack_None,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player
    ),
    .vs_damage = CC::cDamageFrom_All,
    .status = CC::cStatus_None,
    .callback = [](ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
        if (TimeClock* self = cc_self->getOwner<TimeClock>(); self != nullptr)
            self->collect(cc_other->getOwner()->getPlayerNo());
    }
};

    
// Main code
zap::TimeClock::TimeClock(const ActorCreateParam& param)
    : ActorState(param)
    , mModel(nullptr)
    , mMovementHandler()
    , mReactivationEvent(0)
    , mCollectionEvent(0)
    , mActorAliveTime(0.0f)
    , mCollectAnimProgress(0.0f)
    , mBadClock(false)
    , mSmallClock(false)
    , mGreenTex(false)
    , mDisableSfx(false)
    , mUseBonusAnim(false)
    , mUseCollectAnim(false)
{ }

ActorBase::Result zap::TimeClock::create() {
    // Movement setup
    const u8 nybble20 = red::SpriteUtil::getNybble20(this);
    if (nybble20 > cPos_KinokoLift) {
        tk::fatal("Movement type was out of bounds");
    }
    const ParentMovementType movementType = static_cast<ParentMovementType>(nybble20);
    u32 movementMask = mMovementHandler.getTypeMask(movementType);

    setupMovement(mPos, movementMask, movementType, mParamEx.course.movement_id);

    // Model
    mModel = AnimModel::create("timeclock", "timeclockA", 0, 1);
    
    // Hitbox
    mCollisionCheck.set(this, cCollisionData);
    
    // Time selection
    mTimeSelectionDelta = cTimes[mParam0 & 0xFFF];
    
    // small clock

    mSmallClock = red::SpriteUtil::getNybble14(this);

    if (mSmallClock) {
        mCollisionCheck.setHalfSize(6.0f, 6.0f);
        mScale = {0.5f, 0.5f, 0.5f};
    }
    
    // bad clock
    mBadClock = red::SpriteUtil::getNybble13(this);
    
    // set color
    mGreenTex = (mTimeSelectionDelta >= 30 && !mBadClock);
    
    // green 0, blue 1, red 2
    u32 tex = mBadClock ? 2 : (mGreenTex ? 0 : 1); 

    // texture anim for colors
    mModel->playTexAnim("tex");
    mModel->getTexAnim(0)->getFrameCtrl().setFrame(tex);
    // pause it 
    mModel->getTexAnim(0)->getFrameCtrl().setRate(0.0f);

    // disable sfx
    mDisableSfx = red::SpriteUtil::getNybble9(this);

    // use collect anim
    mUseBonusAnim = red::SpriteUtil::getNybble1(this);
    // use bonus anim
    mUseCollectAnim = red::SpriteUtil::getNybble2(this);
    
    // Event IDs
    mReactivationEvent = (red::SpriteUtil::getNybble5(this) << 4) | red::SpriteUtil::getNybble6(this);
    mCollectionEvent = (red::SpriteUtil::getNybble7(this) << 4) | red::SpriteUtil::getNybble8(this);
    
    // Setup effects
    if (mBadClock) {
        mPulseColor = { 10.0f, 0.0f, 0.0f, 1.0f };
    } else if (mGreenTex) {
        mPulseColor = { 0.2f, 10.0f, 0.1f, 1.0f };
    } else { // blue
        mPulseColor = { 1.0f, 1.0f, 2.0f, 0.0f };
    }

    changeState(StateID_Active);

    updateModel(); // make sure it appears on the first frame

    return cResult_Success;
}

void zap::TimeClock::setupMovement(const sead::Vector3f& position, u32 movement_mask, ParentMovementType movement_type, u32 movement_id) {
    // use different link function if pivotal rotation, prevents glitches
    if (movement_type == ParentMovementType::cPos_CenterRotation) {
        mMovementHandler.linkPivotal(mPos, movement_mask, movement_id);
    } else {
        mMovementHandler.link(mPos, movement_mask, movement_id);
    }

    // set type-specific data
    setMovementParamaters(movement_type);

    mMovementHandler.execute();
}

void zap::TimeClock::setMovementParamaters(ParentMovementType movement_type) {
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

bool zap::TimeClock::execute() {
    // Delete when offscreen
    screenOutCheck(0);

    // handle movement
    mMovementHandler.execute();
    mPos.x = mMovementHandler.getPosition().x;
    mPos.y = mMovementHandler.getPosition().y;

    updateModel();

    executeState();

    // update the effect color
    mPulseEffect.setColor(mPulseColor);

    return true;
}

bool zap::TimeClock::draw() {
    mModel->draw();
    return true;
}

void zap::TimeClock::updateModel() const {
    mModel->update(mPos, mAngle, mScale);
}

void zap::TimeClock::collect(s8 player) {
    if (isState(StateID_Active)) {
        tk::println("timeclock::collect (active)");
        if (mCollectionEvent != 0)  {
            // an event is set
            SwitchFlagMgr::instance()->set(mCollectionEvent - 1, 0, true);
        }

        removeCollisionCheck();
        setPlayerNo(player);
        changeState(StateID_Collecting);
    }
    tk::println("timeclock::collect (inactive;skipped)");
    return;
}

/** STATE: Active */
void zap::TimeClock::initializeState_Active() { 
    tk::println(":3 Active!");
    reviveCollisionCheck();
    mAngle.y() = CoinOrigin::instance()->getCoinAngle() * (mBadClock ? -0.5f : 0.5f);
}

void zap::TimeClock::executeState_Active() {
    //* angle overflows every rotation, but we can get away with exactly 0.5x (180 degrees) because the model is symmetrical
    mAngle.y() = CoinOrigin::instance()->getCoinAngle() * (mBadClock ? -0.5f : 0.5f);

    sead::Vector3f scale = sead::Vector3f(0.7f, 0.7f, 0.7f);
    sead::Vector3f scaleSmall = sead::Vector3f(0.4f, 0.4f, 0.4f);

    EffectID effect = (mBadClock || !mGreenTex) ? RP_RingRed : RP_RingGreen;
    
    mSparkleEffect.createEffect(effect, &mPos, nullptr, mSmallClock ? &scaleSmall : &scale);
    
    if (!mBadClock && !mGreenTex) // blue clock needs color change since there's no blue ring effect
        mSparkleEffect.setColor({ 0.2f, 2.0f, 6.0f, 1.0f });

    // Remove the ring glow from the coin ring effect emitter so it just uses the sparkles
    mSparkleEffect.setVisible(false, 2);

    //TODO: drctouch effect
    // if (CoinOrigin::instance()->getCoinAngle() == 0) {
    //     mPulseEffect.kill();
    //     mPulseEffect.createEffect(RP_UI_DRC_Touch, &mPos, nullptr, &scale);
    //     mPulseEffect.setColor(mPulseColor);
    // }
}

void zap::TimeClock::finalizeState_Active() { }

/** STATE: Collecting */
void zap::TimeClock::initializeState_Collecting() { 
    tk::println(":3 Collecting");

    // Sound
    if (!mDisableSfx && !mUseBonusAnim) { // bonustime has its own sound already
        if (mBadClock) {
            GameAudio::getAudioObjMap()->startSound("SE_MG_CM_PANEL_NG", mPos);
        } else {
            if (mSmallClock) {
                GameAudio::getAudioObjMap()->startSound("SE_MG_CM_PANEL_OK", mPos);
            } else {
                GameAudio::getAudioObjMap()->startSound("SE_BOSS_CMN_GET_COIN_BONUS", mPos);
            }
        }
    }

    s16 time = mBadClock ? -mTimeSelectionDelta : mTimeSelectionDelta;
    bool timeDepleted = mBadClock && (static_cast<s32>(CourseTimer::fromUnits(CourseTimer::instance()->getTime())) - static_cast<s32>(mTimeSelectionDelta) <= 0);
    if (!timeDepleted) {
        if (mUseBonusAnim) {
            // bonus time handles time
            CourseTimer::instance()->setBonusTime(time);
            CourseTask::instance()->doBonusTime(mPlayerNo);
        } else {
            // add the time
            CourseTimer::instance()->addTimeLimitSeconds(time);
        }
    } else {
        CourseTimer::instance()->setTimer(0);
    }
    
    // smallclock scale
    sead::Vector3f effectScale = sead::Vector3f(1.8f, 1.8f, 1.8f);
    EffectID collectEffect = mBadClock ? RP_CoinRedGet : (mGreenTex ? RP_CoinGreenGet : RP_CoinBlueGet);
    EffectCreateUtil::createEffect(collectEffect, &mPos, nullptr, mSmallClock ? nullptr : &effectScale); // dont enlarge the scale for small clock
    
    if (!mUseCollectAnim || mBadClock) { // Don't use collect animation
        deleteActor(true);
    } // use collect animation: continue to collecting state execute; plays anim
}

void zap::TimeClock::executeState_Collecting() { 
    tk::println("timeclock::executeState_Collecting");
    if (mCollectAnimProgress >= sead::Mathf::pi()) { 
        deleteActor(true);
    }
    
    f32 prevSin = sead::Mathf::sin(mCollectAnimProgress);
    mCollectAnimProgress += 1.0f / cCollectAnimDuration;
    f32 yOffset = (sead::Mathf::sin(mCollectAnimProgress) - prevSin) * (cCollectAnimTiles * 16.0f);

    mPos.y += yOffset;

    mAngle.y() += sead::Mathf::deg2idx(mBadClock ? -30.0f : 30.0f);
}

void zap::TimeClock::finalizeState_Collecting() { }

/** TODO
 * Movement controller 🔄 test
 * Event activation (SwitchFlagMgr) 🔄 test
 * Add an alternative "simple" animation, similar to points text ui or maybe sparkles at the timer ui
 * Badclock no collect animation allowed in spritedata
 * Fake timeclock: RP_ObakeDoor_Disapp or RP_Poltergeist_Disapp, SFX: SE_EMY_FIRE_SNAKE_EXTINCT or boo laugh, how does it look?
 */
