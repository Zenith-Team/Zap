#include <zap/actor/MagicPlatform.h>
#include <zap/Zap.h>
#include <game_info/CourseInfo.h>
#include <map/CourseData.h>
#include <red/utility/SpriteUtil.h>
#include <map/Bg.h>
#include <collision/ActorBgCollisionMgr.h>
#include <graphics/Renderer.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::MagicPlatform, Actor)

using ACI = ActorCreateInfo;
const ActorCreateInfo zap::MagicPlatform::cCreateInfo = {
    .offset_x = 0, .offset_y = 0,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 0, .half_size_y = 0
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ACI::cFlag_IgnoreSpawnRange | ACI::cFlag_MapObj
};

Profile* zap::MagicPlatform::sProfile = zap::getRegistrar()->newProfile<zap::MagicPlatform>("magicplatform")
    .createInfo(cCreateInfo)
    .executePriority(232) // aligns visuals with collision
    .build();

zap::MagicPlatform::MagicPlatform(const ActorCreateParam& param)
    : Actor(param)
    , mTileData(nullptr)
    , mTileSize(0, 0)
{ }

ActorBase::Result zap::MagicPlatform::create() {
    // Movement setup
    const u8 nybble20 = red::SpriteUtil::getNybble20(this);
    if (nybble20 > cPos_KinokoLift) {
        tk::fatal("Movement type was out of bounds");
    }
    const ParentMovementType movementType = static_cast<ParentMovementType>(nybble20);
    u32 movementMask = mMovementMgr.getTypeMask(movementType);

    setupMovement(mPos, movementMask, movementType, mParamEx.course.movement_id);

    // Setting: Location ID
    u8 locationID = (red::SpriteUtil::getNybble11(this) << 4) | red::SpriteUtil::getNybble12(this);
    
    // find location
    const CourseDataFile* area = CourseData::instance()->getFile(CourseInfo::instance()->getFileNo());
    const Location* location = area->getLocation(nullptr, locationID);
    
    if (location == nullptr) {
        tk::fatal("MagicPlatform failed to get location");
        return cResult_Failed;
    }
    
    // init tile data
    u32 locX = location->offset.x & ~0xF;
    u32 locY = location->offset.y & ~0xF;
    mTileSize.x = (location->size.x + (location->offset.x & 0xF) + 0xF) / 16;
    mTileSize.y = (location->size.y + (location->offset.y & 0xF) + 0xF) / 16;
    
    if (mTileSize.x == 0 || mTileSize.y == 0) {
        tk::fatal("MagicPlatform failed to get tile size");
        return cResult_Failed;
    }
    
    // scan and copy tile data
    mTileData = new u16[mTileSize.x * mTileSize.y];
    for (u32 y = 0; y < mTileSize.y; y++) {
        for (u32 x = 0; x < mTileSize.x; x++) {
            u16* tile = Bg::getUnitCurrentCdFile(locX + x * 16, locY + y * 16, mLayer);
            mTileData[x + y * mTileSize.x] = tile ? *tile : 0;
        }
    }
    
    // Setting: Collision Type
    mCollisionType = red::SpriteUtil::getNybble10(this);
    if (mCollisionType >= cCollisionType_Max) {
        tk::fatal("MagicPlatform invalid collision type");
        return cResult_Failed;
    }
    
    // Setting: Collider Interaction Type
    const u8 interactionType = (red::SpriteUtil::getNybble7(this) << 4) | red::SpriteUtil::getNybble8(this);
    if (interactionType > BgCollision::cType_InvisibleBlock) {
        tk::fatal("MagicPlatform invalid interaction type");
        return cResult_Failed;
    }
    
    // Setting: Collider Surface Type
    const u8 surfaceType = red::SpriteUtil::getNybble9(this);
    if (surfaceType > BgUnitCode::cCarpet) {
        tk::fatal("MagicPlatform invalid surface type");
        return cResult_Failed;
    }

    const u8 damageType = red::SpriteUtil::getNybbleRange(this, 13, 14);
    if (damageType != 0 && damageType > PlayerBase::cDamageType_Num) {
        tk::fatal("MagicPlatform invalid damage type");
        return cResult_Failed;
    }
    
    // TODO! Make it crush the player
    switch (mCollisionType) {
        case cCollisionType_Solid: {
            mSolidCollider.set(this, {
                .pos_offset         = { 0.0f, 0.0f },
                .rot_pivot_offset   = { 0.0f, 0.0f },
                .left_top_offset    = { mTileSize.x * -8.0f, mTileSize.y *  8.0f },
                .right_under_offset = { mTileSize.x * 8.0f, mTileSize.y * -8.0f },
                .angle              = mAngle.z()
            });
            
            mSolidCollider.setType(static_cast<BgCollision::Type>(interactionType));
            mSolidCollider.setAttr(static_cast<BgUnitCode::Attr>(surfaceType));

            if (damageType != 0) {
                // Subtract one from damageType, the spritedata uses the 0 value to denote no damage, so therefore all the settings are offset by one.
                mDamageType = damageType - 1;
                mSolidCollider.setCallback(
                    &MagicPlatform::callbackFoot,
                    &MagicPlatform::callbackHead,
                    &MagicPlatform::callbackWall
                );
            }
            
            ActorBgCollisionMgr::instance()->entry(mSolidCollider);
            break;
        }
        
        case cCollisionType_Semisolid: {
            const sead::Vector2f points[2] = {
                { mTileSize.x * -8.0f, mTileSize.y * 8.0f },
                { mTileSize.x *  8.0f, mTileSize.y * 8.0f }
            };
            
            mSemisolidCollider.set(this, {
                .pos_offset       = { 0.0f, 0.0f },
                .rot_pivot_offset = { 0.0f, 0.0f },
                .points           = points,
                .angle            = mAngle.z()
            });
            
            mSemisolidCollider.setType(static_cast<BgCollision::Type>(interactionType));
            mSemisolidCollider.setAttr(static_cast<BgUnitCode::Attr>(surfaceType));
            
            if (damageType != 0) {
                // Subtract one from damageType, the spritedata uses the 0 value to denote no damage, so therefore all the settings are offset by one.
                mDamageType = damageType - 1;
                mSemisolidCollider.setCallback(
                    &MagicPlatform::callbackFoot,
                    &MagicPlatform::callbackHead,
                    &MagicPlatform::callbackWall
                );
            }

            ActorBgCollisionMgr::instance()->entry(mSemisolidCollider);
            
            break;
        }
    }
    
    return cResult_Success;
}

