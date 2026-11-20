#include "GoalEffect.h"

GoalEffect::~GoalEffect()
{
    Clear();
}

void GoalEffect::Trigger(float startX, float startY)
{
    

    auto& effekseer = EffekseerManager::GetInstance();

    // 1. 拡大率 LOAD_SCALE を指定して読み込み（2回目以降はキャッシュを即返す）
    const int resourceHandle = effekseer.Load(EFFECT_PATH, LOAD_SCALE);
    if (resourceHandle == EffekseerManager::INVALID_HANDLE)
    {
        return;
    }

    // 2. 指定座標 (startX, startY) で再生
    const int playingHandle = effekseer.Play(EFFECT_PATH, startX, startY, PLAY_SCALE);
    if (playingHandle == EffekseerManager::INVALID_HANDLE)
    {
        printfDx("【GoalEffectエラー】エフェクトの再生に失敗しました: %s\n", EFFECT_PATH);
        return;
    }
    // 3. 再生速度を設定（ゆっくり再生）
    effekseer.SetSpeed(playingHandle, PLAY_SPEED);

    m_handles.push_back(playingHandle);

}

void GoalEffect::Update()
{
    const auto& effekseer = EffekseerManager::GetInstance();

    // C++20: 再生が終了したエフェクトのハンドルをまとめて削除
    std::erase_if(m_handles, [&effekseer](int handle) {
        return !effekseer.IsPlaying(handle);
        });
}

void GoalEffect::Draw() const
{
  

    const auto& effekseer = EffekseerManager::GetInstance();

    for (const int handle : m_handles)
    {
        effekseer.DrawEffect(handle);
    }
}

void GoalEffect::Clear()
{
    auto& effekseer = EffekseerManager::GetInstance();

    for (const int handle : m_handles)
    {
        effekseer.Stop(handle);
    }
    m_handles.clear();
}