#pragma once
#include "UI.h"

#include "GameConfig.h"

class PlayerPieceUI;
class FocusUI;
class Player;

/* memo :
* プレイヤーの形状変更UI(外枠 + 3×3の内側マス + OK/Backボタン)。
* 役割は「表示と入力」だけ。形状が正しいかの判定・適用は Player 側が行う。
*
* 使い方:
*   Open()  … 現在のPlayerの形をUIに反映して表示(Playerは編集中で停止)
*   Close() … 非表示にしてPlayerの停止を解除
*
* 選べないマス(地形にめり込む / つながりが切れる / 基盤)は
* Player に問い合わせて毎フレームロックし、濃い影で表示する。
* そのため OK を押した時点で、形状は常に正しい状態になっている。
*/
class PlayerShapeUI : public UI
{
public:
	PlayerShapeUI() = default;
	~PlayerShapeUI() = default;

	void Init() override;
	void Update() override;
	void Draw() override;

	void Open();
	void Close();
	void OnShapeInput();

	// _column : 横(x) / _row : 縦(y)。どちらも画面の見た目どおりの向き
	bool IsPieceSelected(int _column, int _row) const;

private:
	void CreatePieceUI(int _column, int _row);
	void CreateFocusUI();

	void SyncFromPlayer();		// Playerの現在形状 → UI
	void RefreshPieceLocks();	// 今の選択状態から、各マスのロック(選べるか)を更新
	void ApplyShapeFromUI();	// UI → Player(判定はPlayer側)

private:
	Player* mpPlayer = nullptr;

	// [column][row] の順で保持 (Player::mPieces と同じ並び)
	PlayerPieceUI* mPieceUI
		[GameConfig::PLAYER_PIECE_SIZE]
		[GameConfig::PLAYER_PIECE_SIZE]{};

	UI* mpConfirmUI = nullptr;
	UI* mpBackUI = nullptr;
	UI* mpOverlay = nullptr;
	UI* mpBackGround = nullptr;
	FocusUI* mpFocus = nullptr;	// キーボードでのマス / ボタン選択
};
