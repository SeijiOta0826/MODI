#include "UIText.h"

#include "UI.h"

#include "Master.h"
#include "FontManager.h"

#include <vector>

namespace
{
	// 文字列を'\n'で行ごとに分割する
	// (Shift-JISの2バイト目に'\n'(0x0A)は来ないので、1バイトずつ見ても安全)
	std::vector<std::string> SplitLines(const std::string& _text)
	{
		std::vector<std::string> lines;
		size_t start = 0;
		while (true)
		{
			const size_t pos = _text.find('\n', start);
			if (pos == std::string::npos)
			{
				lines.push_back(_text.substr(start));
				break;
			}
			lines.push_back(_text.substr(start, pos - start));
			start = pos + 1;
		}
		return lines;
	}
}

void UIText::Initialize(UI* _pUI)
{
	mpUI = _pUI;
}

void UIText::Draw()
{
	if (mpUI == nullptr || mText.empty())
		return;

	// -- 描画する文字列を決める(文字送りが有効なら、出ている分だけ) -- //
	const UITypewriterText& typewriter = mpUI->GetTypewriter();
	const std::string visibleText = typewriter.IsActive() ? typewriter.GetVisibleText() : mText;
	if (visibleText.empty())
		return;

	// -- フォントハンドルを取得(未登録でもDefaultが返るので-1チェック不要) -- //
	const int fontHandle = Master::mpFontManager->GetHandle(mFontId);

	// -- 行に分割 -- //
	// 配置(中央揃えなど)は「全文」の行で計算し、描くのは「見えている分」の行。
	// こうすると文字送り中に中央揃えの文字が左右にずれていかない
	const std::vector<std::string> fullLines = SplitLines(mText);
	const std::vector<std::string> visibleLines = SplitLines(visibleText);

	// -- floatingを適用(有効でない場合はデフォルト) -- //
	const VECTOR floating = mpUI->GetFloatingMotion().GetOffset();

	for (size_t i = 0; i < visibleLines.size() && i < fullLines.size(); ++i)
	{
		VECTOR position = CalculateLineDrawPosition(fullLines[i], i, fullLines.size(), fontHandle);
		position = VAdd(position, floating);

		DrawStringToHandle(
			static_cast<int>(position.x),
			static_cast<int>(position.y),
			visibleLines[i].c_str(),
			mColor,
			fontHandle,
			mEdgeColor
		);

		if (mbUnderline)
			DrawUnderline(visibleLines[i], position, fontHandle);
	}
}

int UIText::GetLineHeight(int _fontHandle) const
{
	// 行間が未指定(0以下)ならフォントサイズをそのまま使う
	return (mnLineSpacing > 0) ? mnLineSpacing : GetFontSizeToHandle(_fontHandle);
}

VECTOR UIText::CalculateLineDrawPosition(const std::string& _line, size_t _lineIndex, size_t _lineCount, int _fontHandle) const
{
	auto& transform = mpUI->GetTransform();
	VECTOR basePosition = transform.GetCenterPosition();

	// -- 描画サイズを「実際に使うフォント」から取得 -- //
	const int lineWidth = GetDrawStringWidthToHandle(
		_line.c_str(),
		static_cast<int>(_line.size()),
		_fontHandle
	);
	const int fontSize = GetFontSizeToHandle(_fontHandle);
	const int lineHeight = GetLineHeight(_fontHandle);

	// テキスト全体(全行)の高さ。1行のときは fontSize になり、従来の計算と同じ
	const float blockHeight = static_cast<float>(lineHeight * (static_cast<int>(_lineCount) - 1) + fontSize);

	// -- 横の配置(行ごと) -- //
	float drawX = basePosition.x;
	switch (mAlign)
	{
	case UITextAlign::LEFT:
		break;

	case UITextAlign::CENTER:
		drawX -= lineWidth / 2.0f;
		break;

	case UITextAlign::RIGHT:
		drawX -= lineWidth;
		break;
	}

	// -- 縦の配置(全行のかたまりで揃える) -- //
	float drawY = basePosition.y;
	switch (mVerticalAlign)
	{
	case UITextVerticalAlign::TOP:
		break;

	case UITextVerticalAlign::CENTER:
		drawY -= blockHeight / 2.0f;
		break;

	case UITextVerticalAlign::BOTTOM:
		drawY -= blockHeight;
		break;
	}

	// -- 何行目か分だけ下へ -- //
	drawY += static_cast<float>(lineHeight * static_cast<int>(_lineIndex));

	return VAdd(VGet(drawX, drawY, 0.0f), mvOffset);
}

void UIText::DrawUnderline(const std::string& _line, const VECTOR& _textPosition, int _fontHandle) const
{
	const int width = GetDrawStringWidthToHandle(
		_line.c_str(),
		static_cast<int>(_line.size()),
		_fontHandle
	);
	const int height = GetFontSizeToHandle(_fontHandle);

	const int left = static_cast<int>(_textPosition.x) - UNDERLINE_PADDING;
	const int right = static_cast<int>(_textPosition.x) + width + UNDERLINE_PADDING;
	const int top = static_cast<int>(_textPosition.y) + height + UNDERLINE_GAP;

	DrawBox(left, top, right, top + UNDERLINE_THICKNESS, mColor, TRUE);
}