void zap::MagicPlatform::setupMovement(const sead::Vector3f& position, u32 movement_mask, ParentMovementType movement_type, u32 movement_id) {
    // use different link function if pivotal rotation, prevents glitches
    if (movement_type == ParentMovementType::cPos_CenterRotation) {
        ParentMovementMgr::PivotalRotationSettings pivotSettings;
        pivotSettings.position       = position;
        pivotSettings.movement_id    = movement_id;
        pivotSettings.movement_mask  = movement_mask;
        pivotSettings.pivot_center   = sead::Vector3f(0.0f, 0.0f, 0.0f);
        pivotSettings.upside_down    = red::SpriteUtil::getNybble19(this) & 0x1;
        pivotSettings.gyroscopic     = (red::SpriteUtil::getNybble19(this) >> 1) & 0x1;
        pivotSettings.tilted         = (red::SpriteUtil::getNybble19(this) >> 2) & 0x1;
        pivotSettings._21            = (red::SpriteUtil::getNybble19(this) >> 3) & 0x1;
        pivotSettings.movement_param = 1;

        mMovementMgr.linkPivotal2(pivotSettings);
    } else {
        mMovementMgr.link(position, movement_mask, movement_id);
    }

    // set type-specific data
    setMovementParamaters(movement_type);

    mMovementMgr.execute();
}

