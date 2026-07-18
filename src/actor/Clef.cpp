#include <zap/Zap.h>
#include <zap/actor/Clef.h>
#include <zap/actor/Note.h>
#include <audio/GameAudio.h>
#include <actor/ActorMgr.h>
#include <map/SwitchFlagMgr.h>
#include <red/util/SpriteUtil.h>
#include <effect/EffectID.h>
#include <effect/EffectCreateUtil.h>
#include <event/EventMgr.h>
#include <player/PlayerMgr.h>
#include <player/PlayerObject.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::Clef, ActorMultiState);

CREATE_STATE_ID(zap::Clef, Waiting)
CREATE_STATE_ID(zap::Clef, GameActive)
CREATE_STATE_ID(zap::Clef, AnimateCollecting)
CREATE_STATE_ID(zap::Clef, AnimateAppear)

static constexpr f32 cScaleFactor = 0.17f;
static constexpr f32 cCollectAnimDuration = 9.0f; // frames
static constexpr f32 cCollectAnimTiles = 1.5f;

static constexpr sead::SafeArray<u16, 9> cRewards = { 591, 592, 594, 593, 595, 596, 598, 597, 599 };

const ActorCreateInfo zap::Clef::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .spawn_range = {
        .offset_x = 0, .offset_y = 0,
        .half_size_x = 8, .half_size_y = 8
    },
    .cull_range = {
        .up = 0, .down = 0, .left = 0, .right = 0
    },
    .flag = ActorCreateInfo::cFlag_MapObj
};

using CC = ActorCollisionCheck;
const CC::CollisionData zap::Clef::cCollisionData = {
    .center_offset = { 0.0f, 0.0f },
    .half_size = { 8.0f, 16.0f },
    .shape_type = CC::ActorCollisionCheck::cShapeType_Box,
    .kind = CC::cKind_Item, 
    .attack = CC::cAttack_None,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player
    ),
    .vs_damage = CC::cDamageFrom_All,
    .status = CC::cStatus_None,
    .callback = [](ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) {
        zap::Clef* self = cc_self->getOwner<zap::Clef>();
        if (self != nullptr) {
            self->collect(cc_other->getOwner<PlayerObject>()->getPlayerNo());
        }
    }
};

Profile* zap::Clef::sProfile = zap::getRegistrar()->newProfile<zap::Clef>("clef")
    .resources<"clef">(ProfileInfo::cResType_Course)
    .createInfo(&cCreateInfo)
    .flag(Profile::cFlag_DrawCullCheck)
    .build();

zap::Clef::Clef(const ActorCreateParam& param)
    : ActorMultiState(param)
    , mClefModel(nullptr)
    , mTime(0.0f)
    , mManagerID(0)
    , mRewardID(0)
    , mTimeLimit(0)
    , mScanAttempt(0)
    , mReady(false)
    , mAttemptsRemaining(0)
    , mTargetNoteCount(0)
    , mPhaseCount(0)
    , mCurrentPhaseTimer(0)
    , mActivePhaseID(0)
    , mCollectedNoteCount(0)
    , mStartCollectAnimTime(0)
    , mGameWonEventID(0)
{ }

ActorBase::Result zap::Clef::create() {
    // Model setup
    mClefModel = AnimModel::create("clef", "clef", 3, 0, 1);
    mClefModel->playTexSrtAnim("anim_color");
    
    // Positioning
    mScale = sead::Vector3f(cScaleFactor, cScaleFactor, cScaleFactor);

    // Movement setup
    const u8 nybble20 = red::SpriteUtil::getNybble20(this);
    if (nybble20 > cPos_KinokoLift) {
        tk::fatal("Movement type was out of bounds");
    }
    const ParentMovementType movementType = static_cast<ParentMovementType>(nybble20);
    u32 movementMask = mMovementHandler.getTypeMask(movementType);
    mMovementHandler.link(mPos, movementMask, mParamEx.course.movement_id); // nybble 21-22

    // Collision
    mCollisionCheck.set(this, cCollisionData);
    reviveCollisionCheck();

    // Members
    mTargetNoteCount = (red::SpriteUtil::getNybble3(this) << 4) | red::SpriteUtil::getNybble4(this);
    
    mPhaseCount = red::SpriteUtil::getNybble8(this);
    if (mPhaseCount > cPhaseLimit) {
        tk::fatal("Too many phases, please limit to 8");
    } else if (mPhaseCount == 0) {
        tk::fatal("Clef phase count starts at 1");
    }

    mTimeLimit = (red::SpriteUtil::getNybble6(this) << 4) | red::SpriteUtil::getNybble7(this);
    mManagerID = (red::SpriteUtil::getNybble1(this) << 4) | red::SpriteUtil::getNybble2(this);
    mRewardID = red::SpriteUtil::getNybble5(this);
    mGameWonEventID = (red::SpriteUtil::getNybble10(this) << 4) | red::SpriteUtil::getNybble11(this);
    mAttemptsRemaining = red::SpriteUtil::getNybble9(this);

    changeState(StateID_Waiting);
    
    updateModel();
    
    return cResult_Success;
}

