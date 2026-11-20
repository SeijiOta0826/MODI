#pragma once

#include <DxLib.h>

class Camera;

class ClearSequence
{
public:
    ClearSequence() = default;
    ~ClearSequence() = default;

    // ゴール演出を開始する
    void Start(Camera* _camera, const VECTOR& _targetPosition);

    // ゴール演出を更新する
    void Update(float _deltaTime);

    // 現在のゲーム世界の時間倍率を取得する
    float GetWorldTimeScale() const;

    // ゴール演出が終了したか
    bool IsFinished() const;

    // ゴール演出をリセットする
    void Reset();

private:
    // ズーム対象のカメラ
    Camera* mpCamera = nullptr;

    // ゴール演出開始時のカメラ位置
    VECTOR mvStartPosition = VGet(0.0f, 0.0f, 0.0f);

    // ゴールしたプレイヤーの位置
    VECTOR mvTargetPosition = VGet(0.0f, 0.0f, 0.0f);

    // ゴール演出の経過時間
    float mfTimer = 0.0f;

    // ゴール演出の時間
    static constexpr float CLEAR_SEQUENCE_TIME = 2.0f;

    // 開始時のズーム倍率
    static constexpr float START_ZOOM = 1.0f;

    // 終了時のズーム倍率
    static constexpr float END_ZOOM = 1.35f;

    // ゴール演出開始時の時間倍率
    static constexpr float START_TIME_SCALE = 1.0f;

    // ゴール演出終了時の時間倍率
    static constexpr float END_TIME_SCALE = 0.1f;
};