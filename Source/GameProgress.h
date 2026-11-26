#pragma once

// 難易度
enum class Difficulty
{
    Easy,       // 今まで通り : ミキサー / 奈落でリスポーンするだけ
    Normal,     // Easy + ミスすると今回のプレイで取ったアイテムもリセット
};

class GameProgress
{
public:
    // -- チュートリアルの終了状態のアクセサ -- //
    static void SetTutorialCleared(bool _isCleared) { mbIsTutorialCleared = _isCleared; }
    static bool IsTutorialCleared() { return mbIsTutorialCleared; }

    // -- 難易度のアクセサ(StageSelectSceneで設定し、GameSceneで参照する) -- //
    static void SetDifficulty(Difficulty _difficulty) { mDifficulty = _difficulty; }
    static Difficulty GetDifficulty() { return mDifficulty; }

private:
    static bool mbIsTutorialCleared;    // チュートリアルの終了を示す
    static Difficulty mDifficulty;      // 選ばれている難易度
};