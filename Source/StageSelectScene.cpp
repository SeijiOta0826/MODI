#include "StageSelectScene.h"

#include "UI.h"
#include "UIManager.h"
#include "ItemIconUI.h"
#include "FocusUI.h"
#include "DifficultySelectUI.h"

#include "ItemTable.h"
#include "ItemRecord.h"
#include "GameProgress.h"

#include "Master.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "SoundManager.h"
#include "FontManager.h"

#include "ScreenConfig.h"

#include <algorithm>	// std::min
#include <cstring>		// std::strlen

namespace
{
	// -- 画像パス -- //
	constexpr const char* BACKGROUND_PATH = "Resource/UI/StageSelect/CuttingBoard.jpg";
	constexpr const char* RECIPE_BOOK_PATH = "Resource/UI/StageSelect/RecipeBook.png";
	constexpr const char* DISH_PANEL_PATH = "Resource/UI/Common/Overlay.png";
	constexpr const char* ITEM_PANEL_PATH = "Resource/UI/StageSelect/Circle.png";
	constexpr const char* ESC_KEY_PATH = "Resource/UI/Common/Esc-Key.png";

	// -- 文字列 -- //
	constexpr const char* TITLE_TEXT = "ステージ選択";
	constexpr const char* ESC_HUD_TEXT = "   タイトルへ戻る";

	// -- シークレット表示 -- //
	constexpr const char* SECRET_NAME = "???";
	constexpr const char* SECRET_DISH = "?";

	// -- レイヤー -- //
	constexpr int LAYER_BACKGROUND = 0;	// まな板
	constexpr int LAYER_BOOK = 1;		// レシピ本
	constexpr int LAYER_CARD = 2;		// 料理枠(Overlay)
	constexpr int LAYER_PANEL = 3;		// アイテム枠(Circle)
	constexpr int LAYER_CONTENT = 4;	// 料理名・料理画像・アイテム画像
	constexpr int LAYER_FOCUS = 5;		// フォーカス枠(料理パネルの中身より上)
	constexpr int LAYER_HUD = 6;		// タイトル・ESC案内(難易度選択のポップアップ(20~)より下)

	// -- レイアウト座標系 -- //
	// 位置・大きさはすべて「RecipeBook.png(942×527)上のピクセル座標」で書き、
	// 画面(1920×1080)へは LAYOUT_SCALE を掛けて変換する
	constexpr float LAYOUT_WIDTH = 942.0f;
	constexpr float LAYOUT_HEIGHT = 527.0f;
	constexpr float LAYOUT_SCALE_X = ScreenConfig::SCREEN_WIDTH / LAYOUT_WIDTH;
	constexpr float LAYOUT_SCALE_Y = ScreenConfig::SCREEN_HEIGHT / LAYOUT_HEIGHT;

	// -- タイトル -- //
	constexpr float TITLE_CENTER_X = 471.0f;
	constexpr float TITLE_CENTER_Y = 47.0f;

	// -- 料理枠 -- //
	// 並びは左ページ 1,2 / 3,4、右ページ 5,6 / 7,8
	constexpr float STAGE_CENTERS[GameConfig::STAGE_COUNT][2] = {
		{ 242.5f, 206.0f }, { 385.0f, 206.0f },	// Stage1, 2
		{ 242.5f, 410.5f }, { 390.5f, 410.5f },	// Stage3, 4
		{ 546.5f, 207.0f }, { 688.5f, 207.0f },	// Stage5, 6
		{ 546.5f, 411.0f }, { 690.5f, 411.5f },	// Stage7, 8
	};

	constexpr float CARD_WIDTH = 117.0f;
	constexpr float CARD_HEIGHT = 174.0f;
	constexpr int CARD_ALPHA = 75;	// Overlay.png(黒・不透明度約55%)をさらに薄め、完成イメージの明るい灰色に合わせる

	constexpr float NAME_OFFSET_Y = -63.0f;	// 枠の中心からのずれ
	constexpr float DISH_OFFSET_Y = -3.0f;
	constexpr float ITEM_OFFSET_Y = 62.0f;
	constexpr float ITEM_SPACING = 37.0f;	// アイテム枠どうしの間隔

	constexpr float DISH_IMAGE_WIDTH = 100.0f;	// 料理画像を収める大きさ
	constexpr float DISH_IMAGE_HEIGHT = 75.0f;

	constexpr float ITEM_PANEL_SIZE = 32.0f;
	constexpr float ITEM_IMAGE_SIZE = 26.0f;

