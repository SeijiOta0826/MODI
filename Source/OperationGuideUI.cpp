#include "OperationGuideUI.h"
#include "UIManager.h"
#include "FocusUI.h"

void OperationGuideUI::Init()
{
    // -- 「A」ボタンの生成 -- //
    mpGuideA = CreateGuideUI(
        "Resource/UI/Pause/OperationGuide/A-Key.png",
        "左回転",
        VGet(-125.0f, -100.0f, 0.0f)
    );

    // -- 「D」ボタンの生成 -- //
    mpGuideD = CreateGuideUI(
        "Resource/UI/Pause/OperationGuide/D-Key.png",
        "右回転",
        VGet(-125.0f, 0.0f, 0.0f)
    );

    // -- 「E」ボタンの生成 -- //
    mpGuideE = CreateGuideUI(
        "Resource/UI/Pause/OperationGuide/E-Key.png",
        "変形！",
        VGet(-125.0f, 100.0f, 0.0f)
    );

    // -- 「P」ボタンの生成 -- //
    mpGuideP = CreateGuideUI(
        "Resource/UI/Pause/OperationGuide/P-Key.png",
        "一時停止",
        VGet(-125.0f, 200.0f, 0.0f)
    );
    mpGuideP->GetText().SetOffset(
        VGet(
            375.0f, 0.0f, 0.0f
        )
    );

    // -- 「戻る」ボタンの生成 -- //
        // memo : これはフォントや画像サイズの関係で、共通化できるとこが少ないのでベタ書き
    mpBackButton = GetUIManager()->CreateUI<UI>();
    mpBackButton->LoadUIGraph("Resource/UI/Pause/Pause_button.png");
    mpBackButton->GetText().SetFontId(FontId::SubTitle);
    mpBackButton->GetText().SetText("戻る");
    mpBackButton->SetLayer(4);
    mpBackButton->GetTransform().SetPosition(VGet(0.0f, 400.0f, 0.0f));
    mpBackButton->GetHoverScaler().SetActive(true);    // 選ばれていることが分かるように拡大演出を付ける
    mpBackButton->SetEnabled(true);
    AddChild(mpBackButton);
    mpBackButton->SetParent(this);

    // -- キーボード選択(FocusUI) -- //
    // 選べるのは「戻る」だけだが、Spaceで押せるようにするために登録する
    mpFocus = GetUIManager()->CreateUI<FocusUI>();
    AddChild(mpFocus);
    mpFocus->SetParent(this);
    mpFocus->Add(mpBackButton);
}

void OperationGuideUI::OnOpen()
{
    // 閉じている間は Update() が呼ばれないので、前回の true が残ったままになるのを防ぐ
    mbBackRequested = false;
    mpFocus->ResetFocus();
}

void OperationGuideUI::Update()
{
    UI::Update();
    mbBackRequested = mpBackButton->GetButton().IsClicked();
}

UI* OperationGuideUI::CreateGuideUI(const std::string _filename, const std::string _text, const VECTOR _offset)
{
    auto button = GetUIManager()->CreateUI<UI>();
    button->LoadUIGraph(_filename);
    button->GetTransform().SetScale(VGet(1.8f, 1.8f, 0.0f));
    button->GetTransform().SetPosition(_offset);
    button->GetText().SetAlign(UITextAlign::RIGHT);
    button->GetText().SetOffset(VGet(300.0f, 0.0f, 0.0f));
    button->GetText().SetFontId(FontId::Button);
    button->GetText().SetText(_text);
    button->SetEnabled(true);
    AddChild(button);
    button->SetParent(this);

    return button;
}