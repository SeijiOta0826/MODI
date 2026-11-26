#include "TutorialScene.h"

// -- Manager -- //
#include "Master.h"
#include "SceneManager.h"
#include "ObjectManager.h"
#include "UIManager.h"
#include "EffectManager.h"
#include "InputManager.h"
#include "SoundManager.h"
#include "ScreenConfig.h"

// -- GameObject / UI / Effect -- //
#include "Stage.h"
#include "Player.h"
#include "Goal.h"
#include "PlayerShapeUI.h"
#include "PauseUI.h"
#include "EffectClear.h"
#include "UI.h"
#include "StageBackGround.h"
#include "MessageUI.h"

#include "Transform.h"

#include "GameProgress.h" // SetTutorialCleared()

#include <DxLib.h>
#include <string>

void TutorialScene::Initialize()
{
    mbIsClear = false;
    mfClearTimer = 0.0f;

    // -- Player・変形UIの生成 (変形UIはInit内でPlayerを探すため後に作る) -- //
    const VECTOR initPos = VGet(200.0f, 350.0f, 0.0f);
    auto player = GetObjectManager()->CreateObject<Player>(initPos);
    if(player)
        mpShapeUI = GetUIManager()->CreateUI<PlayerShapeUI>();
    
    // -- ステージ読み込み -- //
    auto* stage = new Stage();
    stage->Load("Resource/StageMap/Stage0.csv");

    mpGoal = GetObjectManager()->FindObject<Goal>();

    // -- 背景UI -- //
    mpBackgroundUI = GetUIManager()->CreateUI<StageBackGround>();

    // -- ポーズUI -- //
    mpPauseUI = GetUIManager()->CreateUI<PauseUI>();
    if (mpPauseUI != nullptr)
    {
        mpPauseUI->SetEnabled(false);
        mpPauseUI->SetResumeOnly(true);
    }

    // -- 「Skip」ボタン -- //
    mpSkipButton = GetUIManager()->CreateUI<UI>();
    mpSkipButton->LoadUIGraph("Resource/UI/Tutorial/Skip.png");
    mpSkipButton->SetEnabled(true);
    mpSkipButton->SetLayer(1);
    mpSkipButton->GetHoverScaler().SetActive(true);
    mpSkipButton->GetText().SetFontId(FontId::Button);
    mpSkipButton->GetText().SetColor(GetColor(255, 255, 255));
    mpSkipButton->GetText().SetText("Skip");
    mpSkipButton->GetTransform().SetPosition(VGet(480.0f, -280.0f, 0.0f));

    // -- メッセージウィンドウ -- //
    mnTutorialStep = 0;
    mfStepTimer = 0.0f; // ★タイマーを0に初期化
    mpMessageUI = GetUIManager()->CreateUI<MessageUI>();
    // ★最初は導入メッセージを表示！
    SetTutorialMessage("これからチュートリアルを始めるよ！\n基本の操作を一緒に覚えよう！");

    // -- BGM再生 -- //
    SoundManager::GetInstance()->StopBGM();
    SoundManager::GetInstance()->PlayBGM(SoundManager::SOUND_BGM::BGM_Game, true);
}

void TutorialScene::SetTutorialMessage(const char* _text)
{
    if (mpMessageUI == nullptr)
        return;

    mpMessageUI->Show(_text);
}

void TutorialScene::Update(float _deltaTime)
{
    // -- クリア演出中の更新 -- //
    if (mbIsClear)
    {
        // Scene::Updateを通らないので、メッセージだけはここで更新する
        if (mpMessageUI != nullptr)
        {
            mpMessageUI->Update();
        }
        if (GetEffectManager() != nullptr)
        {
            GetEffectManager()->Update(_deltaTime);
        }

        mfClearTimer += _deltaTime;

        if (mnTutorialStep != 99 && mfClearTimer >= 1.8f)
        {
            mnTutorialStep = 99;
            SetTutorialMessage("ここからは材料があるから、\nそれを拾って豆腐料理を作ろう！");
        }

        if (mfClearTimer >= 5.0f ||
            (mnTutorialStep == 99 && mpMessageUI->IsFinished() && InputManager::GetInstance().GetButtonDown(Button::Confirm)))
        {
            GameProgress::SetTutorialCleared(true);   // チュートリアルのクリアを記録
            Master::mpSceneManager->RequestScene(SCENE_TYPE::STAGE_SELECT_SCENE);
        }
        return;
    }

    // -- 「Skip」ボタンが押された場合 -- //
    if (mpSkipButton != nullptr && mpSkipButton->GetButton().IsClicked())
    {
        InputManager::GetInstance().ConsumeButton(Button::Confirm);
        SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_SELECTUI);
        SoundManager::GetInstance()->StopBGM();

        GameProgress::SetTutorialCleared(true);   // チュートリアルのクリアを記録
        Master::mpSceneManager->RequestScene(SCENE_TYPE::STAGE_SELECT_SCENE);
        return;
    }

 
    // -- ポーズ制御 & 最初の挨拶中（Step 0）の操作ロック -- //
    if (mnTutorialStep > 0)
    {
        // Step 0 が終わってからポーズキーを受け付ける
        UpdatePause();
    }

    float gameDeltaTime = _deltaTime;

    // ポーズ中、または「これからチュートリアルを始めるよ（Step 0）」の間は時間を止めて操作不可にする
    if ((mpPauseUI != nullptr && mpPauseUI->IsEnabled()) || mnTutorialStep == 0)
    {
        gameDeltaTime = 0.0f;
    }
    else
    {
        // Step 0 が終わっており、かつポーズ中でないときだけ変形UIの開閉を受け付ける
        UpdateShapeUIToggle();
    }


    // -- 進行ステップ更新 (メッセージの文字送りはScene::Update内のUIManagerで更新される) -- //
    UpdateTutorialStep(_deltaTime);

    Scene::Update(gameDeltaTime);

    // -- ゴール判定 -- //
    if (mpGoal != nullptr && mpGoal->IsPlayerTouching())
    {
        mbIsClear = true;
        mfClearTimer = 0.0f;

        if (mpPauseUI != nullptr) mpPauseUI->SetEnabled(false);
        if (mpShapeUI != nullptr && mpShapeUI->IsEnabled()) mpShapeUI->Close();

        SoundManager::GetInstance()->StopBGM();
        SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_GOALVOICE);

        if (GetEffectManager() != nullptr)
        {
            GetEffectManager()->Play<EffectClear>(
                ScreenConfig::SCREEN_WIDTH / 2.0f,
                ScreenConfig::SCREEN_HEIGHT / 2.0f
            );
        }

        SetTutorialMessage("チュートリアルクリア！ おめでとう！");
    }
}