void zap::Clef::scanNotes() {
    mScanAttempt++;
    if (mScanAttempt >= 4) {
        tk::fatal("Couldnt find notes after 3 attempts...");
    }
    
    // find notes:
    u32 foundNoteCount[cPhaseLimit] = { 0 };
    ActorMgr* actorMgr = ActorMgr::instance();
    for (auto it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        if (Note* note = sead::DynamicCast<Note>(*it)) {
            if (note->getManagerID() == mManagerID) {
                const u8 phase = note->getPhaseID();
                if (phase >= mPhaseCount) {
                    tk::fatal("Note phase was outside the Clef phase count");
                    return;
                }
                foundNoteCount[phase]++;
                //tk::println("Found note %u of %u for phase %u of %u", foundNoteCount[phase], mTargetNoteCount, phase + 1, mPhaseCount);
            }
        }
    }
    
    // verification
    for (u32 i = 0; i < mPhaseCount; i++) {
        if (foundNoteCount[i] > mTargetNoteCount) {
            tk::fatal("Too many notes found");
            return;
        }
        
        if (foundNoteCount[i] != mTargetNoteCount) {
            tk::println("Couldn't find all notes!: found only %u notes in phase %u", foundNoteCount[i], i);
            return;
        }
    }
    
    // Allocate notes
    for (u32 i = 0; i < mPhaseCount; i++) {
        nNotes[i] = new(mActorHeap) ActorUniqueID[mTargetNoteCount];
    }
    
    u32 storedIndex[cPhaseLimit] =  { 0 };
    for (auto it = actorMgr->getActorBegin(); it != actorMgr->getActorEnd(); it++) {
        if (Note* note = sead::DynamicCast<Note>(*it)) {
            if (note->getManagerID() == mManagerID) {
                note->setParent(mActorUniqueID); // set the parent to this clef instance
                const u8 phase = note->getPhaseID();
                if (phase >= mPhaseCount) {
                    tk::fatal("Note phase was outside the Clef phase count");
                    return;
                }
                if (storedIndex[phase] >= mTargetNoteCount) {
                    tk::fatal("Too many notes in phase");
                    return;
                }
                
                nNotes[phase][storedIndex[phase]] = note->getActorUniqueID();
                storedIndex[phase]++;
            }
        }
    }
    
    // all notes found!
    // tk::println("Target note count per round: %u", mTargetNoteCount);
    // for (u32 i = 0; i < cPhaseLimit; i++) {
    //     tk::println("Phase %u found notes: %u", i, foundNoteCount[i]);
    // }
    mReady = true;
}

bool zap::Clef::execute() {
    if (!mReady) {
        scanNotes();
    }
    
    mTime++;

    executeState();

    return true;
}

bool zap::Clef::draw() {
    if (!isState(StateID_GameActive))
        mClefModel->draw();

    return true;
}

void zap::Clef::updateModel() {
    mClefModel->update(mPos, mAngle, mScale);
}

void zap::Clef::collect(s8 playerNo) {
    if (!mReady) 
        return;

    mCollectedPlayer = playerNo;
    GameAudio::getAudioObjMap()->startSound("SE_BOSS_WENDY_RING_BOUND", mPos);

    removeCollisionCheck();

    changeState(StateID_AnimateCollecting);

    // tk::println("Game started!");
} 

