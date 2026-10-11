#pragma once

#include <enemy/Enemy.h>
#include <graphics/AnimModel.h>
#include <effect/EffectObj.h>
#include <actor/Profile.h>
#include <enemy/EnemyEatData.h>
#include <collision/ActorCollisionDrcTouchCallback.h>
#include <enemy/EnemyBoyoMgr.h>

namespace zap {

class AngryGrrrol : public Enemy {
    SEAD_RTTI_OVERRIDE(AngryGrrrol, Enemy);

protected:
    class DrcTouchCB : public ActorCollisionDrcTouchCallback {
    public:
        bool ccSetTouchNormal(ActorCollisionCheck* cc, const sead::Vector2f& pos) override;
        void ccOnTouch(ActorCollisionCheck* cc, const sead::Vector2f& pos) override {
            GameAudio::getAudioObjEmy()->startSound("SE_EMY_CMN_TOUCH_HARD", pos, 
            nw::snd::OutputLine::OUTPUT_LINE_DRC|nw::snd::OutputLine::OUTPUT_LINE_MAIN);
        }
    };

public:
    static Profile* sProfile;

    AngryGrrrol(const ActorCreateParam& param);
    ~AngryGrrrol() override = default;
    
    Result create() override;
    bool execute() override;
    bool draw() override;

    void waterSplashEffect(const sead::Vector3f& pos) override;
    void yoganSplashEffect(const sead::Vector3f& pos) override;
    void poisonSplashEffect(const sead::Vector3f& pos) override;
    
    void calcMdl_Base() override;

    void vsYoshiHitCheck_Normal(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;

    bool hitCallback_Shell(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;
    bool hitCallback_Fire(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;
    bool hitCallback_Ice(ActorCollisionCheck* cc_self, ActorCollisionCheck* cc_other) override;

    void yoshiFumiScoreSet(Actor* player) override;

    s16 calculateSoundVolume();
    void waterSplashEffect();

    static const ActorCreateInfo cCreateInfo;
    static const ActorCollisionCheck::CollisionData cCollisionData;

    DECLARE_STATE_ID(AngryGrrrol, Roll);
    
private:
    AnimModel* mModel;
    EnemyEatData mYoshiEatData;
    DrcTouchCB mDrcTouchCallback;
    EnemyBoyoMgr mBoyoMgr;
    EffectObj mEffectSparks;
    bool mTouchingWall;
    bool mTouchingFloor;
    bool mTouchingWater;
};

} // namespace zap
