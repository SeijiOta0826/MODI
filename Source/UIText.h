#pragma once

#include <string>
#include <DxLib.h>

#include "FontManager.h" // FontId

class UI;

enum class UITextAlign
{
	LEFT,
	CENTER,
	RIGHT
};

enum class UITextVerticalAlign
{
	TOP,
	CENTER,
	BOTTOM
};

/* memo :
* 各UIにコンポジットする「テキスト描画」の要素。
* フォントの見た目(名前・サイズ・太さ)はFontIdで指定し、FontManagerから取得する。
* 色・縁の色・配置はこのクラスで管理する。
* '\n' で改行でき、行ごとに配置(Align)を適用する。
* 付与先UIのUITypewriterTextが有効なら、全文ではなく「文字送りで出ている分」だけを描く。
*/
class UIText
{
public:
	UIText() = default;
	~UIText() = default;

	void Initialize(UI* _pUI);
	void Draw();

	// -- テキスト -- //
	void SetText(const std::string& _text) { mText = _text; }
	const std::string& GetText() const { return mText; }

	// -- フォント -- //
	void SetFontId(FontId _fontId) { mFontId = _fontId; }
	FontId GetFontId() const { return mFontId; }

	// -- 色 -- //
	void SetColor(unsigned int _color) { mColor = _color; }
	void SetEdgeColor(unsigned int _edgeColor) { mEdgeColor = _edgeColor; }	// 縁付きフォントのときのみ有効

	// -- 配置 -- //
	void SetAlign(UITextAlign _align) { mAlign = _align; }
	UITextAlign GetAlign() const { return mAlign; }

	// -- 下線 -- //
	void SetUnderline(bool _enabled) { mbUnderline = _enabled; }

	void SetVerticalAlign(UITextVerticalAlign _align) { mVerticalAlign = _align; }
	UITextVerticalAlign GetVerticalAlign() const { return mVerticalAlign; }

	void SetOffset(const VECTOR& _offset) { mvOffset = _offset; }

	// -- 行間 -- //
	void SetLineSpacing(int _lineSpacing) { mnLineSpacing = _lineSpacing; }	// 行の上端から次の行の上端まで(px)。0以下ならフォントサイズ

private:
	int GetLineHeight(int _fontHandle) const;	// 1行分の送り幅を返す
	VECTOR CalculateLineDrawPosition(const std::string& _line, size_t _lineIndex, size_t _lineCount, int _fontHandle) const;	// Alignを考慮した各行の描画座標を返す
	void DrawUnderline(const std::string& _line, const VECTOR& _textPosition, int _fontHandle) const;

private:
	std::string mText;	// 描画するテキストの全文

	FontId mFontId = FontId::Default;	// 使用するフォント

	unsigned int mColor = GetColor(255, 255, 255);	// 文字色
	unsigned int mEdgeColor = GetColor(0, 0, 0);	// 縁の色

	UITextAlign mAlign = UITextAlign::CENTER;					// 横の配置(デフォルトは中央揃え)
	UITextVerticalAlign mVerticalAlign = UITextVerticalAlign::CENTER;	// 縦の配置(デフォルトは中央揃え)

	VECTOR mvOffset = VGet(0.0f, 0.0f, 0.0f);	// 配置後の追加オフセット

	int mnLineSpacing = 0;	// 行間(0以下ならフォントサイズ)

	bool mbUnderline = false;	// 下線を引くか
	static constexpr int UNDERLINE_GAP = 4;			// 文字と下線の隙間
	static constexpr int UNDERLINE_THICKNESS = 3;	// 下線の太さ
	static constexpr int UNDERLINE_PADDING = 6;		// 下線を文字幅より左右に伸ばす量

	UI* mpUI = nullptr;	// 付与先のUI
};
