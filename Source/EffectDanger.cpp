#include "EffectDanger.h"

EffectDanger::~EffectDanger()
{
    Clear();
}

void EffectDanger::Update(bool isNearSpike, float playerX, float playerY)
{
    auto& effekseer = EffekseerManager::GetInstance();

    if (isNearSpike)
    {
        const float drawX = playerX;
        const float drawY = playerY + OFFSET_Y;

        // まだ再生していなければ再生開始（重複再生を防ぐ）
        if (!effekseer.IsPlaying(m_handle))
        {
            const int resourceHandle = effekseer.Load(EFFECT_PATH, LOAD_SCALE);
            if (resourceHandle == EffekseerManager::INVALID_HANDLE)
            {
                return;
            }

            m_handle = effekseer.Play(EFFECT_PATH, drawX, drawY, PLAY_SCALE);
        }
        else
        {
            // 再生中なら Player の頭上に追従させる
            effekseer.SetPosition(m_handle, drawX, drawY);
        }
    }
    else
    {
        // Spike から離れたら停止して消す
        Clear();
    }
}

void EffectDanger::Draw() const
{
    if (m_handle == EffekseerManager::INVALID_HANDLE)
    {
        return;
    }

    EffekseerManager::GetInstance().DrawEffect(m_handle);
}

void EffectDanger::Clear()
{
    if (m_handle != EffekseerManager::INVALID_HANDLE)
    {
        EffekseerManager::GetInstance().Stop(m_handle);
        m_handle = EffekseerManager::INVALID_HANDLE;
    }
}