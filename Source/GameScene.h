#pragma once

#include "Scene.h"
#include "ClearSequence.h"
#include "ItemRecord.h"		// StageFlags を使うため
#include "GoalEffect.h"
#include "EffectDanger.h"
#include "EffectHappy.h"

class Player;
class Goal;
class PauseUI;
class ClearUI;
class ItemHUD;
class PlayerShapeUI;
class RespawnUI;
class KitchenFrame;

class StageBackGround;

class GameScene : public Scene
{
public:
    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw() override;
    void Finalize() override;

private:
    // ゲームの進行状態
    enum class PlayState
    {
        Playing,    // 通常プレイ（ポーズ含む）
        Clearing,	// ゴール演出中
        Cleared,    // クリア後
    };

private:
    void UpdatePlaying(float deltaTime);    // Playing中の更新
    void UpdateClearing(float deltaTime);   // Clearing中の更新
    void UpdateCleared(float deltaTime);    // Cleared中の更新
    void EnterClear();                      // Playing → Cleared への切り替え（1回だけ呼ばれる）

    void UpdatePauseInput();        // 入力に伴い、PauseUIを有効にする
    void UpdateShapeInput();        // 変形キーで変形UIを開閉する
    void UpdateRestartInput(float deltaTime);
    void UpdateItems();             // Item の取得状態を確認する
    void UpdateDangerEffect(); //Playerの危険を知らせるUIの表示
    void UpdateHappyEffect();  //PlayerがGoalに近づいたときのUI表示
    void UpdateMissPenalty();       // 難易度Normal : ミスしたらアイテムをリセットする
    void ResetPickedItems();        // 今回のプレイで取ったアイテムを、取る前の状態に戻す

private:
    PlayState mState = PlayState::Playing;

    Player* mpPlayer = nullptr;
    Goal* mpGoal = nullptr;
    RespawnUI* mpRespawnUI = nullptr;

    PauseUI* mpPauseUI = nullptr;
    ClearUI* mpClearUI = nullptr;
    ItemHUD* mpItemHUD = nullptr;
    ItemRecord::StageFlags mPickedItems{};		// 今回のプレイの仮取得

    PlayerShapeUI* mpShapeUI = nullptr;
    StageBackGround* mpBackgroundUI = nullptr;
    KitchenFrame* mpKitchenFrame = nullptr;
    ClearSequence mClearSequence;

    GoalEffect m_goalEffect;


    EffectDanger m_dangerEffect;
    EffectHappy m_happyEffect;

};