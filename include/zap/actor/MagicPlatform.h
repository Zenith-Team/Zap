#pragma once

#include <player/PlayerObject.h>
#include <actor/Actor.h>
#include <actor/Profile.h>
#include <map_obj/ParentMovementMgr.h>
#include <collision/ActorBoxBgCollision.h>
#include <collision/ActorPolylineBgCollision.h>

namespace zap {

class MagicPlatform : public Actor {
    SEAD_RTTI_OVERRIDE(MagicPlatform, Actor)

public:
    enum CollisionType {
        cCollisionType_Solid,
        cCollisionType_Semisolid,
        cCollisionType_None,
        
        cCollisionType_Max
    };

public:
    static Profile* sProfile;

    MagicPlatform(const ActorCreateParam& param);
    ~MagicPlatform() override = default;
    
    Result create() override;
    bool execute() override;
    bool draw() override;

    void setupMovement(const sead::Vector3f& position, u32 movement_mask, ParentMovementType movement_type, u32 movement_id);
    void setMovementParamaters(ParentMovementType movement_type);

    static void callbackFoot(BgCollision* bc_self, ActorBgCollisionCheck* cc_other);
    static void callbackHead(BgCollision* bc_self, ActorBgCollisionCheck* cc_other);
    static void callbackWall(BgCollision* bc_self, ActorBgCollisionCheck* cc_other, u8 direction); 
    void callbackGeneral(MagicPlatform* self, PlayerObject* other);
    
    static const ActorCreateInfo cCreateInfo;
    
private:
    u16* mTileData;
    sead::Vector2u mTileSize;
    ParentMovementMgr mMovementMgr;
    u8 mCollisionType;
    u8 mDamageType;
    ActorBoxBgCollision mSolidCollider;
    ActorPolylineBgCollision<1> mSemisolidCollider;
};

} // namespace zap
