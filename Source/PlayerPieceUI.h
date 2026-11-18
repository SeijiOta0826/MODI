#pragma once
#include "UI.h"

/* memo :
* 変形UIの「内側」1マス分のUI。
* クリックで選択(=ブロックを置く)/非選択を切り替える。
* 基盤ブロックのマスと、ロック中のマスはクリックしても切り替わらない。
*   ロックの判定は Player 側 (PlayerShapeUI 経由で毎フレーム設定される)
*   濃い影は「ステージにめり込むため置けない」マスだけに付ける
*/
class PlayerPieceUI : public UI
{
public:
	PlayerPieceUI() = default;
	~PlayerPieceUI() = default;

	void Init() override;

	void Update() override;
	void Draw() override;

	// -- 選択状態のアクセサ -- //
	bool IsSelected() const { return mbSelected; }
	void SetSelected(bool _selected) { mbSelected = _selected; }
	void ToggleSelected() { mbSelected = !mbSelected; }

	// -- 基盤ブロック状態のアクセサ -- //
	bool IsBase() const { return mbIsBase; }
	void SetBase(bool _isBase) { mbIsBase = _isBase; }

	// -- ロック状態のアクセサ (true = クリックしても切り替えられない) -- //
	bool IsLocked() const { return mbLocked; }
	void SetLocked(bool _locked) { mbLocked = _locked; }

	// -- ステージにめり込む状態のアクセサ (true = 濃い影で表示) -- //
	bool IsTerrainBlocked() const { return mbTerrainBlocked; }
	void SetTerrainBlocked(bool _blocked) { mbTerrainBlocked = _blocked; }

private:
	int GetShadeAmount() const;	// 減算ブレンドで暗くする量(0 = そのまま)
	void DrawBaseFrame();		// 基盤マスの赤枠描画

private:
	bool mbSelected = false;
	bool mbIsBase = false;
	bool mbLocked = false;
	bool mbTerrainBlocked = false;
};
