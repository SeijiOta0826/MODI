#pragma once
#include "UI.h"

/* memo :
* GameSceneの画面外周に描くフレーム(枠)のUI。
* 画像(Resource/UI/Game/GameFrame.png)は中央が透明な1920x1080の1枚絵なので、
* 画面中央に置いて画面サイズに合わせるだけでよい。
*
* 描画順 :
*   UIManagerはレイヤーの小さい順に描くので、LAYER = -1 にして「UIの中で一番奥」に置く。
*   → ステージやPlayer(ObjectManager)より手前、HUD・ポーズ画面・クリア画面より奥になる。
*/
class GameFrameUI : public UI
{
public:
	GameFrameUI() = default;
	~GameFrameUI() = default;

	void Init() override;

private:
	static constexpr int LAYER = -1;
};
