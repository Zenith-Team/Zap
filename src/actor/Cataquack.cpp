#include <actor/ActorMgr.h>
#include <actor/Profile.h>
#include <player/Yoshi.h>
#include <player/PlayerBase.h>
#include <red/util/SpriteUtil.h>
#include <effect/EffectCreateUtil.h>
#include <zap/Zap.h>
#include <zap/actor/Cataquack.h>
#include <telkin/Print.h>
#include <imgui/imgui.h>

SEAD_RTTI_OVERRIDE_IMPL(zap::Cataquack, Enemy)

// TODO: "Notice" state like Stingby

CREATE_STATE_ID(zap::Cataquack, Walk)
CREATE_STATE_ID(zap::Cataquack, Turn)
CREATE_STATE_ID(zap::Cataquack, Launch)
CREATE_STATE_VIRTUAL_ID_OVERRIDE(zap::Cataquack, Enemy, DieFall)

const ActorCreateInfo zap::Cataquack::cCreateInfo = {
    .offset_x = 8, .offset_y = -8,
    .spawn_range = {
        .offset_x = 8, .offset_y = -8,
        .half_size_x = 32, .half_size_y = 32,
    },
    .cull_range = { .up = 0, .down = 0, .left = 0, .right = 0 },
    .flag = ActorCreateInfo::cFlag_None,
};

constexpr f32 cScaleFactor = 0.1f;
constexpr f32 cAnimBlendTime = 8.0f;

Profile* zap::Cataquack::sProfile = zap::getRegistrar()->newProfile<zap::Cataquack>("cataquack")
    .resources<"poihana">(ProfileInfo::cResType_Course)
    .createInfo(&cCreateInfo)
    .flag(Profile::Flag::cFlag_DrawCullCheck)
    .build();

zap::Cataquack::Cataquack(const ActorCreateParam& param)
    : Enemy(param)
    , mModel(nullptr)
    , mHasLanded(true)
    , mForceLanded(false)
    , mChasing(false)
    , mLaunchForce(0.0f, 4.5f)
{ }

//* ===== Primary Functions ===== *//

ActorBase::Result zap::Cataquack::create() {
    // anims: walk, throw, overturn
    this->mModel = JointBlendModel::create("poihana", "poihana", 3);
    this->mScale = { cScaleFactor, cScaleFactor, cScaleFactor };
    this->mModel->setAnm("walk", cAnimBlendTime);

    // Launch knockback
    const u8 customLaunchForceX = red::SpriteUtil::getBitRange(this, 16, 16 + 8); // nybbles 5 & 6
    if (customLaunchForceX != 0) {
        this->mLaunchForce.x = customLaunchForceX * 0.1f;
    }
    // Launch height
    const u8 customLaunchForceY = red::SpriteUtil::getBitRange(this, 24, 24 + 8); // nybbles 7 & 8
    if (customLaunchForceY != 0) {
        this->mLaunchForce.y = customLaunchForceY * 0.1f;
    }

    // direction
    this->mDirection = this->getPlayerDirLR();
    this->mAngle.y() = cBaseAngleY[this->mDirection];

    // sensors
    static const ActorBgCollisionCheck::Sensor foot = { -6.0f, 6.0f, 0.0f };
    static const ActorBgCollisionCheck::Sensor head = { -6.0f, 6.0f, 28.0f };
    static const ActorBgCollisionCheck::Sensor wall = { 8.0f, 24.0f, 12.0f };
    this->mBgCheckObj.set(this, &foot, &head, &wall);

    // collision
    this->mCollisionCheck.set(this, Cataquack::cCollisionData);
    this->reviveCollisionCheck();

    this->mWaterCalcType = cWaterCalcType_EnablePreCheck;
    this->mCheckWaterNeeded = true; // Required for checkWater to work + makes the actor have water physics in water (less gravity + slower movement)

#ifdef ZAP_DEBUG
    __os_snprintf(this->mImGuiWindowID, sizeof(this->mImGuiWindowID), "Cataquack %i", this->mActorUniqueID.getValue());
#endif

    this->changeState(Cataquack::StateID_Walk);
    this->calcMdl_Normal();
    return cResult_Success;
}

