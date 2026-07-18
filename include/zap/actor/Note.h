#pragma once

#include <telkin/Print.h>
#include <actor/ActorState.h>
#include <actor/Actor.h>
#include <actor/Profile.h>
#include <graphics/AnimModel.h>
#include <map_obj/ParentMovementMgr.h>

namespace zap {

class Note : public ActorMultiState {
    SEAD_RTTI_OVERRIDE(Note, ActorMultiState)

public:
    static Profile* sProfile;

    Note(const ActorCreateParam& param);
    ~Note() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override; 
    
    void updateModel();
    void setParent(ActorUniqueID parent) {
        mClefParent = parent;
    }

    void collect(s8);
    void reset();
    
    [[nodiscard]]
    u8 getManagerID() const {
        return mManagerID;
    }

    [[nodiscard]]
    u8 getPhaseID() const {
        return mPhaseID;
    }

    [[nodiscard]]
    bool isCollected() const {
        return mCollected;
    }

    static const ActorCreateInfo cCreateInfo;
    static const ActorCollisionCheck::CollisionData cCollisionData;

    DECLARE_STATE_ID(Note, Idle)
    DECLARE_STATE_ID(Note, Active)
    DECLARE_STATE_ID(Note, AnimateCollecting)
    DECLARE_STATE_ID(Note, AnimateAppear)
    DECLARE_STATE_ID(Note, AnimateDisappear)
    DECLARE_STATE_ID(Note, AnimateExpiry)

private:
    AnimModel* mModel;
    
    ActorUniqueID mClefParent;
    
    u8 mManagerID;
    u8 mPhaseID;

    bool mCollected;

    u32 mWarnTime;

    ParentMovementMgr mMovementHandler;
};

} // namespace zap
