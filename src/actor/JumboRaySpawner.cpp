#include <zap/actor/JumboRaySpawner.h>
#include <zap/Zap.h>
#include <red/util/SpriteUtil.h>
#include <actor/ActorMgr.h>
#include <zap/actor/JumboRay.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::JumboRaySpawner, Actor)

const ActorCreateInfo zap::JumboRaySpawner::cCreateInfo = {
    .offset_x = 0, .offset_y = 0,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 64, .half_size_y = 64
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_None
};

Profile* zap::JumboRaySpawner::sProfile = zap::getRegistrar()->newProfile<JumboRaySpawner>("jumborayspawner")
    .resources<"manjirou">(ProfileInfo::cResType_Course)
    .build();

zap::JumboRaySpawner::JumboRaySpawner(const ActorCreateParam& param)
    : Actor(param)
    , mTimer(0)
    , mInterval(0)
    , mLimit(0)
    , mSpawnedCount(0)
    , mTriggeringEventID(0)
{ }

ActorBase::Result zap::JumboRaySpawner::create() {
    // Setting: Interval
    mInterval = (red::SpriteUtil::getNybble1(this) << 4) | red::SpriteUtil::getNybble2(this);
    mInterval += 1; // minimum 1 second delay
    
    // Setting: Limit
    mLimit = red::SpriteUtil::getNybble3(this);
    
    // Setting: Triggering Event ID
    mTriggeringEventID = (red::SpriteUtil::getNybble21(this) << 4) | red::SpriteUtil::getNybble22(this);
    
    return cResult_Success;
}

bool zap::JumboRaySpawner::execute() {
    if (!isActive())
        return true;
    
    // Spawn one immediately to start
    if (isActive() && mTimer == 0) {
        spawnRay();
        mTimer++;
    }
        
    mTimer++;
    
    if (mTimer >= mInterval * 60 + 1) {
        // Spawn one and reset the timer
        mTimer = 1;
        spawnRay();
    }
    
    return true;
}

void zap::JumboRaySpawner::spawnRay() {
    if (mLimit != 0 && mSpawnedCount >= mLimit) {
        // we have a limit and exhausted it.
        deleteRequest();
    }
    
    mSpawnedCount++;
    
    ActorCreateParam ray;
    ray.profile = JumboRay::sProfile;
    ray.position = mPos;
    
    // forward our settings
    ray.param_0 = mParam0;
    ray.param_1 = mParam1;
    
    ActorMgr::instance()->createImmediately(ray);
}