bool zap::Cataquack::execute() {
#ifdef ZAP_DEBUG
    if (ImGui::Begin(this->mImGuiWindowID)) {
        ImGui::DragFloat("Launch Y Speed", &this->mLaunchForce.y, 0.2f, 0.0f, 25.6f);
        ImGui::DragFloat("Launch X Speed", &this->mLaunchForce.x, 0.2f, 0.0f, 25.6f);
    } ImGui::End();
#endif

    this->executeState();
    this->landonEffect();
    this->calcMdl_Normal();
    this->screenOutCheck(0);

    // TODO: proper drowning animation
    if (this->mIsSubmerged) {
        this->changeState(Cataquack::StateID_DieYoshiFumi);
    }

    return true;
}

bool zap::Cataquack::draw() {
    this->mModel->draw();
    return true;
}

void zap::Cataquack::calcMdl_Base() {
    this->mModel->update(
        this->mPos + sead::Vector3f(0.0f, 20.0f, 0.0f),
        this->mAngle,
        this->mScale
    );
}

//* ===== Collision ===== *//

// TODO:
// block barrels (break them)
// dont die to goalpole
// maybe do something about mini players holding barrel/POW not being reachable? (causes stalemate)

using CC = ActorCollisionCheck;
const CC::CollisionData zap::Cataquack::cCollisionData = {
    .center_offset = { 0.0f, 14.0f },
    .half_size = { 11.0f, 14.0f },
    .shape_type = CC::cShapeType_Box,
    .kind = CC::cKind_Enemy,
    .attack = CC::cAttack_Shell,
    .vs_kind = CC::TargetKind(
        CC::cTargetKind_Player |
        CC::cTargetKind_Yoshi |
        CC::cTargetKind_ChibiYoshi |
        CC::cTargetKind_Enemy |
        CC::cTargetKind_Tama // Tama = Projectiles (Fireball/Iceball)
    ),
    .vs_damage = CC::DamageFrom(CC::cDamageFrom_Star | CC::cDamageFrom_FireBall | CC::cDamageFrom_IceBall),
    .status = CC::cStatus_None,
    .callback = &Enemy::normal_collcheck,
};

void zap::Cataquack::vsPlayerHitCheck_Normal(CC* cc_self, CC* cc_other) {
    this->mTarget = cc_other->getOwner()->getActorUniqueID();
    if (!this->isState(Cataquack::StateID_Launch)) {
        this->changeState(Cataquack::StateID_Launch);
    }
}

void zap::Cataquack::vsYoshiHitCheck_Normal(CC* cc_self, CC* cc_other) {
    const Yoshi* yoshi = cc_other->getOwner<Yoshi>();
    if (yoshi->getPlayerRideOn() == nullptr) {
        return; // Don't bounce Yoshis without a rider
    }
    
    this->mTarget = yoshi->getActorUniqueID();
    if (!this->isState(Cataquack::StateID_Launch)) {
        this->changeState(Cataquack::StateID_Launch);
    }
}

void zap::Cataquack::vsEnemyHitCheck_Normal(CC* cc_self, CC* cc_other) {
    const f32 enemyHitRevX = cc_self->getRevisionX(ActorCollisionCheck::cKind_Enemy); // stolen from KuriboBase <3
    
    // only turn if we hit something in the direction we're facing
    if (this->mDirection == cDirType_Left && enemyHitRevX > 0.0f || this->mDirection == cDirType_Right && enemyHitRevX < 0.0f) {
        if (this->isState(Cataquack::StateID_Walk)) {
            this->changeState(Cataquack::StateID_Turn);
        }
    }
}

bool zap::Cataquack::hitCallback_Fire(CC* cc_self, CC* cc_other) {
    return false; // immune
}

bool zap::Cataquack::hitCallback_Ice(CC* cc_self, CC* cc_other) {
    this->iceballInvalid(cc_other);
    return false; // immune
}

// Handles many "throwable" projectiles such as bob-ombs and mechakoopas, not only shells.
bool zap::Cataquack::hitCallback_Shell(CC* cc_self, CC* cc_other) {
    return false; // immune
}

//* ===== Utility Functions ===== *//

