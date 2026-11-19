#include "PlayerPieceUI.h"

#include <DxLib.h>	// 減算ブレンド・枠描画用

namespace
{
	constexpr int UNSELECTED_SUB_BLEND = 100;	// 非選択マスを暗くする減算量
	constexpr int TERRAIN_BLOCKED_SUB_BLEND = 200;	// ステージにめり込むマスを暗くする減算量(非選択より濃い影)
	constexpr int BASE_FRAME_THICKNESS = 4;		// 基盤マスの赤枠の太さ(px)
}

void PlayerPieceUI::Init()
{
	// 画像は生成側(PlayerShapeUI)で「内側.png」をロードする
	GetTransform().SetAnchor(VGet(0.5f, 0.5f, 0.0f));
}

void PlayerPieceUI::Update()
{
	if (!GetButton().IsClicked())
		return;

	// 基盤ブロック・ロック中のマスは切り替えない
	if (IsBase() || IsLocked())
		return;

	ToggleSelected();
}

void PlayerPieceUI::Draw()
{
	const int shade = GetShadeAmount();

	if (shade > 0)
	{
		SetDrawBlendMode(DX_BLENDMODE_SUB, shade);
		UI::Draw();
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	else
	{
		UI::Draw();
	}

	if (IsBase())
		DrawBaseFrame();
}

int PlayerPieceUI::GetShadeAmount() const
{
	// 選択中のマスは元の明るさのまま
	// (外せない選択マスまで暗くすると「非選択」に見えてしまうため)
	if (IsSelected())
		return 0;

	return IsTerrainBlocked() ? TERRAIN_BLOCKED_SUB_BLEND : UNSELECTED_SUB_BLEND;
}

void PlayerPieceUI::DrawBaseFrame()
{
	auto& transform = GetTransform();
	const VECTOR center = transform.GetCenterPosition();
	const VECTOR size = transform.GetSize();
	const VECTOR scale = transform.GetScale();

	const int halfW = static_cast<int>(size.x * scale.x * 0.5f);
	const int halfH = static_cast<int>(size.y * scale.y * 0.5f);
	const int cx = static_cast<int>(center.x);
	const int cy = static_cast<int>(center.y);
	const unsigned int red = GetColor(255, 0, 0);

	// 内側へ1pxずつ縮めながら重ね描きして太線にする
	for (int i = 0; i < BASE_FRAME_THICKNESS; ++i)
	{
		DrawBox(cx - halfW + i, cy - halfH + i,
			cx + halfW - i, cy + halfH - i,
			red, FALSE);
	}
}
