#include "ConfigUI.h"

#include "UIManager.h"
#include "FocusUI.h"
#include "SoundManager.h"

#include <DxLib.h>
#include <string>

namespace
{
    // -- 配置(画面中央からのずれ) -- //
    constexpr float TITLE_POS_Y = -300.0f;
    constexpr float BGM_ROW_POS_Y = -80.0f;
    constexpr float SE_ROW_POS_Y = 80.0f;
    constexpr float BACK_POS_Y = 400.0f;

    constexpr float LABEL_POS_X = -450.0f;
    constexpr float DOWN_POS_X = -250.0f;
    constexpr float UP_POS_X = 250.0f;
    constexpr float VALUE_POS_X = 430.0f;

    // -- 「－」「＋」ボタン(Pause_button.png を小さくして流用) -- //
    const VECTOR STEP_BUTTON_SCALE = VGet(0.35f, 0.9f, 0.0f);

    // -- ゲージ(豆腐) -- //
    const char* const TOFU_GRAPH_PATH = "Resource/UI/Pause/Config/Tofu.png";
    constexpr float GAUGE_MARGIN_X = 85.0f;     // ボタンの中心からゲージの端までの距離
    constexpr float TOFU_DRAW_SIZE = 30.0f;     // 豆腐1個を何pxで描くか(画像の大きさから拡大率を計算する)
    constexpr int   TOFU_BRIGHT_ON = 255;       // 段階に達している豆腐 = 元の色
    constexpr int   TOFU_BRIGHT_OFF = 60;       // 段階に達していない豆腐 = 暗くする

    constexpr int LAYER_CONFIG = 4;             // 黒オーバーレイ(0)より上
}

void ConfigUI::Init()
{
    SetLayer(LAYER_CONFIG);

    // -- 見出し -- //
    mpTitle = CreateTextUI("設定", FontId::Title, VGet(0.0f, TITLE_POS_Y, 0.0f));

    // -- 音量の行 -- //
    mBgmRow = CreateVolumeRow("BGM", BGM_ROW_POS_Y);
    mSeRow = CreateVolumeRow("SE", SE_ROW_POS_Y);

    // -- 「戻る」ボタン(OperationGuideUIと同じ見た目) -- //
    mpBackButton = GetUIManager()->CreateUI<UI>();
    mpBackButton->LoadUIGraph("Resource/UI/Pause/Pause_button.png");
    mpBackButton->GetText().SetFontId(FontId::SubTitle);
    mpBackButton->GetText().SetText("戻る");
    mpBackButton->SetLayer(LAYER_CONFIG);
    mpBackButton->GetTransform().SetPosition(VGet(0.0f, BACK_POS_Y, 0.0f));
    mpBackButton->GetHoverScaler().SetActive(true);
    mpBackButton->SetEnabled(true);
    AddChild(mpBackButton);
    mpBackButton->SetParent(this);

    // -- キーボード選択(FocusUI) -- //
    // 方向キーで画面上の位置から自動で移動先が決まるので、登録順は「最初に選ばれるもの」だけ気にすればよい
    mpFocus = GetUIManager()->CreateUI<FocusUI>();
    AddChild(mpFocus);
    mpFocus->SetParent(this);
    mpFocus->Add(mBgmRow.pDownButton);  // 先頭 = 開いたときに選ばれる
    mpFocus->Add(mBgmRow.pUpButton);
    mpFocus->Add(mSeRow.pDownButton);
    mpFocus->Add(mSeRow.pUpButton);
    mpFocus->Add(mpBackButton);

    // -- 今の音量を表示に反映 -- //
    auto* sound = SoundManager::GetInstance();
    RefreshRow(mBgmRow, sound->GetBGMVolume());
    RefreshRow(mSeRow, sound->GetSEVolume());
}

void ConfigUI::OnOpen()
{
    // 閉じている間は Update() が呼ばれないので、前回の true が残ったままになるのを防ぐ
    mbBackRequested = false;
    mpFocus->ResetFocus();

    // 別の場所で音量が変わっていても正しく出るように、開くたびに表示を合わせる
    auto* sound = SoundManager::GetInstance();
    RefreshRow(mBgmRow, sound->GetBGMVolume());
    RefreshRow(mSeRow, sound->GetSEVolume());
}

void ConfigUI::Update()
{
    UI::Update();

    auto* sound = SoundManager::GetInstance();

    const int bgm = UpdateVolumeRow(mBgmRow, sound->GetBGMVolume());
    if (bgm != sound->GetBGMVolume())
    {
        sound->SetBGMVolume(bgm);
        RefreshRow(mBgmRow, sound->GetBGMVolume());
    }

    const int se = UpdateVolumeRow(mSeRow, sound->GetSEVolume());
    if (se != sound->GetSEVolume())
    {
        sound->SetSEVolume(se);
        RefreshRow(mSeRow, sound->GetSEVolume());
    }

    mbBackRequested = mpBackButton->GetButton().IsClicked();
}

