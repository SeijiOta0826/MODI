#include "UIHoverScaler.h"
#include "UI.h"

void UIHoverScaler::Update()
{
    if (!mbActive || mpUI == nullptr) return;

    const bool  isHovered = mpUI->GetButton().IsHighlighted();	// マウスが乗っている or FocusUIに選ばれている
    const float target = isHovered ? mfHoverScale : 1.0f;

    // Todo : deltaTime対応(フレームレートに依存しない補間にする)
    mfCurrentScale += (target - mfCurrentScale) * mfLerpRate;
}