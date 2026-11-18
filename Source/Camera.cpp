#include "Camera.h"
#include <algorithm>

Camera::Camera()
{
    Initialize();
}

void Camera::Initialize()
{
    m_position = VGet(CENTER_X, CENTER_Y, ZERO_VALUE);
    m_shakeOffset = VGet(ZERO_VALUE, ZERO_VALUE, ZERO_VALUE);
    m_zoom = DEFAULT_ZOOM;
}

void Camera::Apply() const
{
    // 2D平行投影でズーム倍率を反映
    SetCameraNearFar(NEAR_DISTANCE, FAR_DISTANCE);
    SetupCamera_Ortho(VIEW_SIZE / m_zoom);

    // 揺れを足した座標をDxLibのカメラに設定
    const VECTOR finalPos = GetFinalPosition();
    const VECTOR eyePos = VGet(finalPos.x, finalPos.y, CAMERA_Z);
    const VECTOR target = VGet(finalPos.x, finalPos.y, TARGET_Z);

    SetCameraPositionAndTarget_UpVecY(eyePos, target);
}

void Camera::SetPosition(float x, float y) noexcept
{
    m_position = VGet(x, y, ZERO_VALUE);
}

void Camera::SetShakeOffset(float offsetX, float offsetY) noexcept
{
    m_shakeOffset = VGet(offsetX, offsetY, ZERO_VALUE);
}

void Camera::AddShakeOffset(const VECTOR& offset) noexcept
{
    m_shakeOffset = VAdd(m_shakeOffset, offset);
}

VECTOR Camera::GetFinalPosition() const noexcept
{
    return VAdd(m_position, m_shakeOffset);
}

VECTOR Camera::GetDrawOffset() const noexcept
{
    const VECTOR finalPos = GetFinalPosition();

    // 初期中心（960, 540）からのズレ量を算出
    return VGet(
        CENTER_X - finalPos.x,
        CENTER_Y - finalPos.y,
        ZERO_VALUE
    );
}

VECTOR Camera::WorldToScreen(const VECTOR& worldPos) const noexcept
{
    return VAdd(worldPos, GetDrawOffset());
}

VECTOR Camera::ScreenToWorld(const VECTOR& screenPos) const noexcept
{
    return VSub(screenPos, GetDrawOffset());
}

void Camera::SetZoom(float zoom) noexcept
{
    m_zoom = (std::max)(MIN_ZOOM, zoom);
}