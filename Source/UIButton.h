#pragma once

class UI;

/* memo :
* UIを「押せるボタン」として扱うModule。
* 押し方は2通り
*   1. マウス     … カーソルがUIの範囲内で Confirm(左クリック / Enter)
*   2. FocusUI   … FocusUIに選ばれている(フォーカス中)状態で FocusDecide(Space など)
* どちらで押されても IsClicked() が true になるので、呼び出し側は押し方を気にしなくてよい。
*/
class UIButton
{
public:
    UIButton() = default;
    ~UIButton() = default;

    void Initialize(UI* _pUI);

    // マウスカーソルがUIの範囲内にあるか
    bool IsMouseInside();

    // このフレームで決定入力が行われたか(マウス / FocusUI どちらでも)
    bool IsClicked();

    // 見た目上「選ばれている」か(UIHoverScalerが拡大に使う)
    //   FocusUIの管理下 … フォーカス中か
    //   それ以外         … マウスが乗っているか
    bool IsHighlighted();

    // -- FocusUIから設定される状態 -- //
    void SetFocused(bool _focused) { mbFocused = _focused; }
    bool IsFocused() const { return mbFocused; }
    void SetFocusManaged(bool _managed) { mbFocusManaged = _managed; }

    // FocusDecideで押されたときに決定SEを鳴らすか(シーン側で別のSEを鳴らす場合は false)
    void SetDecideSE(bool _play) { mbPlayDecideSE = _play; }

private:
    bool IsFocusDecided();  // FocusUI経由で押されたか

private:
    UI* mpUI = nullptr;

    bool mbFocused = false;         // FocusUIに選ばれているか
    bool mbFocusManaged = false;    // FocusUIの管理下にあるか
    bool mbPlayDecideSE = true;
};
