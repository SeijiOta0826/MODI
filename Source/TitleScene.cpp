#include "TitleScene.h"
// #include <DxLib.h>

#include "UI.h"
#include "UIManager.h"
#include "FocusUI.h"

#include "Master.h"
#include "SceneManager.h"

#include "InputManager.h"
#include "SoundManager.h"

#include "GameProgress.h"	// IsTutorialCleared()

#include "ScreenConfig.h"	// 背景画像の拡縮
#include "KitchenToolCatalog.h"

#include <algorithm>	// std::max
#include <vector>

namespace
{
	// -- 描画レイヤー(小さいほど奥) -- //
	constexpr int LAYER_BACKGROUND = 0;		// まな板
	constexpr int LAYER_BOOK = 1;			// 本の表紙
	constexpr int LAYER_BOOK_COVER = 2;		// 表紙の豆腐・ロゴ・文字
	constexpr int LAYER_KITCHEN_TOOL = 10;	// 調理器具(10, 11, 12... と器具の数だけ使う)
	constexpr int LAYER_BUTTON = 100;		// Start / Quit(器具より手前)

	const std::string KITCHEN_TOOL_DIR = "Resource/UI/Title/kitchenTools/";

	// -- 動かせる調理器具の配置(画像・当たり判定・基準の大きさは KitchenToolCatalog) -- //
	const std::vector<KitchenToolPlacement> KITCHEN_TOOLS =
	{
		{ KitchenToolType::MeasuringCup,		VGet( 200.0f, 160.0f, 0.0f), -0.20f },
		{ KitchenToolType::WoodenSpatula,		VGet( 390.0f, 420.0f, 0.0f),  0.55f },
		{ KitchenToolType::Otama,				VGet( 120.0f, 790.0f, 0.0f), -0.25f },
		{ KitchenToolType::MeasuringSpoon,		VGet( 400.0f, 915.0f, 0.0f),  0.40f },
		{ KitchenToolType::FryingPan,			VGet(1680.0f, 230.0f, 0.0f), -0.50f },
		{ KitchenToolType::KitchenKnife,		VGet(1660.0f, 590.0f, 0.0f), -0.10f },
		{ KitchenToolType::CookingChopsticks,	VGet(1730.0f, 870.0f, 0.0f),  0.45f },
	};

	// 動かない画像UIを生成する(座標は画面中央からのずれ)
	UI* CreateStaticImage(UIManager* _uiManager, const std::string& _filePath, const VECTOR& _position, float _scale, int _layer)
	{
		UI* ui = _uiManager->CreateUI<UI>();
		ui->LoadUIGraph(_filePath);
		ui->SetEnabled(true);
		ui->SetLayer(_layer);
		ui->GetTransform().SetScale(VGet(_scale, _scale, 0.0f));
		ui->GetTransform().SetPosition(_position);
		return ui;
	}
}

void TitleScene::Initialize()
{
	CreateBackground();
	CreateKitchenTools();
	CreateButtons();

	// -- BGMの再生 -- //
	SoundManager::GetInstance()->PlayBGM(SoundManager::SOUND_BGM::BGM_Title, true);
}

void TitleScene::CreateBackground()
{
	auto uiManager = GetUIManager();

	// -- まな板(画面全体) -- //
	mpBackGround = uiManager->CreateUI<UI>();
	mpBackGround->LoadUIGraph(KITCHEN_TOOL_DIR + "Cutting board.png");
	mpBackGround->SetEnabled(true);
	mpBackGround->SetLayer(LAYER_BACKGROUND);
		// -- 拡縮 : 縦横比を保ったまま画面を覆う(はみ出た分は画面外) -- //
	const VECTOR size = mpBackGround->GetTransform().GetSize();
	const float coverScale = (std::max)(ScreenConfig::SCREEN_WIDTH / size.x, ScreenConfig::SCREEN_HEIGHT / size.y);
	mpBackGround->GetTransform().SetScale(VGet(coverScale, coverScale, 0.0f));

	// -- 本の表紙・豆腐・ロゴ -- //
	mpRecipeBook = CreateStaticImage(uiManager, KITCHEN_TOOL_DIR + "Tofubook.png",	VGet(40.0f, 10.0f, 0.0f),	1.88f, LAYER_BOOK);
	mpTofu = CreateStaticImage(uiManager, KITCHEN_TOOL_DIR + "TitleTofu.png",		VGet(58.0f, 40.0f, 0.0f),	0.91f, LAYER_BOOK_COVER);
	mpLogo = CreateStaticImage(uiManager, KITCHEN_TOOL_DIR + "Logo.png",			VGet(57.0f, -345.0f, 0.0f),	0.94f, LAYER_BOOK_COVER);

	// -- 「豆腐レシピ本」(画像ではなくUITextで描画) -- //
	mpRecipeText = uiManager->CreateUI<UI>();
	mpRecipeText->SetEnabled(true);
	mpRecipeText->SetLayer(LAYER_BOOK_COVER);
	mpRecipeText->GetTransform().SetPosition(VGet(60.0f, -200.0f, 0.0f));
	mpRecipeText->GetText().SetFontId(FontId::Button);					// PixelMplus10 / 70px
	mpRecipeText->GetText().SetColor(GetColor(245, 240, 225));			// ロゴの豆腐と同じクリーム色
	mpRecipeText->GetText().SetEdgeColor(GetColor(245, 240, 225));
	mpRecipeText->GetText().SetUnderline(true);
	mpRecipeText->GetText().SetText("豆腐レシピ本");
}

