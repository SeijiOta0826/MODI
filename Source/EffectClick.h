#pragma once

#include "EffectBase.h"
#include "EffekseerManager.h"

// =============================================================================
// ■ EffectClick クラス（マウスクリック時の Effekseer 2D エフェクト）
// =============================================================================
class EffectClick final : public EffectBase
{
public:
    // --- 調整用定数（ここを変えるだけでパスや大きさを調整できる） ---
    // ※ TofuClick_2D.efk と同じフォルダに Texture フォルダを配置すること
    static constexpr const char* EFFECT_PATH = "Resource/Effect/Effekseer/Click/Click.efkefc";

    // 読み込み時の拡大率（DXライブラリの2Dでは 1.0f ≒ 1px のため、まず 40.0f を設定）
    static constexpr float LOAD_SCALE = 27.0f;

    // 再生時の追加倍率（通常は 1.0f のままでOK）
    static constexpr float PLAY_SCALE = 1.0f;

    // 生存時間は Effekseer 側の再生終了に合わせて管理するため 0.0f（無制限）にする
    static constexpr float AUTO_LIFETIME = 0.0f;

public:
    EffectClick(float fX, float fY);
    ~EffectClick() override;

    void Draw() const override;

protected:
    void OnUpdate(float fDeltaTime) override;

private:
    int m_handle = EffekseerManager::INVALID_HANDLE;
};