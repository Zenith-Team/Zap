#include <zap/Zap.h>
#include <zap/actor/Clef.h>
#include <telkin/Print.h>
#include <zap/actor/Note.h>
#include <actor/ActorMgr.h>
#include <red/util/SpriteUtil.h>
#include <input/InputMgr.h>
#include <player/PlayerObject.h>
#include <effect/EffectCreateUtil.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::Note, ActorMultiState)

CREATE_STATE_ID(zap::Note, Idle)
CREATE_STATE_ID(zap::Note, Active)
CREATE_STATE_ID(zap::Note, AnimateCollecting)
CREATE_STATE_ID(zap::Note, AnimateAppear)
CREATE_STATE_ID(zap::Note, AnimateDisappear)
CREATE_STATE_ID(zap::Note, AnimateExpiry)

static constexpr f32 cScaleFactor = 0.17f;

const ActorCreateInfo zap::Note::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = { 
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_MapObj | ActorCreateInfo::cFlag_IgnoreSpawnRange
};

using CC = ActorCollisionCheck;
const CC::CollisionData zap::Note::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 10.0f, 10.0f },
    .shape_type = CC::ActorCollisionCheck::cShapeType_Box,
    .kind = CC::cKind_Enemy,
    .attack = CC::cAttack_None,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player
    ),
    .vs_damage = CC::cDamageFrom_All,
    .status = CC::cStatus_None,
    .callback = [](ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) { 
        zap::Note* self = cc_self->getOwner<zap::Note>();
        if (self != nullptr) {
            self->collect(cc_other->getOwner<PlayerObject>()->getPlayerNo());
        }
    }
};

Profile* zap::Note::sProfile = zap::getRegistrar()->newProfile<zap::Note>("note")
    .resources<"note">(ProfileInfo::cResType_Course)
    .flag(Profile::cFlag_DrawCullCheck)
    .createInfo(&cCreateInfo)
    .build();

zap::Note::Note(const ActorCreateParam& param)
    : ActorMultiState(param)
    , mModel(nullptr)
    , mClefParent()
    , mManagerID()
    , mPhaseID()
    , mCollected(false)
    , mWarnTime(0)
{ }

ActorBase::Result zap::Note::create() {
    mModel = AnimModel::create("note", "note", 4, 0, 1);
    mModel->playTexSrtAnim("anim_color");
    
    // Setting: Randomize Texture
    if (red::SpriteUtil::getNybble4(this)) {
        mModel->getShuAnim(0)->getFrameCtrl().setFrame(InputMgr::instance()->getRandom().getF32Range(0.0f, mModel->getShuAnim(0)->getFrameCtrl().getFrameEnd()));
    }
    
    mScale = sead::Vector3f(cScaleFactor, cScaleFactor, cScaleFactor);

    mCollisionCheck.set(this, cCollisionData);
    
    // Setting: Manager ID
    mManagerID = (red::SpriteUtil::getNybble1(this) << 4) | red::SpriteUtil::getNybble2(this);

    // Setting: Phase ID
    mPhaseID = red::SpriteUtil::getNybble3(this);
    if (mPhaseID > Clef::cPhaseLimit) {
        tk::fatal("Phase ID was too high");
    }
    
    // Movement setup
    const u8 nybble20 = red::SpriteUtil::getNybble20(this);
    if (nybble20 > cPos_KinokoLift) {
        tk::fatal("Movement type was out of bounds");
    }
    const ParentMovementType movementType = static_cast<ParentMovementType>(nybble20);
    u32 movementMask = mMovementHandler.getTypeMask(movementType);
    mMovementHandler.link(mPos, movementMask, mParamEx.course.movement_id); // nybble 21-22

    changeState(StateID_Idle);
    
    updateModel();

    return cResult_Success;
}

bool zap::Note::execute() {
    mMovementHandler.execute();
    mPos = mMovementHandler.getPosition();

    executeState();
    
    return true;
}

bool zap::Note::draw() {
    mModel->draw();

    return true;
}

void zap::Note::updateModel() {
    mModel->update(mPos, mAngle, mScale);
}

void zap::Note::collect(s8 playerNo) {
    if (mCollected || (!isState(StateID_Active) && !isState(StateID_AnimateExpiry))) {
        return;
    }

    ActorBase* parent = ActorMgr::instance()->getActorPtr(mClefParent);
    if (parent == nullptr) {
        tk::println("Note: failed to notify parent about collection.");
        return;
    }

    Clef* clef = static_cast<Clef*>(parent);
    if (!clef->isState(Clef::StateID_GameActive)) {
        return;
    }

    mCollected = true;
    changeState(StateID_AnimateCollecting);

    clef->noteCollected(this);
}

void zap::Note::reset() { }

/** STATE: Idle */

void zap::Note::initializeState_Idle() {
    mIsDrawEnable = false;
}

void zap::Note::executeState_Idle() { }

void zap::Note::finalizeState_Idle() { }

/** STATE: Active */

void zap::Note::initializeState_Active() { 
    // Note activated

    reviveCollisionCheck();

    mModel->playSklAnim("Wait");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_Repeat);
    
    mCollected = false;

    updateModel();
    mIsDrawEnable = true;
}

void zap::Note::executeState_Active() { 
    updateModel();
}

void zap::Note::finalizeState_Active() { 
    removeCollisionCheck();
}

/** STATE: AnimateCollecting */

void zap::Note::initializeState_AnimateCollecting() { 
    mModel->playSklAnim("Got");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void zap::Note::executeState_AnimateCollecting() { 
    updateModel();

    if (mModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        changeState(StateID_Idle);
    }
}

void zap::Note::finalizeState_AnimateCollecting() { }

/** STATE: AnimateAppear */
// Use this state to set the note to Active.
void zap::Note::initializeState_AnimateAppear() { 
    mModel->playSklAnim("Appear");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void zap::Note::executeState_AnimateAppear() { 
    updateModel();
    if (mModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        changeState(StateID_Active);
    }
}

void zap::Note::finalizeState_AnimateAppear() { }


/** STATE: AnimateDisappear */
// Use this state to set the note to Idle.
void zap::Note::initializeState_AnimateDisappear() { 
    mModel->playSklAnim("Disappear");
    mModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void zap::Note::executeState_AnimateDisappear() { 
    updateModel();
    if (mModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        changeState(StateID_Idle);
    }
}

void zap::Note::finalizeState_AnimateDisappear() { }

/** STATE: AnimateExpiry */
void zap::Note::initializeState_AnimateExpiry() { 
    mWarnTime = 0;
    reviveCollisionCheck();
}

void zap::Note::executeState_AnimateExpiry() { 
    mWarnTime++;
    
    // every 20ms turn rendering on or off
    if (mWarnTime % 8 == 0 && !mCollected) {
        mIsDrawEnable = !mIsDrawEnable;
    }
    
    updateModel();
}

void zap::Note::finalizeState_AnimateExpiry() {
    removeCollisionCheck();
    mIsDrawEnable = true;
}
