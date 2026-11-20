#pragma once
#include "UI.h"

class FocusUI;

class OperationGuideUI : public UI
{
public:
    void Init() override;
    void Update() override;

    // 戻るボタンが押されたか（PauseUIが確認する）
    bool IsBackRequested() const { return mbBackRequested; }

    // 開いたときに呼ぶ(前回の「戻る」要求を消し、フォーカスを「戻る」に合わせる)
    void OnOpen();

private:
    // 操作説明をしているUIを生成する純粋関数
    UI* CreateGuideUI(const std::string _filename, const std::string _text, const VECTOR _offset);

private:
    UI* mpGuideA;
    UI* mpGuideD;
    UI* mpGuideE;
    UI* mpGuideP;
    UI* mpBackButton = nullptr;
    FocusUI* mpFocus = nullptr;     // 「戻る」をキーボードで押せるようにする
    bool mbBackRequested = false;
};