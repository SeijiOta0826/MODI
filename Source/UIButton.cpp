#include "UIButton.h"
#include "UI.h"
#include "UITransform.h"
#include "InputManager.h"
#include "SoundManager.h"

void UIButton::Initialize(UI* _pUI)
{
    mpUI = _pUI;
}

bool UIButton::IsMouseInside()
{
    if (mpUI == nullptr)
        return false;

    if (!mpUI->IsEnabled())
        return false;

    UITransform& transform = mpUI->GetTransform();

    VECTOR center = transform.GetCenterPosition();
    VECTOR size = transform.GetSize();
    VECTOR scale = transform.GetScale();

    // 描画(DrawRotaGraph3)と同じく拡縮後の大きさで判定する
    size.x *= scale.x;
    size.y *= scale.y;

    int mouseX = 0;
    int mouseY = 0;

    GetMousePoint(&mouseX, &mouseY);

    return
        mouseX >= center.x - size.x * 0.5f &&
        mouseX <= center.x + size.x * 0.5f &&
        mouseY >= center.y - size.y * 0.5f &&
        mouseY <= center.y + size.y * 0.5f;
}

bool UIButton::IsClicked()
{
    // -- 1. マウスでのクリック -- //
    if (IsMouseInside()
        && InputManager::GetInstance().GetButtonDown(Button::Confirm))
        return true;

    // -- 2. FocusUIでの決定 -- //
    return IsFocusDecided();
}

bool UIButton::IsHighlighted()
{
    if (mbFocusManaged)
        return mbFocused && mpUI != nullptr && mpUI->IsEnabled();

    return IsMouseInside();
}

bool UIButton::IsFocusDecided()
{
    if (!mbFocused || mpUI == nullptr || !mpUI->IsEnabled())
        return false;

    auto& input = InputManager::GetInstance();
    if (!input.GetButtonDown(Button::FocusDecide))
        return false;

    // パッドの決定ボタンは Confirm と共用。その場合は EffectManager 側で決定SEが鳴るので二重にしない
    if (mbPlayDecideSE && !input.GetButtonDown(Button::Confirm))
        SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_DECIDE);

    // 1回の決定入力は1つのボタンだけが使う
    // (「操作説明」を押した同じフレームに、開いた先の「戻る」まで押されてしまうのを防ぐ)
    input.ConsumeButton(Button::FocusDecide);
    return true;
}
