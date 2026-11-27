#pragma once

#include "Scene.h"
#include "UIHoverScaler.h"
#include "KitchenToolTable.h"

class UI;
class FocusUI;

class TitleScene : public Scene
{
public:
	TitleScene() = default;
	~TitleScene() = default;

	void Initialize() override;
	void Update(float _deltaTime) override;
	void Draw() override;
	void Finalize() override;

private:
	void CreateBackground();	// まな板・本・豆腐・ロゴ・「豆腐レシピ本」
	void CreateKitchenTools();	// 動かせる調理器具
	void CreateButtons();		// Start / Quit

	bool IsMouseOnButton();		// カーソルがボタンの上にあるか(器具より優先するため)

private:
	bool mbStartSelected = false;
	bool mbPreviousMouseLeft = false;

private:
	UI* mpBackGround = nullptr;	// 背景画像(まな板)
	UI* mpRecipeBook = nullptr;	// 本の表紙
	UI* mpTofu = nullptr;		// 表紙の豆腐
	UI* mpLogo = nullptr;		// ロゴ画像(MOD!)
	UI* mpRecipeText = nullptr;	// 「豆腐レシピ本」の文字
	UI* mpStartButton = nullptr;
	UI* mpQuitButton = nullptr;
	FocusUI* mpFocus = nullptr;	// キーボードでのボタン選択

	KitchenToolTable mKitchenTools;	// ドラッグ・衝突できる調理器具
};