void TitleScene::CreateKitchenTools()
{
	mKitchenTools.SetBaseLayer(LAYER_KITCHEN_TOOL);

	for (const auto& placement : KITCHEN_TOOLS)
		mKitchenTools.AddTool(GetUIManager(), KitchenToolCatalog::MakeDesc(placement));
}

void TitleScene::CreateButtons()
{
	auto uiManager = GetUIManager();

	// -- 「Start」ボタン -- //
	mpStartButton = uiManager->CreateUI<UI>();
	mpStartButton->LoadUIGraph("Resource/UI/Title/Spoon.png");
	mpStartButton->SetEnabled(true);
	mpStartButton->SetLayer(LAYER_BUTTON);
	mpStartButton->GetHoverScaler().SetActive(true);	// ボタンのホバー演出を適用
	mpStartButton->GetText().SetFontId(FontId::SubTitle);
	mpStartButton->GetText().SetColor(GetColor(255, 255, 255));
	mpStartButton->GetText().SetText("Start!");
	mpStartButton->GetTransform().SetScale(VGet(1.2f, 1.2f, 0.0f));
	mpStartButton->GetTransform().SetPosition(VGet(50.0f, 275.0f, 0.0f));

	// -- 「Quit」ボタン -- //
	mpQuitButton = uiManager->CreateUI<UI>();
	mpQuitButton->LoadUIGraph("Resource/UI/Title/Fork.png");
	mpQuitButton->SetEnabled(true);
	mpQuitButton->SetLayer(LAYER_BUTTON);
	mpQuitButton->GetHoverScaler().SetActive(true);	// ボタンのホバー演出を適用
	mpQuitButton->GetText().SetFontId(FontId::SubTitle);
	mpQuitButton->GetText().SetColor(GetColor(255, 255, 255));
	mpQuitButton->GetText().SetText("Quit");
	mpQuitButton->GetTransform().SetScale(VGet(1.2f, 1.2f, 0.0f));
	mpQuitButton->GetTransform().SetPosition(VGet(50.0f, 375.0f, 0.0f));

	// -- キーボード選択(FocusUI) -- //
	// 決定時はこのシーンで SE_SELECTUI を鳴らすので、ボタン側の決定SEは切る
	mpStartButton->GetButton().SetDecideSE(false);
	mpQuitButton->GetButton().SetDecideSE(false);

	mpFocus = uiManager->CreateUI<FocusUI>();
	mpFocus->Add(mpStartButton);
	mpFocus->Add(mpQuitButton);
	mpFocus->ResetFocus();
}

bool TitleScene::IsMouseOnButton()
{
	return (mpStartButton != nullptr && mpStartButton->GetButton().IsMouseInside())
		|| (mpQuitButton != nullptr && mpQuitButton->GetButton().IsMouseInside());
}

void TitleScene::Update(float _deltaTime)
{
	// -- 調理器具(ボタンの上では新しく掴まない / 押さない) -- //
	mKitchenTools.Update(_deltaTime, !IsMouseOnButton());

	// -- 「Start」ボタンを押した場合 -- //
	if (mpStartButton != nullptr
		&& !mKitchenTools.IsDragging()
		&& mpStartButton->GetButton().IsClicked())
	{
		//  Confirm を消費し、共通のクリックSE判定をスキップ
		InputManager::GetInstance().ConsumeButton(Button::Confirm);
		SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_SELECTUI);

		const SCENE_TYPE next = GameProgress::IsTutorialCleared()
			? SCENE_TYPE::STAGE_SELECT_SCENE
			: SCENE_TYPE::TUTORIAL_SCENE;
		Master::mpSceneManager->RequestScene(next);
	}

	// -- 「Quit」ボタンを押した場合 -- //
	if (mpQuitButton != nullptr
		&& !mKitchenTools.IsDragging()
		&& mpQuitButton->GetButton().IsClicked())
	{
		//  Confirm を消費し、共通のクリックSE判定をスキップ
		InputManager::GetInstance().ConsumeButton(Button::Confirm);
		SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_SELECTUI);

		Master::mpSceneManager->RequestQuit();
	}

	Scene::Update(_deltaTime);
}

void TitleScene::Draw()
{
	Scene::Draw();
}

void TitleScene::Finalize()
{
	mKitchenTools.Clear();
	Scene::Finalize();
}
