#include "PlayerShapeUI.h"

// -- 所有するUI -- //
#include "PlayerPieceUI.h"
#include "FocusUI.h"

// -- UIの生成用 -- //
#include "Master.h"
#include "SceneManager.h"
#include "Scene.h"
#include "UIManager.h"

#include "PauseUI.h"

#include "ObjectManager.h"	// Playerのアドレス取得用
#include "Player.h"

namespace
{
	constexpr float BUTTON_OFFSET_X = 180.0f;	// OK / Back ボタンの左右位置
	constexpr float BUTTON_OFFSET_Y = 300.0f;	// OK / Back ボタンの縦位置
	constexpr int BUTTON_BRIGHT = 170;			// スプーン / フォーク画像の明るさ (255 = 元の色)

	constexpr int LAYER_OVERLAY = 0;
	constexpr int LAYER_FRAME = 1;
	constexpr int LAYER_PIECE = 2;
	constexpr int LAYER_BUTTON = 3;
	constexpr int LAYER_FOCUS = 4;

	// UIの選択状態を形状マスク(画面基準)にまとめる
	PlayerShapeMask MakeShapeFromUI(const PlayerShapeUI& _ui)
	{
		PlayerShapeMask shape{};
		for (int row = 0; row < GameConfig::PLAYER_PIECE_SIZE; ++row)
		{
			for (int column = 0; column < GameConfig::PLAYER_PIECE_SIZE; ++column)
			{
				shape[column][row] = _ui.IsPieceSelected(column, row);
			}
		}
		return shape;
	}
}

void PlayerShapeUI::Init()
{
	// -- 自身の初期設定 -- //
	LoadUIGraph("Resource/UI/ShapeSelect/Shape_panel.png");
	SetLayer(LAYER_FRAME);
	SetEnabled(false);	// 初期状態は非表示

	// -- piece生成 & 初期処理 -- //
	for (int row = 0; row < GameConfig::PLAYER_PIECE_SIZE; ++row)
	{
		for (int column = 0; column < GameConfig::PLAYER_PIECE_SIZE; ++column)
		{
			CreatePieceUI(column, row);
		}
	}

	mpBackGround = GetUIManager()->CreateUI<UI>();
	mpBackGround->LoadUIGraph("Resource/UI/ShapeSelect/BackGround.png");
	mpBackGround->SetLayer(LAYER_FRAME);
	mpBackGround->GetTransform().SetScale(VGet(1.5f, 1.5f, 1.0f));
	mpBackGround->GetTransform().SetPosition(VGet(-60.0f, 0.0f, 0.0f));

	mpBackGround->SetEnabled(true);
	this->AddChild(mpBackGround);
	mpBackGround->SetParent(this);

	// -- 「OK」ボタン生成 -- //
	mpConfirmUI = GetUIManager()->CreateUI<UI>();
	mpConfirmUI->LoadUIGraph("Resource/UI/ShapeSelect/Spoon.png");
	mpConfirmUI->GetGraphic().SetBright(BUTTON_BRIGHT);	// 背景と同化しないよう少し暗く
	mpConfirmUI->SetLayer(LAYER_BUTTON);
	mpConfirmUI->GetTransform().SetScale(VGet(1.0f, 1.0f, 1.0f));
	mpConfirmUI->GetTransform().SetPosition(VGet(-BUTTON_OFFSET_X, BUTTON_OFFSET_Y, 0.0f));
	mpConfirmUI->GetText().SetColor(GetColor(255, 113, 36));
	mpConfirmUI->GetText().SetFontId(FontId::Button);
	mpConfirmUI->GetText().SetText("OK!");
	mpConfirmUI->GetHoverScaler().SetActive(true);

	this->AddChild(mpConfirmUI);
	mpConfirmUI->SetParent(this);
	mpConfirmUI->SetEnabled(true);

	// -- 「Back」ボタン生成 -- //
	mpBackUI = GetUIManager()->CreateUI<UI>();
	mpBackUI->LoadUIGraph("Resource/UI/ShapeSelect/Fork.png");
	mpBackUI->GetGraphic().SetBright(BUTTON_BRIGHT);	// 背景と同化しないよう少し暗く
	mpBackUI->SetLayer(LAYER_BUTTON);
	mpBackUI->GetTransform().SetScale(VGet(1.0f, 1.0f, 1.0f));
	mpBackUI->GetTransform().SetPosition(VGet(BUTTON_OFFSET_X, BUTTON_OFFSET_Y, 0.0f));
	mpBackUI->GetText().SetColor(GetColor(255, 113, 36));
	mpBackUI->GetText().SetFontId(FontId::Button);
	mpBackUI->GetText().SetText("Back!");
	mpBackUI->GetHoverScaler().SetActive(true);

	this->AddChild(mpBackUI);
	mpBackUI->SetParent(this);
	mpBackUI->SetEnabled(true);

	// -- オーバーレイ生成 -- //
	mpOverlay = GetUIManager()->CreateUI<UI>();
	mpOverlay->LoadUIGraph("Resource/UI/Overlay/Overlay_black.png");
	mpOverlay->SetLayer(LAYER_OVERLAY);
	mpOverlay->SetEnabled(true);
	this->AddChild(mpOverlay);
	mpOverlay->SetParent(this);

	// -- キーボード選択(FocusUI) -- //
	CreateFocusUI();

	// -- Playerのアドレス取得 -- //
	mpPlayer =
		Master::mpSceneManager
		->GetCurrentScene()
		->GetObjectManager()
		->FindObject<Player>();
}

