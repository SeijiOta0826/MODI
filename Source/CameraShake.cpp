#include "CameraShake.h"
#include <algorithm>

void CameraShake::Start(float duration, float power) noexcept
{
    // 時間や強さに0以下が渡された場合は揺らさずに終了
    if (duration <= ZERO_VALUE || power <= ZERO_VALUE)
    {
        Stop();
        return;
    }

    m_duration = duration;
    m_power = power;
    m_elapsedTime = ZERO_VALUE;
}

void CameraShake::Stop() noexcept
{
    // 時間をリセットし、揺れのズレ量も (0, 0, 0) に戻す
    m_duration = ZERO_VALUE;
    m_elapsedTime = ZERO_VALUE;
    m_power = ZERO_VALUE;
    m_offset = VGet(ZERO_VALUE, ZERO_VALUE, ZERO_VALUE);
}

void CameraShake::Update(float deltaTime) noexcept
{
    // 1. 揺れ中でなければ、ズレを0にして何もしない
    if (!IsShaking())
    {
        m_offset = VGet(ZERO_VALUE, ZERO_VALUE, ZERO_VALUE);
        return;
    }

    // 2. 経過時間を進める
    m_elapsedTime += deltaTime;

    // 3. 設定した揺れ時間を過ぎたら、ピタッと止めて終了
    if (m_elapsedTime >= m_duration)
    {
        Stop();
        return;
    }

    // 4. 終わり際に向かってだんだん揺れを弱くする計算（1.0 → 0.0 へ減っていく割合）
    const float remainingRatio = 1.0f - (m_elapsedTime / m_duration);
    const float currentPower = m_power * remainingRatio;

    // 5. 上下左右（X, Y）にランダムな方向へ揺らす量を決める
    const float shakeX = GetRandomNormalized() * currentPower;
    const float shakeY = GetRandomNormalized() * currentPower;

    m_offset = VGet(shakeX, shakeY, ZERO_VALUE);
}

float CameraShake::GetRandomNormalized() noexcept
{
    // DxLibのGetRandを使って 0.0f ? 1.0f の小数を作り、それを -1.0f ? +1.0f に変換する
    const float zeroToOne = static_cast<float>(GetRand(RAND_RESOLUTION)) / static_cast<float>(RAND_RESOLUTION);
    return (zeroToOne * RAND_SCALE) - RAND_OFFSET;
}