void zap::Cataquack::handleMovement() {
    this->calcSpeedY_(); // apply gravity
    this->posMove_(); // apply velocity
    this->bgCheck_(); // Terrain collision
    if (this->bgCheckFoot_()) {
        this->mSpeed.y = 0.0f; // stop accelerating downwards if we hit the ground
    }
}

void zap::Cataquack::setChaseMode(bool active) {
    this->mChasing = active;

    const f32 speed = active ? 1.0f : 0.5f;
    this->mSpeed.x = speed * cEnMuki[this->mDirection];
    this->mSpeedMax.x = speed * cEnMuki[this->mDirection];
    this->mModel->getCurSklAnim()->getFrameCtrl().setRate(speed * 2);
}

//* ===== State Functions ===== *//

/** STATE: Walk */

void zap::Cataquack::initializeState_Walk() {
    const f32 speed = this->mChasing ? 1.0f : 0.5f;
    this->mSpeed.x = speed * cEnMuki[this->mDirection];
    this->mSpeedMax.x = speed * cEnMuki[this->mDirection];
}

void zap::Cataquack::executeState_Walk() {
    this->handleMovement();

    sead::Vector2f distToPlayer;
    if (this->searchNearPlayer(distToPlayer) != -1 && sead::Mathf::abs(distToPlayer.x) < 8.0f * 16 && sead::Mathf::abs(distToPlayer.y) < 6.0f * 16) {
        if ((this->mDirection == cDirType_Left && distToPlayer.x < 0) || (this->mDirection == cDirType_Right && distToPlayer.x > 0)) {
            this->setChaseMode(true);
        }
        if (this->mChasing) {
            if ((this->mDirection == cDirType_Left && distToPlayer.x > 0) || (this->mDirection == cDirType_Right && distToPlayer.x < 0)) {
                this->changeState(Cataquack::StateID_Turn);
            }
        }
    } else {
        this->setChaseMode(false);
    }

    if (this->mBgCheckObj.checkWall(this->mDirection) && !this->mChasing) {
        this->changeState(Cataquack::StateID_Turn);
    }
}

void zap::Cataquack::finalizeState_Walk() { }

/** STATE: Turn */

void zap::Cataquack::initializeState_Turn() {
    this->mDirection = ::InvDirX(this->mDirection);
    this->mSpeed.x = 0;
    this->mSpeedMax.x = 0;
}

void zap::Cataquack::executeState_Turn() {
    this->handleMovement();

    u32 step = sead::Mathf::deg2idx(this->mChasing ? 4.0f : 2.0f);
    if (this->mAngle.y().chaseRest(cBaseAngleY[this->mDirection], step)) {
        this->changeState(Cataquack::StateID_Walk);
    }
}

void zap::Cataquack::finalizeState_Turn() {
    this->mAngle.y() = cBaseAngleY[this->mDirection];
}

/** STATE: Launch */

void zap::Cataquack::initializeState_Launch() { 
    this->mDirection = this->getPlayerDirLR();
    this->mAngle.y() = cBaseAngleY[this->mDirection];

    this->mModel->setAnm("throw", cAnimBlendTime, FrameCtrl::cMode_NoRepeat, 1.2f);
    this->mSpeed.x = 0;
    
    // Can also be Yoshi
    if (PlayerBase* playerlike = static_cast<PlayerBase*>(ActorMgr::instance()->getActorPtr(this->mTarget))) {
        bool bounced = playerlike->bouncePlayer1(
            this->mLaunchForce.y,
            this->mLaunchForce.x * cEnMuki[this->mDirection],
            true,
            PlayerBase::cBounceType_1,
            PlayerBase::cJumpSe_None
        );
        
        if (bounced) {
            GameAudio::getAudioObjMap()->startSound("SE_PLY_RIDE_CLOUD", this->mPos);
        }
    }
}

void zap::Cataquack::executeState_Launch() {
    this->handleMovement();

    if (this->mModel->getCurSklAnim()->getFrameCtrl().isStop()) {
        this->changeState(Cataquack::StateID_Walk);
    }
}

void zap::Cataquack::finalizeState_Launch() {
    this->mModel->setAnm("walk", cAnimBlendTime);
}

/** STATE: DieFall (Enemy override) */
// This is the state the enemy is in when spinning out of screen after being defeated by a Star (for example)

