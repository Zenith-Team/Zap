#pragma once

#include <enemy/Enemy.h>
#include <actor/Profile.h>
#include <graphics/JointBlendModel.h>
#include <map_obj/ParentMovementMgr.h>
#include <enemy/EnemyEatData.h>
#include <enemy/EnemyChibiYoshiEatData.h>

namespace zap {

class ParaBiddybud : public Enemy {
    SEAD_RTTI_OVERRIDE(ParaBiddybud, Enemy)

public:
    static Profile* sProfile;

    ParaBiddybud(const ActorCreateParam& param);
    ~ParaBiddybud() override = default;

    Result create() override;
    bool execute() override;
    bool draw() override;

    void setupMovement(const sead::Vector3f& position, u32 movement_mask, ParentMovementType movement_type, u32 movement_id);
    void setMovementParamaters(ParentMovementType movement_type);

    bool createIceActor() override;
    void calcMdl_Base() override;

    void vsPlayerHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;
    void vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;

    static const ActorCreateInfo cCreateInfo;
    static const ActorCollisionCheck::CollisionData cCollisionData;

    DECLARE_STATE_ID(ParaBiddybud, Idle)

    DECLARE_STATE_VIRTUAL_ID_OVERRIDE(ParaBiddybud, DieOther)

private:
    JointBlendModel* mModel;
    EnemyEatData mYoshiEatData;
    EnemyChibiYoshiEatData mChibiYoshiEatData;
    ParentMovementMgr mMovementHandler;
};

} // namespace zap
