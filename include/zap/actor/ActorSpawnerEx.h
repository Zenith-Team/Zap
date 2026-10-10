#pragma once
#include <actor/Actor.h>
#include <actor/Profile.h>

namespace zap {

class ActorSpawnerEx : public Actor {
    SEAD_RTTI_OVERRIDE(ActorSpawnerEx, Actor);

    enum SpawnerMode {
        cSpawnerMode_SingleUse = 0,
        cSpawnerMode_MultiUse,
        cSpawnerMode_Linked,
    };

public:
    static Profile* sProfile;
    
    ActorSpawnerEx(const ActorCreateParam& param);
    ~ActorSpawnerEx() override = default;

    Result create() override;
    bool execute() override;

private:
    // bank scanners
    bool scanString();
    bool scanNybble();

private:
    static const ActorCreateInfo cCreateInfo;

    u16 mSpawnProfileID;
    u16 mSpawnEventID;
    u8 mStringScanAttempt;
    u8 mNybbleScanAttempt;
    bool mSpawned;
    SpawnerMode mSpawnerMode;
    bool mPrevFrameEvent;
    ActorUniqueID mNybbleBank;
    sead::Vector3f mStartPos;
    ActorUniqueID mChildActor;
};

} // namespace zap
