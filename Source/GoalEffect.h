#pragma once

#include "EffekseerManager.h"
#include <vector>

// =============================================================================
// ■ GoalEffect クラス（Effekseer 2D 版・ゴール演出管理クラス）
// =============================================================================
// 【使い方】
//   1. 発生させたい瞬間に Trigger(x, y) を呼ぶ
//   2. 毎フレーム Update() と Draw() を呼ぶ
//   3. シーン終了やリセット時に Clear() を呼ぶ
// =============================================================================
class GoalEffect
{
public:
    // --- 調整用定数（パスや大きさはここを変えるだけでOK） ---
    // ※ .efk の場合は拡張子を ".efk" に変更してください
    // ※ 同じフォルダに Texture フォルダを配置しておくこと
    static constexpr const char* EFFECT_PATH = "Resource/Effect/Effekseer/Goal/GoalEffect.efkefc";

    // 読み込み時の拡大率（大きすぎ／小さすぎたらここを調整）
    static constexpr float LOAD_SCALE = 2.5f;

    // 再生時の追加倍率（通常は 1.0f のままでOK）
    static constexpr float PLAY_SCALE = 15.0f;

    // 再生速度（1.0f = 通常、0.5f = 半分の速さでゆっくり、2.0f = 2倍速）
    static constexpr float PLAY_SPEED = 0.55f;


public:
    GoalEffect() = default;
    ~GoalEffect();

    // 指定した 2D 座標でゴールエフェクトを発生させる（複数回の同時発生にも対応）
    // 呼び出し例 : m_goalEffect.Trigger(goalX, goalY);
    void Trigger(float startX, float startY);

    // 毎フレームの更新処理（再生が終わったハンドルを自動で片付ける）
    // 呼び出し例 : m_goalEffect.Update();
    void Update();

    // 毎フレームの描画処理（再生中のゴールエフェクトをすべて描画する）
    // 呼び出し例 : m_goalEffect.Draw();
    void Draw() const;

    // 再生中のゴールエフェクトをすべて停止してリセットする
    // 呼び出し例 : m_goalEffect.Clear();
    void Clear();

private:
    // 再生中の Effekseer ハンドル一覧
    std::vector<int> m_handles;
};