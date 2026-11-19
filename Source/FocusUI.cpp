#include "FocusUI.h"

#include "InputManager.h"

#include <DxLib.h>
#include <cfloat>	// FLT_MAX
#include <cmath>	// std::fabs, std::sin

namespace
{
	constexpr int   FOCUS_DEFAULT_LAYER = 10;			// 既定のレイヤー(選べるUIより上に枠を描く)
	constexpr float FOCUS_SIDE_WEIGHT = 2.0f;			// 「横ずれ」をどれだけ嫌うか(大きいほど真っすぐ先を優先)

	constexpr int   FOCUS_FRAME_THICKNESS = 4;		// 枠の太さ(px)
	constexpr int   FOCUS_FRAME_MARGIN = 4;			// UIの外側にどれだけ広げて描くか(px)
	constexpr int   FOCUS_FRAME_ALPHA_MIN = 120;		// 点滅の明るさ(最小 / 最大)
	constexpr int   FOCUS_FRAME_ALPHA_MAX = 255;
	constexpr float FOCUS_FRAME_BLINK_SPEED = 0.006f;	// 点滅の速さ(ミリ秒あたりの角度)
}

void FocusUI::Init()
{
	SetLayer(FOCUS_DEFAULT_LAYER);
	SetEnabled(true);
}

// ------------------------------------------------------------
// 登録
// ------------------------------------------------------------
void FocusUI::Add(UI* _item)
{
	if (_item == nullptr)
		return;

	_item->GetButton().SetFocusManaged(true);	// 見た目の強調をFocusUIに任せる
	mItems.push_back(_item);
}

void FocusUI::SetDefaultFocus(UI* _item)
{
	const int index = FindIndex(_item);
	if (index != -1)
		mnDefaultIndex = index;
}

// ------------------------------------------------------------
// フォーカス操作
// ------------------------------------------------------------
void FocusUI::ResetFocus()
{
	if (mItems.empty())
		return;

	ApplyFocus(mnDefaultIndex);

	// 開いた瞬間にマウス位置へ吸い寄せられないよう、今の位置を「動いていない」扱いにする
	SyncMousePosition();
}

void FocusUI::SetFocus(UI* _item)
{
	const int index = FindIndex(_item);
	if (index != -1)
		ApplyFocus(index);
}

UI* FocusUI::GetFocused() const
{
	if (mnFocusIndex < 0 || mnFocusIndex >= static_cast<int>(mItems.size()))
		return nullptr;

	return mItems[mnFocusIndex];
}

// ------------------------------------------------------------
// 更新
// ------------------------------------------------------------
void FocusUI::Update()
{
	if (mItems.empty())
		return;

	// 一度もフォーカスしていなければ既定のUIを選ぶ
	if (mnFocusIndex == -1)
		ResetFocus();

	UpdateMouseFollow();
	UpdateKeyInput();
}

void FocusUI::UpdateKeyInput()
{
	const auto& input = InputManager::GetInstance();

	float dirX = 0.0f;
	float dirY = 0.0f;

	if (input.GetButtonDown(Button::FocusUp))			dirY = -1.0f;
	else if (input.GetButtonDown(Button::FocusDown))	dirY = 1.0f;
	else if (input.GetButtonDown(Button::FocusLeft))	dirX = -1.0f;
	else if (input.GetButtonDown(Button::FocusRight))	dirX = 1.0f;
	else
		return;

	const int next = FindNext(dirX, dirY);
	if (next != -1)
		ApplyFocus(next);
}

void FocusUI::UpdateMouseFollow()
{
	int mouseX = 0;
	int mouseY = 0;
	GetMousePoint(&mouseX, &mouseY);

	// マウスが動いていないなら、キー操作で選んだフォーカスを優先する
	if (mouseX == mnLastMouseX && mouseY == mnLastMouseY)
		return;

	mnLastMouseX = mouseX;
	mnLastMouseY = mouseY;

	for (int i = 0; i < static_cast<int>(mItems.size()); ++i)
	{
		if (mItems[i]->GetButton().IsMouseInside())
		{
			ApplyFocus(i);
			return;
		}
	}
}