	// -- ESC案内(HUD) : ここだけ画面のピクセル座標 -- //
	constexpr float HUD_MARGIN_X = 40.0f;		// 画面右端からの余白
	constexpr float HUD_MARGIN_Y = 50.0f;		// 画面下端から文字の中心まで
	constexpr float HUD_GAP = 12.0f;			// キー画像と文字の間
	constexpr float ESC_KEY_HEIGHT = 52.0f;		// キー画像を表示する高さ(元画像 50×26 の2倍)
}

void StageSelectScene::Initialize()
{
	CreateBackground();
	CreateRecipeBook();
	CreateTitle();
	CreateEscHud();

	for (int stage = 1; stage <= GameConfig::STAGE_COUNT; ++stage)
		CreateStageCard(stage);

	// -- キーボード選択(FocusUI) -- //
	// 並びは画面上の位置から自動で決まるので、ステージ番号順に登録するだけでよい
	mpFocus = GetUIManager()->CreateUI<FocusUI>();
	mpFocus->SetLayer(LAYER_FOCUS);
	for (UI* panel : mpDishPanels)
	{
		panel->GetButton().SetDecideSE(false);	// SE_SELECTUI はシーン側で鳴らす
		mpFocus->Add(panel);
	}
	mpFocus->ResetFocus();

	// -- 難易度選択のポップアップ(ステージを選ぶまでは隠しておく) -- //
	mpDifficultyUI = GetUIManager()->CreateUI<DifficultySelectUI>();
	mpDifficultyUI->SetEnabled(false);
	mState = State::SelectStage;

	if (!SoundManager::GetInstance()->IsPlayingBGM(SoundManager::SOUND_BGM::BGM_Title))
		SoundManager::GetInstance()->PlayBGM(SoundManager::SOUND_BGM::BGM_Title);
}

void StageSelectScene::Update(float _deltaTime)
{
	switch (mState)
	{
	case State::SelectStage:      UpdateSelectStage();      break;
	case State::SelectDifficulty: UpdateSelectDifficulty(); break;
	case State::Starting:         break;	// シーン切り替え待ち
	}

	Scene::Update(_deltaTime);
}

void StageSelectScene::UpdateSelectStage()
{
	for (int i = 0; i < GameConfig::STAGE_COUNT; ++i)
	{
		UI* panel = mpDishPanels[i];
		if (panel == nullptr || !panel->GetButton().IsClicked())
			continue;

		// Confirm を消費し、共通のクリックSE判定をスキップ
		// FocusDecide も消費する(Space は両方に割り当てられているので、
		// 開いたポップアップの「Easy」まで同じフレームで押されてしまうのを防ぐ)
		InputManager::GetInstance().ConsumeButton(Button::Confirm);
		InputManager::GetInstance().ConsumeButton(Button::FocusDecide);
		SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_SELECTUI);

		OpenDifficultySelect(i + 1);
		break;
	}
}

void StageSelectScene::UpdateSelectDifficulty()
{
	switch (mpDifficultyUI->GetResult())
	{
	case DifficultySelectUI::Result::Decided:
		GameProgress::SetDifficulty(mpDifficultyUI->GetSelectedDifficulty());
		Master::mpSceneManager->RequestGameScene(mnSelectedStage);
		mState = State::Starting;
		break;

	case DifficultySelectUI::Result::Back:
		CloseDifficultySelect();
		break;

	case DifficultySelectUI::Result::None:
		break;
	}
}

void StageSelectScene::OpenDifficultySelect(int _stage)
{
	mnSelectedStage = _stage;
	mState = State::SelectDifficulty;

	// 後ろのステージ選択は操作できないようにする
	mpFocus->SetEnabled(false);

	mpDifficultyUI->SetEnabled(true);
	mpDifficultyUI->OnOpen();
}

void StageSelectScene::CloseDifficultySelect()
{
	mState = State::SelectStage;

	mpDifficultyUI->SetEnabled(false);

	// 戻ったときは、さっき選んだステージを選んだままにする
	mpFocus->SetEnabled(true);
	mpFocus->SetFocus(mpDishPanels[mnSelectedStage - 1]);
}

void StageSelectScene::Draw()
{
	Scene::Draw();
}

void StageSelectScene::Finalize()
{
	// タイトルBGMを停止(旧StageSelectSceneと同じ)
	SoundManager::GetInstance()->StopBGM();

	mpBackground = nullptr;
	mpDishPanels.fill(nullptr);
	mpFocus = nullptr;
	mpDifficultyUI = nullptr;

	Scene::Finalize();
}

void StageSelectScene::CreateBackground()
{
	mpBackground = GetUIManager()->CreateUI<UI>();
	mpBackground->LoadUIGraph(BACKGROUND_PATH);
	mpBackground->SetLayer(LAYER_BACKGROUND);

	// 画面いっぱいに引き伸ばす
	auto& transform = mpBackground->GetTransform();
	const VECTOR size = transform.GetSize();
	if (size.x > 0.0f && size.y > 0.0f)
	{
		transform.SetScale(VGet(
			ScreenConfig::SCREEN_WIDTH / size.x,
			ScreenConfig::SCREEN_HEIGHT / size.y,
			0.0f
		));
	}

	mpBackground->SetEnabled(true);
}

