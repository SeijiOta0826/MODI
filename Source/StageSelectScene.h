#pragma once
#include "Scene.h"
#include "GameConfig.h"

#include <array>
#include <string>
#include <DxLib.h>

class UI;
class FocusUI;
class DifficultySelectUI;

class StageSelectScene : public Scene
{
public:
    void Initialize() override;
    void Update(float _deltaTime) override;
    void Draw() override;
    void Finalize() override;

private:
    // シーン内の状態
    enum class State
    {
        SelectStage,        // ステージを選んでいる
        SelectDifficulty,   // 難易度選択のポップアップを開いている
        Starting,           // ゲームシーンへの切り替えを予約済み(二重に予約しないため)
    };

    // レイアウト座標(完成イメージ / RecipeBook.png 942×527 上のピクセル座標)
    struct Point
    {
        float x;
        float y;
    };

    // -- 更新 -- //
    void UpdateSelectStage();
    void UpdateSelectDifficulty();
    void OpenDifficultySelect(int _stage);
    void CloseDifficultySelect();

    // -- 生成 -- //
    void CreateBackground();                                            // まな板(画面いっぱい)
    void CreateRecipeBook();                                            // レシピ本
    void CreateTitle();                                                 // 「ステージ選択」
    void CreateEscHud();                                                // 右下の「ESC : タイトルへ戻る」
    void CreateStageCard(int _stage);                                   // 料理枠1つ分
    void CreateDishName(int _stage, Point _center, bool _isCompleted);
    void CreateDish(int _stage, Point _center, bool _isCompleted);
    void CreateItems(int _stage, Point _center);

    // -- 配置の補助 -- //
    void PlaceImage(UI* _ui, float _x, float _y, int _layer) const;     // レイヤーと位置
    void FitImage(UI* _ui, float _width, float _height) const;          // 比率を保って枠内に収める
    void StretchImage(UI* _ui, float _width, float _height) const;      // 比率を無視して引き伸ばす
    VECTOR ToUIPosition(float _x, float _y) const;                      // レイアウト座標 → UIのPosition

private:
    UI* mpBackground = nullptr;

    std::array<UI*, GameConfig::STAGE_COUNT> mpDishPanels{};    // クリック判定も兼ねる [ステージ番号 - 1]
    FocusUI* mpFocus = nullptr;                                 // キーボードでのステージ選択

    State mState = State::SelectStage;
    int mnSelectedStage = 0;                                    // 難易度選択中のステージ番号
    DifficultySelectUI* mpDifficultyUI = nullptr;               // 難易度選択のポップアップ
};