#include "ClearSequence.h"

#include "Camera.h"

#include <algorithm>

void ClearSequence::Start(Camera* _camera,const VECTOR& _targetPosition)
{
	mpCamera = _camera;
	mfTimer = 0.0f;

	if (mpCamera != nullptr)
	{
		// ゴール演出開始時のカメラ位置を保存
		mvStartPosition = mpCamera->GetPosition();

		// ゴールしたプレイヤーの位置を保存
		mvTargetPosition = _targetPosition;

		// ズーム倍率を初期化
		mpCamera->SetZoom(START_ZOOM);
	}
}

void ClearSequence::Update(float _deltaTime)
{
    if (mpCamera == nullptr)
        return;

    mfTimer += _deltaTime;

    // 0.0 ～ 1.0 の進行率
    const float rate =
        (std::min)(mfTimer / CLEAR_SEQUENCE_TIME, 1.0f);

    // カメラ移動を滑らかにする
    const float easedRate =
        rate * rate * (3.0f - 2.0f * rate);

    // カメラをゴール地点へ滑らかに移動
    const VECTOR cameraPosition = VAdd(
        mvStartPosition,
        VScale(
            VSub(mvTargetPosition, mvStartPosition),
            easedRate
        )
    );

    mpCamera->SetPosition(cameraPosition);

    // ズームもカメラ移動と同じ進行率で行う
    const float zoom = START_ZOOM + (END_ZOOM - START_ZOOM) * easedRate;

    mpCamera->SetZoom(zoom);
}

float ClearSequence::GetWorldTimeScale() const
{
    // 0.0 ～ 1.0 の演出進行率
    const float rate = (std::min)(mfTimer / CLEAR_SEQUENCE_TIME, 1.0f);

    // 徐々に時間を遅くする
    return START_TIME_SCALE + (END_TIME_SCALE - START_TIME_SCALE) * rate;
}

bool ClearSequence::IsFinished() const
{
	return mfTimer >= CLEAR_SEQUENCE_TIME;
}

void ClearSequence::Reset()
{
	mpCamera = nullptr;
	mfTimer = 0.0f;
}