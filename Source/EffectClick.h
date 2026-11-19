#pragma once
#include "EffectBase.h"

// =============================================================================
// ■ EffectClick クラス（マウスクリック位置にEffekseerエフェクトを出すクラス）
// =============================================================================
class EffectClick final : public EffectBase
{
public:
    // --- 設定用の定数 ---
    static constexpr const char* EFFECT_PATH = "Resource/Effect/EffekseerA/Click.efkefc";
    static constexpr float       SCALE = 15.0;                          // 表示サイズ倍率
    static constexpr float       NO_LIFE_TIME = 0.0f;                   // Effekseer側の再生終了に合わせるため0

public:
    EffectClick(float fX, float fY);
    ~EffectClick() override;

    void Draw() const override;

protected:
    void OnUpdate(float fDeltaTime) override;

private:
    int m_handle = -1; // Effekseerで再生中のエフェクト識別番号

public:
    [[nodiscard]] int GetHandle() const noexcept
    {
        return m_handle;
    }
};