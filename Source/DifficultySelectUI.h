#pragma once
#include "UI.h"
#include "GameProgress.h"   // Difficulty

class FocusUI;

/* memo :
* StageSelectScene でステージを選んだ直後に出る「難易度選択」のポップアップ。
*   [Easy]    … 今まで通り(ミスしてもアイテムはそのまま)
*   [Normal]  … ミスするとアイテムの取得状態もリセット
*   [戻る]    … ステージ選択に戻る
*
* 使い方(ConfigUI / OperationGuideUI と同じ形)
*   開くとき     : SetEnabled(true) → OnOpen()
*   毎フレーム   : GetResult() を見て、Decided なら GetSelectedDifficulty() で結果を受け取る
*/
class DifficultySelectUI : public UI
{
public:
    enum class Result
    {
        None,       // まだ何も押されていない
        Decided,    // 難易度が決まった
        Back,       // 「戻る」が押された
    };

    void Init() override;
    void Update() override;

    // 開いたときに呼ぶ(前回の結果を消し、前回選んだ難易度にフォーカスを合わせる)
    void OnOpen();

    Result GetResult() const { return mResult; }
    Difficulty GetSelectedDifficulty() const { return mSelected; }

private:
    UI* CreateButton(const char* _label, const VECTOR& _position);
    UI* CreateTextUI(const std::string& _text, FontId _font, const VECTOR& _position);

private:
    UI* mpOverlay = nullptr;
    UI* mpTitle = nullptr;
    UI* mpEasyButton = nullptr;
    UI* mpNormalButton = nullptr;
    UI* mpBackButton = nullptr;
    FocusUI* mpFocus = nullptr;

    Result mResult = Result::None;
    Difficulty mSelected = Difficulty::Easy;
};
