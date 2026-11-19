#include "EffectClick.h"
#include "EffekseerManager.h"

EffectClick::EffectClick(float fX, float fY)
    : EffectBase(fX, fY, NO_LIFE_TIME)
{
    // クリックされた座標 (fX, fY) に Effekseer エフェクトを再生し、番号を受け取る
    m_handle = EffekseerManager::GetInstance().Play(EFFECT_PATH, fX, fY, SCALE);
}

EffectClick::~EffectClick()
{
    // 途中で消された場合はエフェクトも止める
    if (EffekseerManager::GetInstance().IsPlaying(m_handle))
    {
        EffekseerManager::GetInstance().Stop(m_handle);
    }
}

void EffectClick::OnUpdate([[maybe_unused]] float fDeltaTime)
{
    // Effekseerの再生が終わったら（またはファイルがまだ無ければ）、このオブジェクトを消滅させる
    if (!EffekseerManager::GetInstance().IsPlaying(m_handle))
    {
        Kill();
    }
}

void EffectClick::Draw() const
{
    // 自分の番号のエフェクトを描画する
    EffekseerManager::GetInstance().DrawEffect(m_handle);
}