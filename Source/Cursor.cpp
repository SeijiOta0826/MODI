#include "Cursor.h"

#include "Master.h"
#include "ResourceManager.h"

#include "InputManager.h"
#include "Mouse.h"

#include "DxLib.h"

namespace
{
    // -- ホットスポットの調整に用いる定数 -- //
    constexpr int offsetX = -50;
    constexpr int offsetY = -100;
}

void Cursor::Initialize()
{
    // -- 箸が開いている画像 -- //
    mOpenHandle =
        Master::mpResource->LoadGraphics(
            "Resource/UI/Cursor/Chopsticks.png"
        );

    // -- 箸が閉じている画像 -- //
    mCloseHandle =
        Master::mpResource->LoadGraphics(
            "Resource/UI/Cursor/Chopsticks(1).png"
        );

    // -- 画像の読み込みに成功したらWindowsのマウスカーソルを非表示 -- //
    if (mOpenHandle != -1)
        SetMouseDispFlag(FALSE);
}

void Cursor::Draw()
{
    if (mOpenHandle == -1 || mCloseHandle == -1)
        return;

    const auto& mouse = InputManager::GetInstance().GetMouse();

    int x = mouse.GetX();
    int y = mouse.GetY();

    // -- 左クリックしている間は閉じた箸にする -- //
    int handle = mOpenHandle;

    if (mouse.IsPress(MOUSE_INPUT_LEFT))
    {
        handle = mCloseHandle;
    }

    DrawGraph(
        x + offsetX,
        y + offsetY,
        handle,
        TRUE
    );
}

void Cursor::Finalize()
{
    // -- Windowsのマウスカーソルを再表示 -- //
    SetMouseDispFlag(TRUE);
}