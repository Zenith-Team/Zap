#include <zap/actor/FrozenDonutBlock.h>
#include <zap/Zap.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::FrozenDonutBlock, zap::ActorDonutBlock)

const ActorCreateInfo zap::FrozenDonutBlock::cCreateInfo = {
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

Profile* zap::FrozenDonutBlock::sProfile = zap::getRegistrar()->newProfile<zap::FrozenDonutBlock>("frozen_donut_block")
    .resources<"obj_frozend_block", "obj_frozdn2_block", "obj_frozdn3_block">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .createInfo(&cCreateInfo)
    .build();

zap::FrozenDonutBlock::FrozenDonutBlock(const ActorCreateParam& param)
    : zap::ActorDonutBlock(param)
{ }

ActorBase::Result zap::FrozenDonutBlock::create() {
    Result result = zap::ActorDonutBlock::create();
    
    mCollider.setAttr(BgUnitCode::cIce);
    
    return result;
}

void zap::FrozenDonutBlock::loadActorRes() {
    static constexpr sead::SafeArray<const char*, 3> cResources = {
        "obj_frozend_block",
        "obj_frozdn2_block",
        "obj_frozdn3_block"
    };
    
    mModel = AnimModel::create(cResources[mLength], cResources[mLength], 2, 2, 2, 2, 2);
    mTexAnim = mModel->getTexAnim(0);
    mTexAnim->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
    mTexAnim->getFrameCtrl().setFrame(0.0f);
}