// ------------------------------------------------------------
// 探索
// ------------------------------------------------------------
int FocusUI::FindIndex(UI* _item) const
{
	for (int i = 0; i < static_cast<int>(mItems.size()); ++i)
	{
		if (mItems[i] == _item)
			return i;
	}
	return -1;
}

int FocusUI::FindNext(float _dirX, float _dirY) const
{
	UI* current = GetFocused();
	if (current == nullptr)
		return -1;

	const VECTOR from = current->GetTransform().GetCenterPosition();

	// 1周目 : 押した方向の「±45度の扇形」の中だけで探す(斜め奥のUIに飛ばないように)
	// 2周目 : 扇形の中に何もなければ、押した方向の半分の画面全体で探す
	for (int pass = 0; pass < 2; ++pass)
	{
		int   bestIndex = -1;
		float bestScore = FLT_MAX;

		for (int i = 0; i < static_cast<int>(mItems.size()); ++i)
		{
			UI* item = mItems[i];
			if (item == current || !item->IsEnabled())
				continue;

			const VECTOR to = item->GetTransform().GetCenterPosition();
			const float dx = to.x - from.x;
			const float dy = to.y - from.y;

			// forward : 押した方向へどれだけ進んでいるか(内積)
			// side    : 押した方向からどれだけ横にずれているか(外積の大きさ)
			const float forward = dx * _dirX + dy * _dirY;
			const float side = std::fabs(dx * _dirY - dy * _dirX);

			if (forward <= 0.0f)
				continue;	// 逆方向・真横は対象外

			if (pass == 0 && side > forward)
				continue;	// 扇形の外

			const float score = forward + side * FOCUS_SIDE_WEIGHT;
			if (score < bestScore)
			{
				bestScore = score;
				bestIndex = i;
			}
		}

		if (bestIndex != -1)
			return bestIndex;
	}

	return -1;
}

void FocusUI::ApplyFocus(int _index)
{
	if (_index < 0 || _index >= static_cast<int>(mItems.size()))
		return;

	if (UI* prev = GetFocused())
		prev->GetButton().SetFocused(false);

	mnFocusIndex = _index;
	mItems[mnFocusIndex]->GetButton().SetFocused(true);
}

void FocusUI::SyncMousePosition()
{
	GetMousePoint(&mnLastMouseX, &mnLastMouseY);
}

// ------------------------------------------------------------
// 描画
// ------------------------------------------------------------
void FocusUI::Draw()
{
	UI* focused = GetFocused();
	if (focused == nullptr || !focused->IsEnabled())
		return;

	// 拡大演出があるUIはそれで十分なので枠は描かない
	if (focused->GetHoverScaler().IsActive())
		return;

	DrawFocusFrame(focused);
}

void FocusUI::DrawFocusFrame(UI* _item)
{
	auto& transform = _item->GetTransform();
	const VECTOR center = transform.GetCenterPosition();
	const VECTOR size = transform.GetSize();
	const VECTOR scale = transform.GetScale();

	const int halfW = static_cast<int>(size.x * scale.x * 0.5f) + FOCUS_FRAME_MARGIN;
	const int halfH = static_cast<int>(size.y * scale.y * 0.5f) + FOCUS_FRAME_MARGIN;
	const int cx = static_cast<int>(center.x);
	const int cy = static_cast<int>(center.y);

	// sin波で明るさを上下させて点滅させる(0.0～1.0 に変換してから最小～最大へ)
	const float wave = (std::sin(GetNowCount() * FOCUS_FRAME_BLINK_SPEED) + 1.0f) * 0.5f;
	const int alpha = FOCUS_FRAME_ALPHA_MIN + static_cast<int>((FOCUS_FRAME_ALPHA_MAX - FOCUS_FRAME_ALPHA_MIN) * wave);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

	const unsigned int color = GetColor(255, 200, 0);
	for (int i = 0; i < FOCUS_FRAME_THICKNESS; ++i)
	{
		DrawBox(cx - halfW - i, cy - halfH - i,
			cx + halfW + i, cy + halfH + i,
			color, FALSE);
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
