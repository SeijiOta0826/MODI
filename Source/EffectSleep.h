#pragma once

#include "EffekseerManager.h"

// 無操作時のSleepと、キー入力で起きたときのWakeUpを管理するエフェクト
class EffectSleep
{
public:
    // Sleep用設定
    static constexpr const char* EFFECT_PATH = "Resource/Effect/Effekseer/Player/SleepUI/SleepUI.efkefc";
    static constexpr float SLEEP_WAIT_TIME = 6.0f;
    static constexpr float OFFSET_X = 75.0f;
    static constexpr float OFFSET_Y = -60.0f;
    static constexpr float LOAD_SCALE = 30.0f;
    static constexpr float PLAY_SCALE = 1.0f;
    static constexpr float PLAY_SPEED = 0.4f;

    // WakeUp用設定
    static constexpr const char* WAKEUP_EFFECT_PATH = "Resource/Effect/Effekseer/Player/WakeUpUI/WakeUpUI.efkefc";
    static constexpr float WAKEUP_OFFSET_X = 0.0f;
    static constexpr float WAKEUP_OFFSET_Y = -80.0f;
    static constexpr float WAKEUP_LOAD_SCALE = 40.0f;
    static constexpr float WAKEUP_PLAY_SCALE = 1.0f;
    static constexpr float WAKEUP_PLAY_SPEED = 1.0f;

public:
    EffectSleep() = default;
    ~EffectSleep();

    // 毎フレーム更新し、SleepとWakeUpの再生と追従を行う
    void Update(bool hasInput, float deltaTime, float playerX, float playerY);

    // 描画処理
    void Draw() const;

    // 全エフェクトの停止と状態リセット
    void Clear();

private:
    void PlayWakeUpEffect(float playerX, float playerY);
    void UpdateWakeUpEffect(float playerX, float playerY);
    void StopSleepEffect();
    void StopWakeUpEffect();

private:
    int m_handle = EffekseerManager::INVALID_HANDLE;
    int m_wakeUpHandle = EffekseerManager::INVALID_HANDLE;
    float m_idleTimer = 0.0f;
    bool m_isSleeping = false;
};