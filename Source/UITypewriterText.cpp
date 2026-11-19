#include "UITypewriterText.h"

#include "UI.h"

#include <mbstring.h>	// _ismbblead

void UITypewriterText::Initialize(UI* _pUI)
{
	mpUI = _pUI;
	mbActive = true;
	Start();
}

void UITypewriterText::Start()
{
	mnVisibleByteCount = 0;
	mnFrameTimer = 0;
	mbIsFinished = false;
}

void UITypewriterText::Update()
{
	if (!IsActive() || mbIsFinished)
		return;

	// -- フレームカウントによるウェイト処理 -- //
	if (mnFrameTimer > 0)
	{
		mnFrameTimer--;
		return;
	}
	mnFrameTimer = mnFrameInterval;

	const std::string& text = mpUI->GetText().GetText();	// コピーせず参照で受け取る

	// -- 文字送りの処理 -- //
	if (mnVisibleByteCount < text.length())
	{
		const unsigned char c = static_cast<unsigned char>(text[mnVisibleByteCount]);

		// 全角文字（先行バイト）なら2バイト、半角なら1バイト進める
		mnVisibleByteCount += _ismbblead(c) ? MULTI_BYTE_SIZE : 1;
	}

	// -- 最後まで表示し終えたか判定(空文字のときもここで終了扱いになる) -- //
	if (mnVisibleByteCount >= text.length())
	{
		mnVisibleByteCount = text.length();
		mbIsFinished = true;
	}
}

std::string UITypewriterText::GetVisibleText() const
{
	if (mpUI == nullptr)
		return std::string();

	// substrは長さが文字列を超えても末尾までで止まるので安全
	return mpUI->GetText().GetText().substr(0, mnVisibleByteCount);
}

void UITypewriterText::Skip()
{
	if (mpUI == nullptr)
		return;

	mnVisibleByteCount = mpUI->GetText().GetText().length();
	mbIsFinished = true;
}
