#include "DifficultySelectUI.h"

#include "UIManager.h"
#include "FocusUI.h"

namespace
{
    constexpr const char* OVERLAY_PATH = "Resource/UI/Overlay/Overlay_black.png";
    constexpr const char* BUTTON_PATH = "Resource/UI/Pause/Pause_button.png";

    // -- レイヤー(StageSelectScene の UI(0～4) より上) -- //
    constexpr int LAYER_OVERLAY = 20;
    constexpr int LAYER_CONTENT = 21;
    constexpr int LAYER_FOCUS = 22;

    // -- 配置(画面中央からのずれ) -- //
    constexpr float TITLE_POS_Y = -280.0f;
    constexpr float BUTTON_POS_X = 300.0f;      // Easy は左(-)、Normal は右(+)
    constexpr float BUTTON_POS_Y = -40.0f;
    constexpr float DESC_OFFSET_Y = 130.0f;     // ボタンから説明文までの距離
    constexpr float BACK_POS_Y = 350.0f;

    constexpr int OVERLAY_ALPHA = 180;          // 後ろのステージ選択が少し透けて見える
}

void DifficultySelectUI::Init()
{
    SetLayer(LAYER_OVERLAY);

    // -- 黒い半透明の背景(後ろのステージ選択を暗くする) -- //
    mpOverlay = GetUIManager()->CreateUI<UI>();
    mpOverlay->LoadUIGraph(OVERLAY_PATH);
    mpOverlay->GetGraphic().SetAlpha(OVERLAY_ALPHA);
    mpOverlay->SetLayer(LAYER_OVERLAY);
    mpOverlay->SetEnabled(true);
    AddChild(mpOverlay);
    mpOverlay->SetParent(this);

    // -- 見出し -- //
    mpTitle = CreateTextUI("難易度を選んでください", FontId::Title, VGet(0.0f, TITLE_POS_Y, 0.0f));

    // -- 難易度ボタン + 説明文 -- //
    mpEasyButton = CreateButton("Easy", VGet(-BUTTON_POS_X, BUTTON_POS_Y, 0.0f));
    CreateTextUI("ミスしても\n取ったアイテムはそのまま", FontId::Message,
        VGet(-BUTTON_POS_X, BUTTON_POS_Y + DESC_OFFSET_Y, 0.0f));

    mpNormalButton = CreateButton("Normal", VGet(BUTTON_POS_X, BUTTON_POS_Y, 0.0f));
    CreateTextUI("ミスすると\n取ったアイテムもリセット", FontId::Message,
        VGet(BUTTON_POS_X, BUTTON_POS_Y + DESC_OFFSET_Y, 0.0f));

    // -- 戻る -- //
    mpBackButton = CreateButton("戻る", VGet(0.0f, BACK_POS_Y, 0.0f));

    // -- キーボード選択(FocusUI) -- //
    mpFocus = GetUIManager()->CreateUI<FocusUI>();
    mpFocus->SetLayer(LAYER_FOCUS);
    AddChild(mpFocus);
    mpFocus->SetParent(this);
    mpFocus->Add(mpEasyButton);     // 先頭 = 既定のフォーカス
    mpFocus->Add(mpNormalButton);
    mpFocus->Add(mpBackButton);
}

void DifficultySelectUI::OnOpen()
{
    // 閉じている間は Update() が呼ばれないので、前回の結果が残ったままになるのを防ぐ
    mResult = Result::None;

    // 前回遊んだ難易度を選んだ状態から始める(続けて同じ難易度で遊びやすいように)
    mpFocus->ResetFocus();
    if (GameProgress::GetDifficulty() == Difficulty::Normal)
        mpFocus->SetFocus(mpNormalButton);
}

void DifficultySelectUI::Update()
{
    UI::Update();

    // memo : IsClicked() は1フレームに1回だけ呼ぶ(Space決定が1回目で消費されるため)
    mResult = Result::None;

    if (mpEasyButton->GetButton().IsClicked())
    {
        mSelected = Difficulty::Easy;
        mResult = Result::Decided;
    }
    else if (mpNormalButton->GetButton().IsClicked())
    {
        mSelected = Difficulty::Normal;
        mResult = Result::Decided;
    }
    else if (mpBackButton->GetButton().IsClicked())
    {
        mResult = Result::Back;
    }
}

UI* DifficultySelectUI::CreateButton(const char* _label, const VECTOR& _position)
{
    UI* button = GetUIManager()->CreateUI<UI>();
    button->LoadUIGraph(BUTTON_PATH);
    button->GetTransform().SetPosition(_position);
    button->GetText().SetFontId(FontId::SubTitle);
    button->GetText().SetText(_label);
    button->GetHoverScaler().SetActive(true);   // 選ばれていることが分かるように拡大演出を付ける
    button->SetLayer(LAYER_CONTENT);
    button->SetEnabled(true);
    AddChild(button);
    button->SetParent(this);
    return button;
}

UI* DifficultySelectUI::CreateTextUI(const std::string& _text, FontId _font, const VECTOR& _position)
{
    // 画像なしで文字だけ出すUI
    UI* ui = GetUIManager()->CreateUI<UI>();
    ui->GetTransform().SetPosition(_position);
    ui->GetText().SetFontId(_font);
    ui->GetText().SetText(_text);
    ui->SetLayer(LAYER_CONTENT);
    ui->SetEnabled(true);
    AddChild(ui);
    ui->SetParent(this);
    return ui;
}
