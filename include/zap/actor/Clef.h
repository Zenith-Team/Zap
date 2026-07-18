#pragma once

#include <zap/actor/Note.h>
#include <actor/Profile.h>
#include <effect/EffectObj.h>
#include <actor/ActorState.h>
#include <graphics/AnimModel.h>
#include <map_obj/ParentMovementMgr.h>
#include <red/actor/event/FreezeFrameEvent.h>

namespace zap {

class Clef : public ActorMultiState {
    SEAD_RTTI_OVERRIDE(Clef, ActorMultiState);

public:
    static constexpr u8 cPhaseLimit = 8;
    
public:
    static Profile* sProfile;

    Clef(const ActorCreateParam& param);    
    ~Clef() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override; 
    
    void scanNotes();
    
    void updateModel();
    void collect(s8 playerNo);

    void noteCollected(Note* note);

    static const ActorCreateInfo cCreateInfo;
    static const ActorCollisionCheck::CollisionData cCollisionData;

    DECLARE_STATE_ID(Clef, Waiting)
    DECLARE_STATE_ID(Clef, GameActive)
    DECLARE_STATE_ID(Clef, AnimateCollecting)
    DECLARE_STATE_ID(Clef, AnimateAppear)

private:
    void setCurrentNotesState(const StateID& state) const;
    void setAllNotesState(const StateID& state) const;

    AnimModel* mClefModel;
    f32 mTime;
    u32 mManagerID;
    u32 mRewardID;

    u32 mTimeLimit;

    u32 mScanAttempt;
    bool mReady;

    u8 mAttemptsRemaining;
    
    u8 mPhaseCount;
    u32 mTargetNoteCount;
    
    u32 mCurrentPhaseTimer;

    u8 mActivePhaseID;
    u32 mCollectedNoteCount;

    s8 mCollectedPlayer;

    u32 mStartCollectAnimTime;

    u16 mGameWonEventID;
    
    sead::SafeArray<ActorUniqueID*, 8> nNotes;

    EffectObj mEffect1;
    EffectObj mEffect2;
    EffectObj mEffect3;
    EffectObj mEffect4;

    ParentMovementMgr mMovementHandler;

    red::FreezeFrameEvent<zap::Clef, zap::Note> mFreezeEvent;
};

} // namespace zap