void zap::Clef::setCurrentNotesState(const StateID& state) const {
    if (mActivePhaseID >= mPhaseCount) {
        return;
    }
    
    for (u32 i = 0; i < mTargetNoteCount; i++) {
        ActorUniqueID uniqueID = nNotes[mActivePhaseID][i];
        ActorBase* actorPtr = ActorMgr::instance()->getActorPtr(uniqueID);
        if (actorPtr != nullptr) {
            Note* note = static_cast<Note*>(actorPtr);
            if (!note->isCollected() || state == Note::StateID_Active) {
                note->changeState(state);
            }
        }
    }
}

void zap::Clef::setAllNotesState(const StateID& state) const {
    for (u32 phase = 0; phase < mPhaseCount; phase++) {
        for (u32 i = 0; i < mTargetNoteCount; i++) {
            ActorUniqueID uniqueID = nNotes[phase][i];
            ActorBase* actorPtr = ActorMgr::instance()->getActorPtr(uniqueID);
            if (actorPtr != nullptr) {
                Note* note = static_cast<Note*>(actorPtr);
                note->changeState(state);
            }
        }
    }
}

void zap::Clef::noteCollected(Note* note) {
    if (!isState(StateID_GameActive))
        return;

    mCollectedNoteCount++;

    // collect sfx
    // TODO: pitch/note change
    GameAudio::getAudioObjMap()->startSound("SE_OBJ_COIN_BOUND", note->getPos());

    if (mCollectedNoteCount >= mTargetNoteCount) {
        
        // reset state variables
        mCollectedNoteCount = 0;
        mActivePhaseID++;
        mCurrentPhaseTimer = 0;
        
        // reset timer on phase switch
        
        if (mActivePhaseID >= mPhaseCount) {
            // end game
            // give powerup reward
            
            if (mRewardID != 0 && mRewardID <= 9) {
                ActorCreateParam info{};
                info.param_0 = 0x6000000; // set "Reward" spawn mode for item profile (nybble 6)
                info.profile = Profile::get(cRewards[mRewardID - 1]);
                
                ActorMgr::instance()->createImmediately(info); // 0 index
            }
            
            for (u32 phase = 0; phase < mPhaseCount; phase++) {
                for (u32 i = 0; i < mTargetNoteCount; i++) {
                    ActorUniqueID uniqueID = nNotes[phase][i];
                    ActorBase* actorPtr = ActorMgr::instance()->getActorPtr(uniqueID);
                    if (actorPtr != nullptr) {
                        Note* n = static_cast<Note*>(actorPtr);

                        // Only delete actors which are already done rendering so deletion isnt visible
                        if (n->isState(Note::StateID_Idle)) {
                            n->deleteActor(true);
                        }
                    }
                }
            }
            
            // Clapping
            GameAudio::setClapSE();
            // Win sfx
            GameAudio::getAudioObjMap()->startSound("SE_SYS_CH_TARGET_MEDAL_UP", note->getPos());
            
            // Activate the event
            SwitchFlagMgr::instance()->set(mGameWonEventID - 1, 0, true);

            removeCollisionCheck();
            deleteActor(true);
            return;
        } else {
            // next phase
            setCurrentNotesState(Note::StateID_Active);
            // TODO: play the freeze frame again...?
        }
    }
}

/** STATE: Waiting */

void zap::Clef::initializeState_Waiting() { 
    reviveCollisionCheck();
}

void zap::Clef::executeState_Waiting() { 
    updateModel();

    mEffect1.setAlpha(0.4f);
    mEffect1.createEffect(RP_Mario_Star_3, &mPos, nullptr/*, &effectScale*/);

    // using a movement controller
    if (mParamEx.course.movement_id != 0) { 
        // TODO: continue following movement handler in idle state so its in the right pos on re-activation
        mMovementHandler.execute();
        mPos = mMovementHandler.getPosition();
    } else {
        // bobbing
        f32 yOffset = sead::Mathf::sin(mTime * (1.0f / 45.0f)) * 0.05;
        mPos.y += yOffset;
    }
}

void zap::Clef::finalizeState_Waiting() { 
    removeCollisionCheck();
}

/** STATE: GameActive */

void zap::Clef::initializeState_GameActive() { }