void PlayerShapeUI::CreatePieceUI(int _column, int _row)
{
	auto pieceUI = GetUIManager()->CreateUI<PlayerPieceUI>();
	if (pieceUI == nullptr)
		return;

	pieceUI->LoadUIGraph("Resource/UI/ShapeSelect/Shape_cell.png");
	pieceUI->SetLayer(LAYER_PIECE);
	mPieceUI[_column][_row] = pieceUI;

	// -- 親子の設定 -- //
	// 子は常に有効にしておき、表示/非表示は親(this)の有効状態だけで切り替える
	this->AddChild(pieceUI);
	pieceUI->SetParent(this);
	pieceUI->SetEnabled(true);

	// -- 初期座標の設定 (中央マス(1,1)を原点とする) -- //
	auto& pieceTransform = pieceUI->GetTransform();
	const VECTOR size = pieceTransform.GetSize();
	pieceTransform.SetPosition(VGet(
		(_column - 1) * size.x,
		(_row - 1) * size.y,
		0.0f
	));
}

void PlayerShapeUI::CreateFocusUI()
{
	mpFocus = GetUIManager()->CreateUI<FocusUI>();
	mpFocus->SetLayer(LAYER_FOCUS);

	// 子にしておくと、変形UIが開いている間だけ操作を受け付ける
	this->AddChild(mpFocus);
	mpFocus->SetParent(this);

	// 3×3のマス → OK / Back の順に登録(上下左右の移動先は位置から自動で決まる)
	for (int row = 0; row < GameConfig::PLAYER_PIECE_SIZE; ++row)
	{
		for (int column = 0; column < GameConfig::PLAYER_PIECE_SIZE; ++column)
		{
			mpFocus->Add(mPieceUI[column][row]);
		}
	}
	mpFocus->Add(mpConfirmUI);
	mpFocus->Add(mpBackUI);

	// 開いたときは中央のマスから
	const int center = GameConfig::PLAYER_PIECE_SIZE / 2;
	mpFocus->SetDefaultFocus(mPieceUI[center][center]);
}

void PlayerShapeUI::Open()
{
	if (mpPlayer == nullptr)
		return;

	// 基盤ブロック・地形にめり込むマスを決めて Player を停止 → その状態をUIへ反映
	mpPlayer->BeginShapeEdit();
	SyncFromPlayer();
	RefreshPieceLocks();

	mpFocus->ResetFocus();
	SetEnabled(true);
}

void PlayerShapeUI::Close()
{
	if (mpPlayer != nullptr)
		mpPlayer->EndShapeEdit();

	SetEnabled(false);
}

void PlayerShapeUI::OnShapeInput()
{
	if (IsEnabled())
		Close();
	else
		Open();
}

void PlayerShapeUI::Update()
{
	// 子(各マス)がクリックを処理する前に、最新の選択状態でロックを更新しておく
	RefreshPieceLocks();

	UI::Update();

	if (mpConfirmUI->GetButton().IsClicked())
	{
		ApplyShapeFromUI();
		return;
	}

	// Back ボタンで変更せずに閉じる (変形キーでの開閉は GameScene 側)
	if (mpBackUI->GetButton().IsClicked())
	{
		Close();
		return;
	}
}

void PlayerShapeUI::Draw()
{
	UI::Draw();
}

bool PlayerShapeUI::IsPieceSelected(int _column, int _row) const
{
	return mPieceUI[_column][_row]->IsSelected();
}

void PlayerShapeUI::SyncFromPlayer()
{
	const PlayerShapeMask shape = mpPlayer->GetScreenShape();

	int baseColumn = -1;
	int baseRow = -1;
	mpPlayer->GetScreenBaseCell(baseColumn, baseRow);

	// 変形中はPlayerが止まっているので、めり込みマスは開いたときに1回反映すればよい
	const PlayerShapeMask terrainBlocked = mpPlayer->GetScreenTerrainBlockedMask();

	for (int row = 0; row < GameConfig::PLAYER_PIECE_SIZE; ++row)
	{
		for (int column = 0; column < GameConfig::PLAYER_PIECE_SIZE; ++column)
		{
			auto pieceUI = mPieceUI[column][row];
			pieceUI->SetSelected(shape[column][row]);
			pieceUI->SetBase(column == baseColumn && row == baseRow);
			pieceUI->SetTerrainBlocked(terrainBlocked[column][row]);
		}
	}
}

void PlayerShapeUI::RefreshPieceLocks()
{
	if (mpPlayer == nullptr || !mpPlayer->IsShapeEditing())
		return;

	// 「どのマスを切り替えてよいか」の判定は Player に任せる
	const PlayerShapeMask toggleable =
		mpPlayer->GetScreenToggleableMask(MakeShapeFromUI(*this));

	for (int row = 0; row < GameConfig::PLAYER_PIECE_SIZE; ++row)
	{
		for (int column = 0; column < GameConfig::PLAYER_PIECE_SIZE; ++column)
		{
			mPieceUI[column][row]->SetLocked(!toggleable[column][row]);
		}
	}
}

void PlayerShapeUI::ApplyShapeFromUI()
{
	if (mpPlayer == nullptr)
		return;

	// 選べないマスはロック済みなので、ここに来る形状は常に正しい。
	// 万一失敗した場合(判定のズレなど)は、何もせず開いたままにする。
	if (mpPlayer->TryChangeShapeFromScreen(MakeShapeFromUI(*this)) == ShapeChangeResult::Success)
		Close();
}
