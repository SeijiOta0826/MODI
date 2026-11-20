#pragma once
#include "UI.h"
#include "SoundManager.h"   // VOLUME_LEVEL_MAX

#include <array>

class FocusUI;

/* memo :
* ポーズ画面から開く「設定(Config)」画面。
* 今は BGM / SE の音量を別々に調節できる。
* 使い方は OperationGuideUI と同じで、PauseUI が OnOpen() / IsBackRequested() を使う。
*
* 1行の並び(音量1種類分)
*   [ラベル]   [－]  (豆腐 × 10)  [＋]   [60%]
*   豆腐は Tofu.png で、音量の段階までは明るく、それより上は暗く(Bright)描く。
*/
class ConfigUI : public UI
{
public:
    void Init() override;
    void Update() override;

    // 戻るボタンが押されたか(PauseUIが確認する)
    bool IsBackRequested() const { return mbBackRequested; }

    // 開いたときに呼ぶ(前回の「戻る」要求を消し、フォーカスを先頭に合わせる)
    void OnOpen();

private:
    // 音量調節1行分のUIをまとめた構造体
    struct VolumeRow
    {
        UI* pLabel = nullptr;       // 「BGM」「SE」の文字
        UI* pDownButton = nullptr;  // 「－」ボタン
        UI* pUpButton = nullptr;    // 「＋」ボタン
        UI* pValue = nullptr;       // 「60%」の文字
        std::array<UI*, SoundManager::VOLUME_LEVEL_MAX> pGauge{};  // 段階を表す豆腐(左から1段階目, 2段階目...)
    };

    VolumeRow CreateVolumeRow(const char* _label, float _posY);
    UI* CreateTextUI(const std::string& _text, FontId _font, const VECTOR& _position);
    UI* CreateStepButton(const char* _text, const VECTOR& _position);
    void CreateGauge(VolumeRow& _row, float _posY);

    // 押された「－」「＋」に合わせて音量を1段階変える。変えたら新しい段階を返す
    int UpdateVolumeRow(const VolumeRow& _row, int _level);
    // 「60%」の文字と豆腐の明るさを今の段階に合わせる
    void RefreshRow(const VolumeRow& _row, int _level);

private:
    UI* mpTitle = nullptr;
    VolumeRow mBgmRow;
    VolumeRow mSeRow;
    UI* mpBackButton = nullptr;
    FocusUI* mpFocus = nullptr;     // キーボード / パッドで選べるようにする
    bool mbBackRequested = false;
};
