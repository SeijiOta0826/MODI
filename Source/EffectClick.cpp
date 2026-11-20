#include "EffectClick.h"

EffectClick::EffectClick(float fX, float fY)
    : EffectBase(fX, fY, AUTO_LIFETIME)
{
    auto& effekseer = EffekseerManager::GetInstance();

    // 1. 拡大率 LOAD_SCALE (40.0f) を指定して読み込み（2回目以降はキャッシュを即返す）
    const int resourceHandle = effekseer.Load(EFFECT_PATH, LOAD_SCALE);
    if (resourceHandle == EffekseerManager::INVALID_HANDLE)
    {
        // Load() 内で詳細なエラー理由がログ出力される。失敗時は即座に自身を削除する
        Kill();
        return;
    }

    // 2. クリック座標 (fX, fY) で再生（内部で SetPosPlayingEffekseer2DEffect を呼ぶ）
    m_handle = effekseer.Play(EFFECT_PATH, fX, fY, PLAY_SCALE);
    if (m_handle == EffekseerManager::INVALID_HANDLE)
    {
        printfDx("【EffectClickエラー】エフェクトの再生に失敗しました: %s\n", EFFECT_PATH);
        Kill();
    }
}

EffectClick::~EffectClick()
{
    if (m_handle != EffekseerManager::INVALID_HANDLE)
    {
        EffekseerManager::GetInstance().Stop(m_handle);
        m_handle = EffekseerManager::INVALID_HANDLE;
    }
}

void EffectClick::OnUpdate([[maybe_unused]] float fDeltaTime)
{
    // Effekseer の再生が終わったら自動で消滅させる
    if (!EffekseerManager::GetInstance().IsPlaying(m_handle))
    {
        m_handle = EffekseerManager::INVALID_HANDLE;
        Kill();
    }
}

void EffectClick::Draw() const
{
 
    if (m_handle == EffekseerManager::INVALID_HANDLE)
    {
        return;
    }

    EffekseerManager::GetInstance().DrawEffect(m_handle);
}