#pragma once
#include "UI.h"

#include <string>

/* memo :
* 「背景パネル(UIGraphic) + 文字送りテキスト(UIText + UITypewriterText)」をまとめたメッセージウィンドウUI。
* チュートリアルの案内文など、画面にセリフ・説明文を出したい場面で使う。
*
* // -- 使い方 -- //
*	auto* message = GetUIManager()->CreateUI<MessageUI>();
*	message->Show("1行目\n2行目");				// 表示 + 文字送り開始
*	if (message->IsFinished()) { ... }			// 全文出し終わったか
*	message->SkipTypewriter();					// 全文を一気に表示
*	message->Hide();							// 非表示
*
*	// 位置・大きさを変えたい場合(画面座標の矩形で指定)
*	message->SetWindowRect(30, 25, 920, 165);
*/
class MessageUI : public UI
{
public:
	void Init() override;

	// -- 表示制御 -- //
	void Show(const std::string& _text);	// テキストを差し替えて表示し、先頭から文字送りする
	void Hide() { SetEnabled(false); }

	// -- 文字送り -- //
	void SkipTypewriter() { GetTypewriter().Skip(); }
	bool IsFinished() { return GetTypewriter().IsFinished(); }
	void SetFrameInterval(int _interval) { GetTypewriter().SetFrameInterval(_interval); }

	// -- 配置 -- //
	void SetWindowRect(int _left, int _top, int _right, int _bottom);	// 画面座標(px)でウィンドウの矩形を指定
	void SetTextPadding(float _x, float _y);							// ウィンドウ左上から文字までの余白(px)

private:
	void ApplyLayout();	// 矩形・余白の設定をTransform / Textへ反映する

private:
	static constexpr const char* PANEL_FILE_PATH = "Resource/UI/Common/MessageWindow.png";

	static constexpr int LAYER = 5;					// ポーズ画面(〜3)などより手前に出す
	static constexpr int PANEL_ALPHA = 190;			// パネルの不透明度(0〜255)
	static constexpr int FRAME_INTERVAL = 2;		// 文字送りの速さ(何フレームごとに1文字か)
	static constexpr int LINE_SPACING = 48;			// 行の間隔(px)

	// -- ウィンドウの矩形(画面座標) -- //
	int mnLeft = 30;
	int mnTop = 25;
	int mnRight = 920;
	int mnBottom = 165;

	// -- ウィンドウ左上から文字までの余白 -- //
	float mfPaddingX = 30.0f;
	float mfPaddingY = 25.0f;

	VECTOR mvPanelSize = VGet(0.0f, 0.0f, 0.0f);	// パネル画像の元のサイズ(拡縮率の計算用)
};
