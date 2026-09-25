#include <zap/actor/ActorDonutBlock.h>
#include <zap/Zap.h>
#include <red/utility/SpriteUtil.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::ActorDonutBlock, red::ActorDonutBlock)

const ActorCreateInfo zap::ActorDonutBlock::cCreateInfo = {
    .offset_x = 8, .offset_y = -16,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = {
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_None
};

Profile* zap::ActorDonutBlock::sProfile = zap::getRegistrar()->newProfile<zap::ActorDonutBlock>("donut_block")
    .resources<"obj_chikuwa_block", "obj_widedn2_block", "obj_widedn3_block">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .executePriority(232) // aligns visuals with collision
    .createInfo(cCreateInfo)
    .build();

zap::ActorDonutBlock::ActorDonutBlock(const ActorCreateParam& param)
    : red::ActorDonutBlock(param)
    , mLength(0)
{ }

ActorBase::Result zap::ActorDonutBlock::create() {
    // Setting: Length
    mLength = red::SpriteUtil::getNybble5(this);
    
    Result result = red::ActorDonutBlock::create();
    
    u8 lengthTiles = mLength + 1;
    
    mCollider.getPoints()[0].x = -8.0f * lengthTiles;
    mCollider.getPoints()[1].x = 8.0f * lengthTiles;
    
    mSize.multScalar(lengthTiles);
    mVisibleAreaSize.multScalar(lengthTiles);
    
    return result;
}

void zap::ActorDonutBlock::loadActorRes() {
    static constexpr sead::SafeArray<const char*, 3> cResources = {
        "obj_chikuwa_block",
        "obj_widedn2_block",
        "obj_widedn3_block"
    };
    
    mModel = AnimModel::create(cResources[mLength], cResources[mLength], 1, 1);
    mModel->playTexAnim(cResources[mLength]);
    mTexAnim = mModel->getTexAnim(0);
    mTexAnim->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
    mTexAnim->getFrameCtrl().setFrame(1.0f);
    mTexAnim->getFrameCtrl().setRate(0.0f);
}
