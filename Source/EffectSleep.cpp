#include "EffectSleep.h"

EffectSleep::~EffectSleep()
{
    Clear();
}

void EffectSleep::Update(bool hasInput, float deltaTime, float playerX, float playerY)
{
    // Ä¶’†‚ÌWakeUp‚ğPlayer‚É’Ç]‚³‚¹‚é
    UpdateWakeUpEffect(playerX, playerY);

    // “ü—Í‚ª‚ ‚Á‚½ê‡
    if (hasInput)
    {
        // Sleep’†‚©‚ç‹N‚«‚½uŠÔ‚¾‚¯WakeUp‚ğÄ¶‚·‚é
        if (m_isSleeping)
        {
            m_isSleeping = false;
            StopSleepEffect();
            PlayWakeUpEffect(playerX, playerY);
        }

        m_idleTimer = 0.0f;
        return;
    }

    m_idleTimer += deltaTime;

    if (m_idleTimer < SLEEP_WAIT_TIME)
    {
        return;
    }

    auto& effekseer = EffekseerManager::GetInstance();
    const float drawX = playerX + OFFSET_X;
    const float drawY = playerY + OFFSET_Y;

    // –¢Ä¶‚È‚çSleep‚ğ“Ç‚İ‚ñ‚ÅÄ¶‚·‚é
    if (!effekseer.IsPlaying(m_handle))
    {
        const int resourceHandle = effekseer.Load(EFFECT_PATH, LOAD_SCALE);
        if (resourceHandle == EffekseerManager::INVALID_HANDLE)
        {
            return;
        }

        m_handle = effekseer.Play(EFFECT_PATH, drawX, drawY, PLAY_SCALE);
        if (m_handle != EffekseerManager::INVALID_HANDLE)
        {
            effekseer.SetSpeed(m_handle, PLAY_SPEED);
            m_isSleeping = true;
        }
    }
    else
    {
        // Ä¶’†‚È‚çPlayer‚Ì‰E’[‚É’Ç]‚³‚¹‚é
        effekseer.SetPosition(m_handle, drawX, drawY);
        m_isSleeping = true;
    }
}

void EffectSleep::Draw() const
{
    const auto& effekseer = EffekseerManager::GetInstance();

    if (m_handle != EffekseerManager::INVALID_HANDLE)
    {
        effekseer.DrawEffect(m_handle);
    }

    if (m_wakeUpHandle != EffekseerManager::INVALID_HANDLE)
    {
        effekseer.DrawEffect(m_wakeUpHandle);
    }
}

void EffectSleep::Clear()
{
    m_idleTimer = 0.0f;
    m_isSleeping = false;
    StopSleepEffect();
    StopWakeUpEffect();
}

void EffectSleep::PlayWakeUpEffect(float playerX, float playerY)
{
    StopWakeUpEffect();

    auto& effekseer = EffekseerManager::GetInstance();
    const int resourceHandle = effekseer.Load(WAKEUP_EFFECT_PATH, WAKEUP_LOAD_SCALE);
    if (resourceHandle == EffekseerManager::INVALID_HANDLE)
    {
        return;
    }

    const float drawX = playerX + WAKEUP_OFFSET_X;
    const float drawY = playerY + WAKEUP_OFFSET_Y;

    m_wakeUpHandle = effekseer.Play(WAKEUP_EFFECT_PATH, drawX, drawY, WAKEUP_PLAY_SCALE);
    if (m_wakeUpHandle != EffekseerManager::INVALID_HANDLE)
    {
        effekseer.SetSpeed(m_wakeUpHandle, WAKEUP_PLAY_SPEED);
    }
}

void EffectSleep::UpdateWakeUpEffect(float playerX, float playerY)
{
    if (m_wakeUpHandle == EffekseerManager::INVALID_HANDLE)
    {
        return;
    }

    auto& effekseer = EffekseerManager::GetInstance();
    if (!effekseer.IsPlaying(m_wakeUpHandle))
    {
        m_wakeUpHandle = EffekseerManager::INVALID_HANDLE;
        return;
    }

    effekseer.SetPosition(m_wakeUpHandle, playerX + WAKEUP_OFFSET_X, playerY + WAKEUP_OFFSET_Y);
}

void EffectSleep::StopSleepEffect()
{
    if (m_handle != EffekseerManager::INVALID_HANDLE)
    {
        EffekseerManager::GetInstance().Stop(m_handle);
        m_handle = EffekseerManager::INVALID_HANDLE;
    }
}

void EffectSleep::StopWakeUpEffect()
{
    if (m_wakeUpHandle != EffekseerManager::INVALID_HANDLE)
    {
        EffekseerManager::GetInstance().Stop(m_wakeUpHandle);
        m_wakeUpHandle = EffekseerManager::INVALID_HANDLE;
    }
}