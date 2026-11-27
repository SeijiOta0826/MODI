#include "GameFrameUI.h"

#include "ScreenConfig.h"

void GameFrameUI::Init()
{
	LoadUIGraph("Resource/UI/Game/GameFrame.png");
	SetEnabled(true);
	SetLayer(LAYER);

	// -- 画面サイズに合わせる(画像が1920x1080なら等倍のまま) -- //
	const VECTOR size = GetTransform().GetSize();
	if (size.x > 0.0f && size.y > 0.0f)
	{
		GetTransform().SetScale(VGet(
			ScreenConfig::SCREEN_WIDTH / size.x,
			ScreenConfig::SCREEN_HEIGHT / size.y,
			0.0f
		));
	}
}