void zap::Cataquack::initializeState_DieFall() {
    Enemy::initializeState_DieFall();
    this->mHasLanded = false;
}

void zap::Cataquack::executeState_DieFall() {
    Enemy::executeState_DieFall();
    this->landonEffect();
}

void zap::Cataquack::finalizeState_DieFall() {
    Enemy::finalizeState_DieFall();
}

//* ===== KuriboBase Functions ===== *//

void zap::Cataquack::landonEffect() {
    if (!this->mHasLanded) {
        sead::Vector3f pos(this->mPos.x, this->mPos.y, 4500.0f);

        if (this->mBgCheckObj.checkFoot()) {
            pos.z = this->getEffectZPos();
            this->mHasLanded = true;

            switch (BgUnitCode::getAttr(this->mBgCheckObj.getBgCheckData(cDirType_Down))) {
                default:
                    EffectCreateUtil::createEffect(RP_Cmn_LandingSmoke_08, &pos);
                    break;
                case BgUnitCode::cNuma:
                case BgUnitCode::cSand:
                    EffectCreateUtil::createEffect(RP_Cmn_LandingSand_04, &pos);
                    break;
                case BgUnitCode::cIce:
                    EffectCreateUtil::createEffect(RP_Cmn_LandingIce_04, &pos);
                    break;
                case BgUnitCode::cSnow:
                    EffectCreateUtil::createEffect(RP_Cmn_LandingSnow_04, &pos);
                    break;
                case BgUnitCode::cWater:
                    EffectCreateUtil::createEffect(RP_Cmn_LandingPillarWtr_04, &pos);
                    break;
                case BgUnitCode::cNone:
                    EffectCreateUtil::createEffect(RP_Cmn_LandingSmoke_08, &pos);
                    break;
            }
        } else {
            sead::Vector3f check_pos = pos;
            check_pos.y -= 2.0f;
            WaterType water_type = ActorBgCollisionCheck::checkWater(&pos.y, check_pos, this->mLayer);
            if (water_type != cWaterType_None) {
                this->mHasLanded = true;
                pos.z = 6500.0f;

                switch (water_type) {
                    default:
                        break;
                    case cWaterType_Water:
                        this->splashEffect_(pos, RP_Cmn_WaterSplash_04, 6, "SE_OBJ_CMN_SPLASH");
                        break;
                    case cWaterType_Lava:
                    case cWaterType_LavaWave:
                        this->splashEffect_(pos, RP_Cmn_LavaSplash_04, 16, "SE_OBJ_CMN_SPLASH_LAVA");
                        break;
                    case cWaterType_Poison:
                        this->splashEffect_(pos, RP_Cmn_PoisonSplash_04, 23, "SE_OBJ_CMN_SPLASH_POISON");
                        break;
                }
            } else if (this->mForceLanded) {
                this->mHasLanded = true;
                sead::Vector3f force_pos(this->mPos.x, this->mPos.y, 4500.0f);
                EffectCreateUtil::createEffect(RP_Cmn_LandingSmoke_08, &force_pos);
            }
        }
    } else if (!this->mBgCheckObj.checkFoot() && !this->checkGround()) {
        this->mHasLanded = false;

        sead::Vector3f check_pos = this->mPos;
        check_pos.y -= 2.0f;
        WaterType water_type = ActorBgCollisionCheck::checkWater(nullptr, check_pos, this->mLayer);

        if (water_type != cWaterType_None) {
            this->mHasLanded = true;

            if (this->mForceLanded) {
                this->mForceLanded = false;
                this->mHasLanded = false;
            }
        } else {
            if (this->mForceLanded) {
                this->mHasLanded = true;
            }
        }
    }
}

bool zap::Cataquack::checkGround() {
    const BgCollisionCheckParam param = {
        ._0 = 0,
        .ignore_quicksand = false,
        .layer = this->mLayer,
        .collision_mask = this->mCollisionMask,
        .type = cBgCollisionCheckType_Solid,
        .callback = nullptr,
    };
    sead::Vector2f p0(this->mPos.x, this->mPos.y);
    sead::Vector2f p1 = p0;
    p1.y -= 4.0f;
    BasicBgCollisionCheck bg_check(param);
    return bg_check.checkArea(nullptr, p0, p1, 1 << cDirType_Down);
}
