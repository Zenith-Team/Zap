#include <zap/actor/ActorSpawnerEx.h>
#include <actor/ActorMgr.h>
#include <actor/MapActor.h>
#include <map/SwitchFlagMgr.h>
#include <red/utility/SpriteUtil.h>
#include <zap/Zap.h>
#include <red/event/ResourceLoadEvent.h>
#include <map/CourseData.h>
#include <game_info/CourseInfo.h>
#include <red/profile/MapActorMgr.h>
#include <zap/actor/StringBank.h>
#include <zap/actor/NybbleBank.h>
#include <map/LayerID.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::ActorSpawnerEx, Actor)

const ActorCreateInfo zap::ActorSpawnerEx::cCreateInfo = {
    .offset_x = 0, .offset_y = 0,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 0, .half_size_y = 0
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_IgnoreSpawnRange
};

Profile* zap::ActorSpawnerEx::sProfile = zap::getRegistrar()->newProfile<zap::ActorSpawnerEx>("actor_spawner_ex")
    .createInfo(cCreateInfo)
    .build();

zap::ActorSpawnerEx::ActorSpawnerEx(const ActorCreateParam& param)
    : Actor(param)
    , mSpawnProfileID(0xFFFF)
    , mSpawnEventID(0)
    , mSpawned(false)
    , mPrevFrameEvent(false)
    , mStringScanAttempt(0)
    , mNybbleScanAttempt(0)
    , mStartPos(mPos)
{ }

ActorBase::Result zap::ActorSpawnerEx::create() {
    mSpawnEventID = red::SpriteUtil::getNybbleRange(this, 1, 2);
    if (mSpawnEventID == 0) {
        tk::fatal("ActorSpawnerEx: Event ID was unset.");
        return cResult_Failed;
    }
    mSpawnEventID -= 1;  // events are 0-indexed

    // Setting: Usage Mode
    mSpawnerMode = static_cast<SpawnerMode>(red::SpriteUtil::getNybble9(this));

    // Setting: ID type
    switch (red::SpriteUtil::getNybble5(this)) {
        case 0: { // Sprite ID
            mSpawnProfileID = MapActor::cProfileID[red::SpriteUtil::getNybbleRange(this, 6, 8)];
            break;
        }
        
        case 1: { // Profile ID
            mSpawnProfileID = red::SpriteUtil::getNybbleRange(this, 6, 8);
            break;
        }
        
        default: { // String ID
            // We must perform some scans to find the string bank and then lookup the ID from that. Do it in execute() instead.
            break;
        }
    }

    execute();

    return cResult_Success;
}

