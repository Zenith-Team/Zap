#pragma once

#include <actor/ActorState.h>
#include <actor/Profile.h>
#include <graphics/AnimModel.h>
#include <effect/EffectObj.h>
#include <map_obj/ParentMovementMgr.h>

namespace zap {

class TimeClock : public ActorState {
    SEAD_RTTI_OVERRIDE(TimeClock, ActorState)

public:
    static Profile* sProfile;

    TimeClock(const ActorCreateParam& param);
    ~TimeClock() override = default;
    
    Result create() override;
    bool execute() override;
    bool draw() override; 

    void setupMovement(const sead::Vector3f& position, u32 movement_mask, ParentMovementType movement_type, u32 movement_id);
    void setMovementParamaters(ParentMovementType movement_type);
    
    void updateModel() const;
    void collect(s8 player);
    
    static const ActorCreateInfo cCreateInfo;
    static const ActorCollisionCheck::CollisionData cCollisionData;
    
private:
    AnimModel* mModel;
    ParentMovementMgr mMovementHandler;

    u32 mReactivationEvent;
    u32 mCollectionEvent;

    f32 mActorAliveTime;
    f32 mCollectAnimProgress;

    bool mBadClock;
    bool mSmallClock;
    bool mGreenTex;

    bool mDisableSfx;

    bool mUseBonusAnim;
    bool mUseCollectAnim;

    EffectObj mSparkleEffect;
    EffectObj mPulseEffect;
    sead::Color4f mPulseColor;

    s16 mTimeSelectionDelta;

    DECLARE_STATE_ID(TimeClock, Active)
    DECLARE_STATE_ID(TimeClock, Collecting)
};

} // namespace zap
