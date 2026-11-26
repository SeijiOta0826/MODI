#include "EffectGloss.h"

EffectGloss::~EffectGloss()
{
    Clear();
}

void EffectGloss::Update(const std::vector<VECTOR>& piecePositions)
{
    auto& effekseer = EffekseerManager::GetInstance();
    const size_t targetCount = piecePositions.size();

    // マス数が減った場合は余分なエフェクトを停止して削除する
    while (m_handles.size() > targetCount)
    {
        const int handle = m_handles.back();
        if (handle != EffekseerManager::INVALID_HANDLE)
        {
            effekseer.Stop(handle);
        }
        m_handles.pop_back();
    }

    // マス数が増えた場合はハンドル枠を追加する
    while (m_handles.size() < targetCount)
    {
        m_handles.push_back(EffekseerManager::INVALID_HANDLE);
    }

    for (size_t i = 0; i < targetCount; ++i)
    {
        const float drawX = piecePositions[i].x + OFFSET_X;
        const float drawY = piecePositions[i].y + OFFSET_Y;

        if (!effekseer.IsPlaying(m_handles[i]))
        {
            const int resourceHandle = effekseer.Load(EFFECT_PATH, LOAD_SCALE);
            if (resourceHandle == EffekseerManager::INVALID_HANDLE)
            {
                return;
            }

            m_handles[i] = effekseer.Play(EFFECT_PATH, drawX, drawY, PLAY_SCALE);
            if (m_handles[i] != EffekseerManager::INVALID_HANDLE)
            {
                effekseer.SetSpeed(m_handles[i], PLAY_SPEED);
            }
        }
        else
        {
            effekseer.SetPosition(m_handles[i], drawX, drawY);
        }
    }
}

void EffectGloss::Draw() const
{
    const auto& effekseer = EffekseerManager::GetInstance();

    // DrawEffect内で全2Dエフェクトが一括描画されるため、最初の1回だけ呼ぶ
    for (const int handle : m_handles)
    {
        if (handle != EffekseerManager::INVALID_HANDLE)
        {
            effekseer.DrawEffect(handle);
            return;
        }
    }
}

void EffectGloss::Clear()
{
    auto& effekseer = EffekseerManager::GetInstance();

    for (const int handle : m_handles)
    {
        if (handle != EffekseerManager::INVALID_HANDLE)
        {
            effekseer.Stop(handle);
        }
    }
    m_handles.clear();
}