void zap::MagicPlatform::setMovementParamaters(ParentMovementType movement_type) {
    static sead::SafeArray<f32, 16> twoWayDistanceMultiplierArr {
        1.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f
    };
    static sead::SafeArray<f32, 16> boltMovementSpeedArr {
        1.0f, 0.25f, 0.5f, 0.75f, 0.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f
    };

    switch (movement_type) {
        case cPos_Screw: {
            mMovementMgr.setBoltSpeed(boltMovementSpeedArr[red::SpriteUtil::getNybble18(this)]);
            mMovementMgr.setBoltDirection(static_cast<DirType>(red::SpriteUtil::getNybble19(this)));
            break;
        }
        case cPos_GoAndCome: {
            mMovementMgr.setTwoWayDistanceMultiplier(twoWayDistanceMultiplierArr[red::SpriteUtil::getNybble18(this)] + (0.01f * red::SpriteUtil::getNybble19(this)));
            break;
        }
        case cPos_ShiftingPlatform: {
            mMovementMgr.setRectPlatformInfo(static_cast<RectPlatformInfo>(red::SpriteUtil::getNybble19(this)));
            break;
        }
        case cPos_FloorGyration: {
            mMovementMgr.setFloorGyrationAngle(0x1000000 * red::SpriteUtil::getNybbleRange(this, 17, 18));
            ParentMovementMgr::MovementProperties newproperty = mMovementMgr.getMovementProperties();
            newproperty.hill_distance_offset = -16.0f * red::SpriteUtil::getNybble19(this);
            mMovementMgr.setMovementProperties(newproperty);
            break;
        }
    }
}

bool zap::MagicPlatform::execute() {
    mMovementMgr.execute();
    mPos = mMovementMgr.getPosition();
    if (!mMovementMgr.getPivotalGyroscopic()) {
        mAngle.z() = mMovementMgr.getAngle();
    }
    
    switch (mCollisionType) {
        case cCollisionType_Solid: {
            mSolidCollider.setAngle(mAngle.z());
            mSolidCollider.execute();
            break;
        }
        
        case cCollisionType_Semisolid: {
            mSemisolidCollider.setAngle(mAngle.z());
            mSemisolidCollider.execute();
            break;
        }
    }
    
    return true;
}

bool zap::MagicPlatform::draw() {
    f32 angleSin, angleCos;
    sead::Mathf::sinCosIdx(&angleSin, &angleCos, mAngle.z());
    
    for (u32 y = 0; y < mTileSize.y; y++) {
        for (u32 x = 0; x < mTileSize.x; x++) {
            const s32 offsetX  = x * 16 - mTileSize.x * 8 + 8;
            const s32 offsetY  = y * 16 - mTileSize.y * 8 + 8;
            const f32 rotatedX =  offsetX * angleCos + offsetY * angleSin;
            const f32 rotatedY = -offsetX * angleSin + offsetY * angleCos;
            
            const sead::Vector3f drawPos(mPos.x + rotatedX, mPos.y - rotatedY, mPos.z);
            
            Renderer::instance()->drawActorBgUnit(static_cast<UnitID>(mTileData[y * mTileSize.x + x]), drawPos, mAngle.z(), mScale);
        }
    }
    
    return true;
}

void zap::MagicPlatform::callbackFoot(BgCollision* bc_self, ActorBgCollisionCheck* cc_other) {
    MagicPlatform* self = (MagicPlatform*)bc_self->getOwner();
    Actor* other = cc_other->getOwner();
    if (other->getKind() == cActorKind_Player || other->getKind() == cActorKind_Yoshi) {
        self->callbackGeneral(self, static_cast<PlayerObject*>(other));
    }
}

void zap::MagicPlatform::callbackHead(BgCollision* bc_self, ActorBgCollisionCheck* cc_other) {
    MagicPlatform* self = (MagicPlatform*)bc_self->getOwner();
    Actor* other = cc_other->getOwner();
    if (other->getKind() == cActorKind_Player || other->getKind() == cActorKind_Yoshi) {
        self->callbackGeneral(self, static_cast<PlayerObject*>(other));
    }
}

void zap::MagicPlatform::callbackWall(BgCollision* bc_self, ActorBgCollisionCheck* cc_other, u8 direction) {
    MagicPlatform* self = (MagicPlatform*)bc_self->getOwner();
    Actor* other = cc_other->getOwner();
    if (other->getKind() == cActorKind_Player || other->getKind() == cActorKind_Yoshi) {
        self->callbackGeneral(self, static_cast<PlayerObject*>(other));
    }
}

void zap::MagicPlatform::callbackGeneral(MagicPlatform* self, PlayerObject* other) {
    other->setDamage(self, static_cast<PlayerBase::DamageType>(self->mDamageType));
}
