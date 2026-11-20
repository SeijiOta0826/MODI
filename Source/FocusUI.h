#pragma once
#include "UI.h"

#include <vector>

/* memo :
* キーボード(テンキー / 矢印キー / Space)やパッドで、UIを選んで決定できるようにするUI。
*
* 使い方:
*   FocusUI* focus = GetUIManager()->CreateUI<FocusUI>();
*   focus->Add(mpStartButton);   // 選べるUIを登録(登録順 = 0, 1, 2...)
*   focus->Add(mpQuitButton);
*   focus->ResetFocus();         // 既定のUI(最初に登録したUI)を選ぶ
*
*   あとは今までどおり mpStartButton->GetButton().IsClicked() で判定するだけ。
*   Space で押されたときも IsClicked() が true になる。
*
* 仕組み:
*   ・方向キーを押すと、その方向にある「一番近いUI」へフォーカスを移す(画面上の位置で自動判定)
*     → 並びを手で指定しなくても、格子状でもバラバラな配置でも動く
*   ・マウスを動かしてUIに乗せると、フォーカスもそこへ移る(マウス操作と見た目を揃える)
*   ・フォーカスの見た目
*       UIHoverScalerが有効なUI … 少し拡大(マウスホバーと同じ演出)
*       それ以外のUI            … 点滅する枠を描画
*   ・FocusUI自身もUIなので、親子関係を設定すれば「親が表示中のときだけ」操作を受け付ける
*/
class FocusUI : public UI
{
public:
	FocusUI() = default;
	~FocusUI() = default;

	void Init() override;
	void Update() override;
	void Draw() override;

	// -- 登録 -- //
	void Add(UI* _item);
	void SetDefaultFocus(UI* _item);	// ResetFocus()で選ぶUI(未設定なら最初に登録したUI)

	// -- フォーカス操作 -- //
	void ResetFocus();					// 既定のUIを選び直す(メニューを開いたときに呼ぶ)
	void SetFocus(UI* _item);
	UI* GetFocused() const;

private:
	void UpdateKeyInput();
	void UpdateMouseFollow();

	int  FindIndex(UI* _item) const;
	int  FindNext(float _dirX, float _dirY) const;	// 指定方向で一番近いUIを探す(なければ -1)
	void ApplyFocus(int _index);

	void DrawFocusFrame(UI* _item);
	void SyncMousePosition();

private:
	std::vector<UI*> mItems;

	int mnFocusIndex = -1;		// 今選んでいるUI(-1 = なし)
	int mnDefaultIndex = 0;

	int mnLastMouseX = -1;		// マウスが「動いたか」を調べるための前回位置
	int mnLastMouseY = -1;
};