int ConfigUI::UpdateVolumeRow(const VolumeRow& _row, int _level)
{
    // memo : IsClicked() は1フレームに1回だけ呼ぶ(Space決定が1回目で消費されるため)
    if (_row.pDownButton->GetButton().IsClicked())
        return _level - 1;  // 範囲外の値は SoundManager 側で 0～最大 に収める

    if (_row.pUpButton->GetButton().IsClicked())
        return _level + 1;

    return _level;
}

void ConfigUI::RefreshRow(const VolumeRow& _row, int _level)
{
    const int percent = _level * 100 / SoundManager::VOLUME_LEVEL_MAX;
    _row.pValue->GetText().SetText(std::to_string(percent) + "%");

    // 左から _level 個までを明るく、残りを暗くする
    for (int i = 0; i < SoundManager::VOLUME_LEVEL_MAX; ++i)
    {
        const int bright = (i < _level) ? TOFU_BRIGHT_ON : TOFU_BRIGHT_OFF;
        _row.pGauge[i]->GetGraphic().SetBright(bright);
    }
}

// ------------------------------------------------------------
// 生成
// ------------------------------------------------------------
ConfigUI::VolumeRow ConfigUI::CreateVolumeRow(const char* _label, float _posY)
{
    VolumeRow row;
    row.pLabel = CreateTextUI(_label, FontId::SubTitle, VGet(LABEL_POS_X, _posY, 0.0f));
    row.pDownButton = CreateStepButton("－", VGet(DOWN_POS_X, _posY, 0.0f));
    row.pUpButton = CreateStepButton("＋", VGet(UP_POS_X, _posY, 0.0f));
    row.pValue = CreateTextUI("", FontId::Button, VGet(VALUE_POS_X, _posY, 0.0f));
    CreateGauge(row, _posY);
    return row;
}

void ConfigUI::CreateGauge(VolumeRow& _row, float _posY)
{
    // 「－」と「＋」の間を段階の数で等分し、各マスの中央に豆腐を置く
    const float startX = DOWN_POS_X + GAUGE_MARGIN_X;
    const float endX = UP_POS_X - GAUGE_MARGIN_X;
    const float cellWidth = (endX - startX) / SoundManager::VOLUME_LEVEL_MAX;

    for (int i = 0; i < SoundManager::VOLUME_LEVEL_MAX; ++i)
    {
        UI* tofu = GetUIManager()->CreateUI<UI>();
        tofu->LoadUIGraph(TOFU_GRAPH_PATH);

        // 画像の大きさが変わっても TOFU_DRAW_SIZE で描かれるように拡大率を求める
        const VECTOR size = tofu->GetTransform().GetSize();
        const float scale = (size.x > 0.0f) ? TOFU_DRAW_SIZE / size.x : 1.0f;
        tofu->GetTransform().SetScale(VGet(scale, scale, 0.0f));

        const float x = startX + cellWidth * (i + 0.5f);
        tofu->GetTransform().SetPosition(VGet(x, _posY, 0.0f));

        tofu->SetLayer(LAYER_CONFIG);
        tofu->SetEnabled(true);
        AddChild(tofu);
        tofu->SetParent(this);

        _row.pGauge[i] = tofu;
    }
}

UI* ConfigUI::CreateTextUI(const std::string& _text, FontId _font, const VECTOR& _position)
{
    // 画像なしで文字だけ出すUI
    UI* ui = GetUIManager()->CreateUI<UI>();
    ui->GetTransform().SetPosition(_position);
    ui->GetText().SetFontId(_font);
    ui->GetText().SetText(_text);
    ui->SetLayer(LAYER_CONFIG);
    ui->SetEnabled(true);
    AddChild(ui);
    ui->SetParent(this);
    return ui;
}

UI* ConfigUI::CreateStepButton(const char* _text, const VECTOR& _position)
{
    UI* button = GetUIManager()->CreateUI<UI>();
    button->LoadUIGraph("Resource/UI/Pause/Pause_button.png");
    button->GetTransform().SetPosition(_position);
    button->GetTransform().SetScale(STEP_BUTTON_SCALE);
    button->GetText().SetFontId(FontId::SubTitle);
    button->GetText().SetText(_text);
    button->GetHoverScaler().SetActive(true);
    button->SetLayer(LAYER_CONFIG);
    button->SetEnabled(true);
    AddChild(button);
    button->SetParent(this);
    return button;
}