void StageSelectScene::CreateRecipeBook()
{
	// RecipeBook.png はレイアウトと同じ 942×527 のキャンバスなので、画面全体に重ねる
	UI* book = GetUIManager()->CreateUI<UI>();
	book->LoadUIGraph(RECIPE_BOOK_PATH);
	PlaceImage(book, LAYOUT_WIDTH * 0.5f, LAYOUT_HEIGHT * 0.5f, LAYER_BOOK);
	StretchImage(book, LAYOUT_WIDTH, LAYOUT_HEIGHT);
	book->SetEnabled(true);
}

void StageSelectScene::CreateTitle()
{
	// 画像を持たない、文字だけのUI
	UI* title = GetUIManager()->CreateUI<UI>();
	title->SetLayer(LAYER_HUD);
	title->GetTransform().SetPosition(ToUIPosition(TITLE_CENTER_X, TITLE_CENTER_Y));

	UIText& text = title->GetText();
	text.SetFontId(FontId::Title);
	text.SetColor(GetColor(0, 0, 0));
	text.SetUnderline(true);
	text.SetText(TITLE_TEXT);

	title->SetEnabled(true);
}

void StageSelectScene::CreateEscHud()
{
	// 右下を基準(Anchor = (1,1))にして、右から「文字」→「キー画像」の順に並べる
	const VECTOR bottomRight = VGet(1.0f, 1.0f, 0.0f);

	// -- 説明文(右端揃え) -- //
	UI* label = GetUIManager()->CreateUI<UI>();
	label->SetLayer(LAYER_HUD);
	label->GetTransform().SetAnchor(bottomRight);
	label->GetTransform().SetPosition(VGet(-HUD_MARGIN_X, -HUD_MARGIN_Y, 0.0f));

	UIText& text = label->GetText();
	text.SetFontId(FontId::HUD);
	text.SetColor(GetColor(255, 255, 255));
	text.SetEdgeColor(GetColor(0, 0, 0));
	text.SetAlign(UITextAlign::RIGHT);
	text.SetText(ESC_HUD_TEXT);

	label->SetEnabled(true);

	// -- ESCキー画像(説明文の左隣) -- //
	// 文字の幅はフォントで変わるので、実際の描画幅を測ってから置く
	const int fontHandle = Master::mpFontManager->GetHandle(FontId::HUD);
	const float textWidth = static_cast<float>(GetDrawStringWidthToHandle(
		ESC_HUD_TEXT,
		static_cast<int>(std::strlen(ESC_HUD_TEXT)),
		fontHandle
	));

	UI* key = GetUIManager()->CreateUI<UI>();
	key->LoadUIGraph(ESC_KEY_PATH);
	key->SetLayer(LAYER_HUD);

	auto& transform = key->GetTransform();
	const VECTOR size = transform.GetSize();
	float keyWidth = size.x;
	if (size.y > 0.0f)
	{
		const float scale = ESC_KEY_HEIGHT / size.y;	// 高さを揃えて、比率は保つ
		transform.SetScale(VGet(scale, scale, 0.0f));
		keyWidth = size.x * scale;
	}

	transform.SetAnchor(bottomRight);
	transform.SetPosition(VGet(
		-(HUD_MARGIN_X + textWidth + HUD_GAP + keyWidth * 0.5f),
		-HUD_MARGIN_Y,
		0.0f
	));

	key->SetEnabled(true);
}

void StageSelectScene::CreateStageCard(int _stage)
{
	const Point center = { STAGE_CENTERS[_stage - 1][0], STAGE_CENTERS[_stage - 1][1] };
	const bool isCompleted = ItemRecord::GetInstance().IsAllObtained(_stage);	// 3つ揃ったか

	CreateDish(_stage, center, isCompleted);
	CreateDishName(_stage, center, isCompleted);
	CreateItems(_stage, center);
}

void StageSelectScene::CreateDishName(int _stage, Point _center, bool _isCompleted)
{
	UI* name = GetUIManager()->CreateUI<UI>();
	name->SetLayer(LAYER_CONTENT);
	name->GetTransform().SetPosition(ToUIPosition(_center.x, _center.y + NAME_OFFSET_Y));

	UIText& text = name->GetText();
	text.SetFontId(FontId::SubTitle);
	text.SetColor(GetColor(0, 0, 0));
	text.SetUnderline(true);	// AllClear / notClear と同じく下線付き
	text.SetText(_isCompleted ? ItemTable::GetDishName(_stage) : SECRET_NAME);

	name->SetEnabled(true);
}