void zap::Clef::executeState_GameActive() { 
    mCurrentPhaseTimer++;
    
    // Warning
    // time (frames) > ((time limit (seconds) * 60 = frames) * 0.75 early warning)
    const u32 warningTime = static_cast<u32>((mTimeLimit * 60) * 0.75f);
    if (mCurrentPhaseTimer == warningTime) { // auto-determined "warning" time calculated from the time limit nybble
        // just send signal to all notes (set a member) then the notes will flash
        setCurrentNotesState(Note::StateID_AnimateExpiry);

        const PlayerObject* collectedPlayer = PlayerMgr::instance()->getPlayerObject(mCollectedPlayer);
        if (collectedPlayer != nullptr) {
            GameAudio::getAudioObjMap()->startSound("SE_SYS_GREEN_COIN_TIMER1", collectedPlayer->getPos());
        } else {
            GameAudio::getAudioObjMap()->startSound("SE_SYS_GREEN_COIN_TIMER1", mPos);
        }
    }
    
    if (mCurrentPhaseTimer == (mTimeLimit * 60)) {
        // game over
        setCurrentNotesState(Note::StateID_AnimateDisappear);

        if (mAttemptsRemaining == 0) {
            // kill all notes
            for (u32 phase = 0; phase < mPhaseCount; phase++) {
                for (u32 i = 0; i < mTargetNoteCount; i++) {
                    ActorUniqueID uniqueID = nNotes[phase][i];
                    ActorBase* actorPtr = ActorMgr::instance()->getActorPtr(uniqueID);
                    if (actorPtr != nullptr) {
                        Note* n = static_cast<Note*>(actorPtr);

                        if (n->isState(Note::StateID_Idle)) {
                            n->deleteActor(true);
                        }
                    }
                }
            }

            const PlayerObject* collectedPlayer = PlayerMgr::instance()->getPlayerObject(mCollectedPlayer);
            if (collectedPlayer != nullptr) {
                GameAudio::getAudioObjMap()->startSound("SE_SYS_GREEN_COIN_TIMER3", collectedPlayer->getPos());
            } else {
                GameAudio::getAudioObjMap()->startSound("SE_SYS_GREEN_COIN_TIMER3", mPos);
            }
            
            // kill self
            removeCollisionCheck();

            deleteActor(true);
        } else {
            mAttemptsRemaining--;

            GameAudio::getAudioObjMap()->startSound("SE_BOSS_WENDY_RING_DISAPP", mPos);

            // Spawn a sfx at the player's position
            const PlayerObject* collectedPlayer = PlayerMgr::instance()->getPlayerObject(mCollectedPlayer);
            if (collectedPlayer != nullptr) {
                GameAudio::getAudioObjMap()->startSound("SE_SYS_GREEN_COIN_TIMER3", collectedPlayer->getPos());
            } else {
                GameAudio::getAudioObjMap()->startSound("SE_SYS_GREEN_COIN_TIMER3", mPos);
            }

            changeState(StateID_AnimateAppear); 

            // reset members
            mTime = 0;
            mCurrentPhaseTimer = 0;
            mActivePhaseID = 0;
            mCollectedNoteCount = 0;
        }
    }
}

void zap::Clef::finalizeState_GameActive() { }

/** STATE: AnimateCollecting */

void zap::Clef::initializeState_AnimateCollecting() { 
    // summon phase notes
    setCurrentNotesState(Note::StateID_Active);

    mClefModel->playSklAnim("Got");
    mClefModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);

    // freeze the game for x seconds
    mFreezeEvent.freeze();
    EventMgr::instance()->pushEvent(&mFreezeEvent);

    mStartCollectAnimTime = 0;
}

void zap::Clef::executeState_AnimateCollecting() { 
    updateModel();

    mStartCollectAnimTime++;

    if (mClefModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        if (mStartCollectAnimTime >= 115) {
            changeState(StateID_GameActive);
        }
    }
}

void zap::Clef::finalizeState_AnimateCollecting() { 
    mFreezeEvent.unfreeze();
}

/** STATE: AnimateAppear */

void zap::Clef::initializeState_AnimateAppear() { 
    mClefModel->playSklAnim("Appear");
    mClefModel->getSklAnim(0)->getFrameCtrl().setPlayMode(FrameCtrl::cMode_NoRepeat);
}

void zap::Clef::executeState_AnimateAppear() { 
    updateModel();

    if (mClefModel->getSklAnim(0)->getFrameCtrl().isStop()) {
        changeState(StateID_Waiting);
    }
}

void zap::Clef::finalizeState_AnimateAppear() {
    // reset notes
    setAllNotesState(Note::StateID_Idle);
}
