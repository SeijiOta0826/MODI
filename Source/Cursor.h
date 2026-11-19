#pragma once

class Cursor
{
public:
    Cursor() = default;
    ~Cursor() = default;

    void Initialize();  // 画像のロード & デフォルトカーソルを非表示
    void Draw();        // カーソルの描画
    void Finalize();    // デフォルトカーソルを再表示

private:
    int mOpenHandle = -1;   // 箸が開いている状態
    int mCloseHandle = -1; // 箸が閉じている状態
};