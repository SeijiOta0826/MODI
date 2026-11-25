#pragma once

#include "EffekseerManager.h"

// =============================================================================
// ■ EffectHappy クラス（PlayerがGoalに近づいたときの喜びエフェクト）
// =============================================================================
class EffectHappy
{
public:
    // --- 調整用定数（パス・距離・位置・大きさはここで調整） ---
    static constexpr const char* EFFECT_PATH = "Resource/Effect/Effekseer/Player/HappyUI/HappyUI.efkefc";

    // Goalにどれくらい近づいたら反応するか（1マス = 100px、接触距離は200px前後のため400pxに設定）
    static constexpr float HAPPY_DISTANCE = 400.0f;

    // Playerの中心から頭上へずらすYオフセット（マイナスで上方向）
    static constexpr float OFFSET_Y = -80.0f;

    // 読み込み時の拡大率
    static constexpr float LOAD_SCALE = 40.0f;

    // 再生時の追加倍率
    static constexpr float PLAY_SCALE = 1.0f;

public:
    EffectHappy() = default;
    ~EffectHappy();

    // 毎フレーム呼ぶ：近くにいる間は再生＆追従、離れたら停止
    // 呼び出し例 : m_happyEffect.Update(isNearGoal, playerPos.x, playerPos.y);
    void Update(bool isNearGoal, float playerX, float playerY);

    // 描画処理
    // 呼び出し例 : m_happyEffect.Draw();
    void Draw() const;

    // 強制停止（シーン終了時やクリア時など）
    // 呼び出し例 : m_happyEffect.Clear();
    void Clear();

private:
    int m_handle = EffekseerManager::INVALID_HANDLE;
};