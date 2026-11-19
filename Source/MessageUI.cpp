#include "MessageUI.h"

#include <DxLib.h>

void MessageUI::Init()
{
	SetLayer(LAYER);
	SetEnabled(false);	// Show()が呼ばれるまでは出さない

	// -- 背景パネル -- //
	LoadUIGraph(PANEL_FILE_PATH);
	mvPanelSize = GetTransform().GetSize();	// LoadUIGraphで画像サイズが入る
	GetGraphic().SetAlpha(255);

	// -- テキスト(ウィンドウ左上から書き始める) -- //
	GetText().SetFontId(FontId::Message);
	GetText().SetColor(GetColor(255, 255, 255));
	GetText().SetAlign(UITextAlign::LEFT);
	GetText().SetVerticalAlign(UITextVerticalAlign::TOP);
	GetText().SetLineSpacing(LINE_SPACING);

	// -- 文字送り(ここでInitializeすると、UITextの描画が文字送りに切り替わる) -- //
	GetTypewriter().Initialize(this);
	GetTypewriter().SetFrameInterval(FRAME_INTERVAL);

	ApplyLayout();
}

void MessageUI::Show(const std::string& _text)
{
	GetText().SetText(_text);
	GetTypewriter().Start();
	SetEnabled(true);
}

void MessageUI::SetWindowRect(int _left, int _top, int _right, int _bottom)
{
	mnLeft = _left;
	mnTop = _top;
	mnRight = _right;
	mnBottom = _bottom;
	ApplyLayout();
}

void MessageUI::SetTextPadding(float _x, float _y)
{
	mfPaddingX = _x;
	mfPaddingY = _y;
	ApplyLayout();
}

void MessageUI::ApplyLayout()
{
	const float width = static_cast<float>(mnRight - mnLeft);
	const float height = static_cast<float>(mnBottom - mnTop);

	// -- 位置：アンカーを画面左上にして、ウィンドウの中心を置く -- //
	auto& transform = GetTransform();
	transform.SetAnchor(VGet(0.0f, 0.0f, 0.0f));
	transform.SetPosition(VGet(mnLeft + width / 2.0f, mnTop + height / 2.0f, 0.0f));

	// -- 大きさ：画像の元サイズに対する拡縮率で合わせる -- //
	if (mvPanelSize.x > 0.0f && mvPanelSize.y > 0.0f)
	{
		transform.SetScale(VGet(width / mvPanelSize.x, height / mvPanelSize.y, 0.0f));
	}

	// -- テキスト：中心からウィンドウ左上 + 余白 の位置へずらす -- //
	GetText().SetOffset(VGet(
		-width / 2.0f + mfPaddingX,
		-height / 2.0f + mfPaddingY,
		0.0f
	));
}
