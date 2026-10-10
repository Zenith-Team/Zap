#include <zap/actor/ActorSpawnerSimple.h>
#include <actor/ActorMgr.h>
#include <actor/MapActor.h>
#include <map/SwitchFlagMgr.h>
#include <red/utility/SpriteUtil.h>
#include <zap/Zap.h>
#include <red/event/ResourceLoadEvent.h>
#include <map/CourseData.h>
#include <game_info/CourseInfo.h>
#include <red/profile/MapActorMgr.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::ActorSpawnerSimple, Actor)

const ActorCreateInfo zap::ActorSpawnerSimple::cCreateInfo = {
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

Profile* zap::ActorSpawnerSimple::sProfile = zap::getRegistrar()->newProfile<zap::ActorSpawnerSimple>("actor_spawner_simple")
    .createInfo(cCreateInfo)
    .build();

zap::ActorSpawnerSimple::ActorSpawnerSimple(const ActorCreateParam& param)
    : Actor(param)
    , mSpawnProfileID(0)
    , mSpawnEventID(0)
    , mSpawned(false)
    , mPrevFrameEvent(false)
{ }

ActorBase::Result zap::ActorSpawnerSimple::create() {
    mSpawnEventID = mParamEx.course.init_state_flag;
    if (mSpawnEventID == 0) {
        tk::fatal("ActorSpawnerSimple: Event ID was unset.");
        return cResult_Failed;
    }
    mSpawnEventID -= 1;  // events are 0-indexed

    s32 id = red::SpriteUtil::getNybbleRange(this, 22, 24);

    // Setting: Multi-use
    mMultiUse = red::SpriteUtil::getBitRange(this, 0, 1);

    // Setting: ID type
    if (!red::SpriteUtil::getBitRange(this, 1, 2)) {
        id = MapActor::cProfileID[id];
    }

    mSpawnProfileID = id;

    execute();

    return cResult_Success;
}

bool zap::ActorSpawnerSimple::execute() {
    if (mSpawned && !mMultiUse)
        return true;

    bool event = SwitchFlagMgr::instance()->isActivated(mSpawnEventID);
    
    if (event && !mPrevFrameEvent) {
        ActorCreateParam child;
        child.profile = red::ProfileEx::get(mSpawnProfileID);

        const ActorCreateInfo& info = child.profile->getActorCreateInfo();
        child.position = mPos + sead::Vector3f(info.offset_x, info.offset_y, 0.0f); // account for spawn offs

        // nybble 1 is inaccessible
        
        child.param_ex_0.course.switch_flag_1 = red::SpriteUtil::getNybble2(this);
        child.param_ex_0.course.switch_flag_0 |= (red::SpriteUtil::getNybble3(this) << 0x04);
        child.param_ex_0.course.switch_flag_0 |= red::SpriteUtil::getNybble4(this);
        child.param_0 = mParam0; // nybbles 5-12
        child.param_1 = mParam1; // nybbles 13-20
        child.param_ex_1.course.movement_id |= red::SpriteUtil::getNybble21(this) << 0x04;
        child.param_ex_1.course.movement_id |= mLayer & 0b00001111; // nybble 22
        child.param_ex_1.course.link_id = (mLayer & 0b11110000) << 0x04; // nybble 23
        
        ActorMgr::instance()->createImmediately(child);
        mSpawned = true;
    }
    
    mPrevFrameEvent = event;

    return true;
}

using namespace red;

namespace zap {
    ResourceLoadEvent::Listener<ResourceLoadEvent::Stage::Course> LoadActorSpawnerSimpleResources([](ResourceLoadEvent& e) {
        const u16 cActorSpawnerSimpleMapActor = MapActorMgr::instance()->profToMap(ActorSpawnerSimple::sProfile->getID());
        if (cActorSpawnerSimpleMapActor == 0xFFFF) {
            return; // doesn't exist in this level
        }
        
        const CourseDataFile* file = CourseData::instance()->getFile(CourseInfo::instance()->getFileNo());
        const MapActorData* actorSpawner = nullptr;
        while ((actorSpawner = file->getMapActor(cActorSpawnerSimpleMapActor, actorSpawner)) != nullptr) {
            s32 id = (actorSpawner->movement_id & 0xF) << 0x08 | (actorSpawner->link_id);
            
            if (actorSpawner->switch_flags & 0x8000) { // uses sprite id
                id = MapActor::cProfileID[id];
            }
            
            Profile* spawnedProfile = ProfileEx::get(id);
            if (spawnedProfile == nullptr) {
                tk::fatal("ActorSpawnerSimple profileID %i was not found", id);
            }
            
            spawnedProfile->loadResource(e.getHeap());
        }
    });
}
