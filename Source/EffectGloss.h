#pragma once

#include "EffekseerManager.h"
#include <DxLib.h>
#include <vector>

// Playerの全アクティブマスに常時表示して光沢感を出すエフェクト
class EffectGloss
{
public:
    static constexpr const char* EFFECT_PATH = "Resource/Effect/Effekseer/Player/GlossUI/GlossUI.efkefc";

    // 各マスの中心からのオフセット
    static constexpr float OFFSET_X = 0.0f;
    static constexpr float OFFSET_Y = 0.0f;

    // 読み込み時の拡大率
    static constexpr float LOAD_SCALE = 40.0f;

    // 再生時の追加倍率
    static constexpr float PLAY_SCALE = 1.0f;

    // 再生速度（1.0fが通常）
    static constexpr float PLAY_SPEED = 1.0f;

public:
    EffectGloss() = default;
    ~EffectGloss();

    // 有効な全PlayerPieceの座標に合わせて再生と追従を行う
    void Update(const std::vector<VECTOR>& piecePositions);

    // 描画処理
    void Draw() const;

    // 強制停止
    void Clear();

private:
    std::vector<int> m_handles;
};