bool zap::ActorSpawnerEx::execute() {
    if (mSpawnProfileID == 0xFFFF) {
        if (!scanString()) {
            return true;
        }
    }
    
    if (!mNybbleBank.isValid() && mNybbleScanAttempt < 6) {
        if (!scanNybble()) {
            return true;
        }
    }
    
    if (mSpawned && mSpawnerMode == cSpawnerMode_SingleUse)
        return true;

    bool event = SwitchFlagMgr::instance()->isActivated(mSpawnEventID);
    
    if (event && !mPrevFrameEvent) {
        ActorCreateParam child;
        child.profile = red::ProfileEx::get(mSpawnProfileID);

        const Actor* nybbleBank = ActorMgr::instance()->getActorPtr<Actor>(mNybbleBank);
        
        child.param_0 = mNybbleBank.isValid() ? nybbleBank->getParam0() : 0;
        child.param_1 = mNybbleBank.isValid() ? nybbleBank->getParam1() : 0;
        child.param_ex_0.course.switch_flag_0 = mNybbleBank.isValid() ? nybbleBank->getSwitchFlag0() : 0;
        child.param_ex_0.course.switch_flag_1 = mNybbleBank.isValid() ? nybbleBank->getSwitchFlag1() : 0;
        child.param_ex_0.course.layer = mNybbleBank.isValid() ? nybbleBank->getLayer() : getLayer();
        child.param_ex_1.course.movement_id = mNybbleBank.isValid() ? nybbleBank->getParamEx().course.movement_id : 0;
        child.param_ex_1.course.link_id = mNybbleBank.isValid() ? nybbleBank->getParamEx().course.link_id : 0;
        child.param_ex_1.course.init_state_flag = mParamEx.course.init_state_flag; // Nybble bank uses the initial state as a bank ID, use the spawner initial state

        const ActorCreateInfo& info = child.profile->getActorCreateInfo();
        // Handle Z offset for spawning on other layers
        // Setup copied from ActorCreateMgr::spawnSpriteActor
        float zOffset = 0.0f;
        if (info.flag & ActorCreateInfo::cFlag_MapObj) {
            if (child.param_ex_0.course.layer == LayerID::cLayerID_Layer1) {
                zOffset = 400.0f;
            } else {
                zOffset = -3100.0f;
            }
        } else {
            if (child.param_ex_0.course.layer == LayerID::cLayerID_Layer1) {
                zOffset = 2200.0f;
            } else {
                zOffset = -2500.0f;
            }
        }
        child.position = mPos + sead::Vector3f(info.offset_x, info.offset_y, zOffset); // account for spawn offs
        
        mChildActor = ActorMgr::instance()->createImmediately(child)->getActorUniqueID();
        mSpawned = true;
    }

    if (!event && mPrevFrameEvent) {
        // If set to linked, delete the spawned actor on event deactivation
        ActorMgr* actorMgr = ActorMgr::instance();
        if (mSpawnerMode == cSpawnerMode_Linked && mSpawned) {
            for (auto it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
                if (Actor* actor = sead::DynamicCast<Actor>(*it)) {
                    if (actor->getActorUniqueID() == mChildActor) {
                        actor->deleteRequest();
                        mSpawned = false;
                    }
                }
            }
        }
    }
    
    mPrevFrameEvent = event;

    return true;
}

bool zap::ActorSpawnerEx::scanString() {
    // Setting: String Bank ID
    const u8 stringBankID = red::SpriteUtil::getNybbleRange(this, 7, 8);
    
    mStringScanAttempt++;
    if (mStringScanAttempt > 5) {
        tk::fatal("ActorSpawnerEx: Failed to find String Bank with ID %i after 5 attempts...", stringBankID);
        return false;
    }
    
    ActorMgr* actorMgr = ActorMgr::instance();
    
    for (auto it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        if (StringBank* bank = sead::DynamicCast<StringBank>(*it)) {
            if (bank->getBankID() == stringBankID && bank->getType() == StringBank::Type::Primary) {
                mSpawnProfileID = red::ProfileEx::get(bank->getString())->getID();
                return true;
            }
        }
    }
    
    return false;
}

bool zap::ActorSpawnerEx::scanNybble() {
    // Setting: Nybble Bank ID
    const u8 nybbleBankID = red::SpriteUtil::getNybbleRange(this, 10, 11);

    if (mNybbleBank.isValid())
        return true;

    mNybbleScanAttempt++;
    if (mNybbleScanAttempt > 5) {
        tk::println("ActorSpawnerEx: Failed to find Nybble Bank with ID %i after 5 attempts, falling back to all zero values...", nybbleBankID);
        return true;
    }

    ActorMgr* actorMgr = ActorMgr::instance();
    
    for (auto it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        if (NybbleBank* bank = sead::DynamicCast<NybbleBank>(*it)) {
            if (bank->getParamEx().course.init_state_flag == nybbleBankID) {
                mNybbleBank = bank->getActorUniqueID();
                tk::println("Found nybble bank");
                return true;
            }
        }
    }
    
    tk::println("Nybble bank not found");
    return false;
}

//! TODO: Preload resources, but it's more complicated here due to having to also scan/link stringbank chains