void StageSelectScene::CreateDish(int _stage, Point _center, bool _isCompleted)
{
	// -- 料理枠(Overlay。クリック判定もこれで行う) -- //
	UI* panel = GetUIManager()->CreateUI<UI>();
	panel->LoadUIGraph(DISH_PANEL_PATH);
	PlaceImage(panel, _center.x, _center.y, LAYER_CARD);
	StretchImage(panel, CARD_WIDTH, CARD_HEIGHT);
	panel->GetGraphic().SetAlpha(CARD_ALPHA);
	panel->SetEnabled(true);

	mpDishPanels[_stage - 1] = panel;

	// -- 料理画像(未完成なら真っ黒なシルエット + 「?」) -- //
	const std::string path = ItemTable::GetDishFilePath(_stage);
	if (path.empty())
		return;

	const float y = _center.y + DISH_OFFSET_Y;

	ItemIconUI* dish = GetUIManager()->CreateUI<ItemIconUI>();
	dish->LoadUIGraph(path);
	PlaceImage(dish, _center.x, y, LAYER_CONTENT);
	FitImage(dish, DISH_IMAGE_WIDTH, DISH_IMAGE_HEIGHT);

	dish->SetShadowBright(0);
	dish->SetPicked(_isCompleted);

	if (!_isCompleted)
	{
		UIText& text = dish->GetText();
		text.SetFontId(FontId::Button);
		text.SetColor(GetColor(255, 255, 255));
		text.SetText(SECRET_DISH);
	}

	dish->SetEnabled(true);
}

void StageSelectScene::CreateItems(int _stage, Point _center)
{
	const float y = _center.y + ITEM_OFFSET_Y;

	// 真ん中の枠が中心に来るようにずらす（3個なら -1, 0, +1 倍）
	constexpr float CENTER_INDEX = (GameConfig::MAX_ITEMS_PER_STAGE - 1) / 2.0f;

	for (int slot = 0; slot < GameConfig::MAX_ITEMS_PER_STAGE; ++slot)
	{
		const float x = _center.x + (slot - CENTER_INDEX) * ITEM_SPACING;

		// -- アイテム枠(Circle。常に表示) -- //
		UI* panel = GetUIManager()->CreateUI<UI>();
		panel->LoadUIGraph(ITEM_PANEL_PATH);
		PlaceImage(panel, x, y, LAYER_PANEL);
		FitImage(panel, ITEM_PANEL_SIZE, ITEM_PANEL_SIZE);
		panel->SetEnabled(true);

		// -- アイテム画像(取得済みのものだけ) -- //
		if (!ItemRecord::GetInstance().IsObtained(_stage, slot))
			continue;

		const std::string path = ItemTable::GetFilePath(_stage, slot);
		if (path.empty())
			continue;

		UI* item = GetUIManager()->CreateUI<UI>();
		item->LoadUIGraph(path);
		PlaceImage(item, x, y, LAYER_CONTENT);
		FitImage(item, ITEM_IMAGE_SIZE, ITEM_IMAGE_SIZE);
		item->SetEnabled(true);
	}
}

void StageSelectScene::PlaceImage(UI* _ui, float _x, float _y, int _layer) const
{
	_ui->SetLayer(_layer);
	_ui->GetTransform().SetPosition(ToUIPosition(_x, _y));
}

void StageSelectScene::FitImage(UI* _ui, float _width, float _height) const
{
	auto& transform = _ui->GetTransform();
	const VECTOR size = transform.GetSize();
	if (size.x <= 0.0f || size.y <= 0.0f)
		return;

	// 縦・横それぞれ「枠に収まる倍率」を出し、小さい方を両方に使う(= はみ出さない)
	const float scale = (std::min)(
		_width * LAYOUT_SCALE_X / size.x,
		_height * LAYOUT_SCALE_Y / size.y
		);
	transform.SetScale(VGet(scale, scale, 0.0f));
}

void StageSelectScene::StretchImage(UI* _ui, float _width, float _height) const
{
	auto& transform = _ui->GetTransform();
	const VECTOR size = transform.GetSize();
	if (size.x <= 0.0f || size.y <= 0.0f)
		return;

	transform.SetScale(VGet(
		_width * LAYOUT_SCALE_X / size.x,
		_height * LAYOUT_SCALE_Y / size.y,
		0.0f
	));
}

VECTOR StageSelectScene::ToUIPosition(float _x, float _y) const
{
	return VGet(
		_x * LAYOUT_SCALE_X - ScreenConfig::SCREEN_WIDTH * 0.5f,
		_y * LAYOUT_SCALE_Y - ScreenConfig::SCREEN_HEIGHT * 0.5f,
		0.0f
	);
}