void TutorialScene::UpdateTutorialStep(float _deltaTime)
{
    auto& input = InputManager::GetInstance();

    // Step 0: 最初の挨拶文を読む時間（文字が出終わってから2.0秒経ったら回転の説明へ）
    if (mnTutorialStep == 0)
    {
        if (mpMessageUI->IsFinished())
        {
            mfStepTimer += _deltaTime;
            if (mfStepTimer >= 2.0f) // ★読む時間の長さ（2.0秒）
            {
                mnTutorialStep = 1;
                SetTutorialMessage("まずは [A][D] キーを押して、\n豆腐を回転させてみよう！");
            }
        }
    }
    // Step 1: 回転入力で変形説明へ
    else if (mnTutorialStep == 1)
    {
        if (input.GetAxis(Axis::Rotation) != 0.0f ||
            CheckHitKey(KEY_INPUT_A) != 0 || CheckHitKey(KEY_INPUT_D) != 0)
        {
            mnTutorialStep = 2;
            SetTutorialMessage("ナイス！ 次は変形キー[E]を押して、\n豆腐の形を変えてみよう！");
        }
    }
    // Step 2: 変形UIを開いたらポーズ説明へ
    else if (mnTutorialStep == 2)
    {
        if (mpShapeUI != nullptr && mpShapeUI->IsEnabled())
        {
            mnTutorialStep = 3;
            SetTutorialMessage("マスをクリックして変形できるよ！\n次は [P]キー でポーズ画面を開いてみよう！");
        }
    }
    // Step 3: ポーズ画面を開いたら再開の案内へ
    else if (mnTutorialStep == 3)
    {
        if (mpPauseUI != nullptr && mpPauseUI->IsEnabled())
        {
            mnTutorialStep = 4;
            SetTutorialMessage("ポーズ画面では再開やステージ選択へ戻れるよ！\n「再開」を押そう！");
        }
    }
    // Step 4: ポーズ画面を閉じたらゴール案内へ
    else if (mnTutorialStep == 4)
    {
        if (mpPauseUI != nullptr && !mpPauseUI->IsEnabled())
        {
            mnTutorialStep = 5;
            SetTutorialMessage("バッチリ！\nそれではステージ右にあるゴールを目指そう！");
        }
    }
}

void TutorialScene::UpdatePause()
{
    if (mpPauseUI == nullptr)
        return;

    if (InputManager::GetInstance().GetButtonDown(Button::Pause))
    {
        // ToggleEnabled() だと「操作説明」中の戻り処理やフォーカスの初期化が行われないので、GameSceneと同じ入口を使う
        mpPauseUI->OnPauseInput();
    }
}

void TutorialScene::UpdateShapeUIToggle()
{
    if (mpShapeUI == nullptr)
        return;

    if (!InputManager::GetInstance().GetButtonDown(Button::Shape))
        return;

    if (mpShapeUI->IsEnabled())
        mpShapeUI->Close();
    else
        mpShapeUI->Open();
}

void TutorialScene::Draw()
{
    // -- 背景描画 -- //
    if (mpBackgroundUI != nullptr)
    {
        mpBackgroundUI->DrawBackground();
    }

    // メッセージウィンドウはMessageUIとしてUIManagerが描画する
    Scene::Draw();
}

void TutorialScene::Finalize()
{
    Scene::Finalize();
}