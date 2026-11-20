#pragma once

#include "EffekseerManager.h"

// =============================================================================
// ■ EffectDanger クラス（PlayerがSpikeに近づいたときの警告エフェクト）
// =============================================================================
class EffectDanger
{
public:
    // --- 調整用定数（パス・距離・位置・大きさはここで調整） ---
    static constexpr const char* EFFECT_PATH = "Resource/Effect/Effekseer/Player/DangerUI/Danger.efkefc";

    // Spikeにどれくらい近づいたら反応するか（ピクセル単位：1マス ≒ 64〜100px 目安）
    static constexpr float DANGER_DISTANCE = 200.0f;

    // Playerの中心から頭上へずらすYオフセット（マイナスで上方向）
    static constexpr float OFFSET_Y = -80.0f;

    // 読み込み時の拡大率
    static constexpr float LOAD_SCALE = 40.0f;

    // 再生時の追加倍率
    static constexpr float PLAY_SCALE = 1.0f;

public:
    EffectDanger() = default;
    ~EffectDanger();

    // 毎フレーム呼ぶ：近くにいる間は再生＆追従、離れたら停止
    // 呼び出し例 : m_dangerEffect.Update(isNearSpike, playerPos.x, playerPos.y);
    void Update(bool isNearSpike, float playerX, float playerY);

    // 描画処理
    // 呼び出し例 : m_dangerEffect.Draw();
    void Draw() const;

    // 強制停止（シーン終了時やクリア時など）
    // 呼び出し例 : m_dangerEffect.Clear();
    void Clear();

private:
    int m_handle = EffekseerManager::INVALID_HANDLE;
};