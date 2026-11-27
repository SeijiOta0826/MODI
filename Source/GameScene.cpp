#include "GameScene.h"

// -- SceneBase -- //
#include "Master.h"
#include "Scene.h"
#include "ObjectManager.h"
#include "ClearSequence.h"
#include "UIManager.h"

// -- Manager(グローバル) -- //
#include "InputManager.h"
#include "SoundManager.h"
#include "ItemRecord.h"
#include "GameProgress.h"

// -- Config -- //
#include "GameConfig.h"
#include "ScreenConfig.h"

// -- GameObject -- //
#include "Stage.h"
#include "Player.h"
#include "Goal.h"
#include "Item.h"
#include "ItemTable.h"
#include "Spike.h"
#include "Transform.h"

// -- UI -- //
#include "StageBackGround.h"
#include "PlayerShapeUI.h"
#include "PauseUI.h"
#include "ClearUI.h"
#include "ItemHUD.h"
#include "RespawnUI.h"

// -- Effect -- //
#include "EffectManager.h"
#include "GameFrameUI.h"

// -- 外部ライブラリ -- //
#include <DxLib.h>

// -- 標準ライブラリ -- //
#include <limits>
#include <string>

void GameScene::Initialize()
{
	m_goalEffect.Clear();
	m_dangerEffect.Clear();
	m_happyEffect.Clear();
	m_sleepEffect.Clear();

	// -- ステージの生成 -- //
	auto* stage = new Stage();
	std::string stageFileName =
		"Resource/StageMap/Stage"
		+ std::to_string(Master::mpSceneManager->GetStageNumber())
		+ ".csv";
	stage->Load(stageFileName);

	// -- 「Player変形UI」の生成 -- //
	mpPlayer = this->GetObjectManager()->FindObject<Player>();
	if (mpPlayer)
		mpShapeUI = GetUIManager()->CreateUI<PlayerShapeUI>();

	// -- 「背景」の生成 -- //
	mpBackgroundUI = GetUIManager()->CreateUI<StageBackGround>();

	// -- 「Goal」の取得 -- //
	mpGoal = this->GetObjectManager()->FindObject<Goal>();

	// -- 「ClearUI」の生成 -- //
	mpClearUI = GetUIManager()->CreateUI<ClearUI>();

	// -- 「PauseUI」の生成 -- //
	mpPauseUI = GetUIManager()->CreateUI<PauseUI>();
	mpPauseUI->SetEnabled(false);

	// -- 「ItemHUD」の生成 -- //
	mpItemHUD = GetUIManager()->CreateUI<ItemHUD>(Master::mpSceneManager->GetStageNumber());
	mpPauseUI->AddMenuContent(mpItemHUD);

	// -- 「RespawnUI」の生成 -- //
	mpRespawnUI = GetUIManager()->CreateUI<RespawnUI>();

	// -- 「GameFrameUI」(画面外周のフレーム)の生成 -- //
	mpGameFrameUI = GetUIManager()->CreateUI<GameFrameUI>();

	SoundManager::GetInstance()->PlayBGM(SoundManager::SOUND_BGM::BGM_Game, true);
}

void GameScene::Update(float deltaTime)
{
	switch (mState)
	{
	case PlayState::Playing:
		// -- 通常プレイ中の更新 -- //
		UpdatePlaying(deltaTime);
		break;

	case PlayState::Clearing:
		// -- ゴール時のスロー・ズーム演出を更新 -- //
		UpdateClearing(deltaTime);
		break;

	case PlayState::Cleared:
		// -- クリア後の更新 -- //
		UpdateCleared(deltaTime);
		break;
	}

	m_goalEffect.Update();
}

void GameScene::UpdatePlaying(float deltaTime)
{
	// -- Pauseの処理 -- //
	UpdatePauseInput();

	// Playerの更新より先に開閉する(開いたフレームからPlayerと移動床を止めるため)
	UpdateShapeInput();

	// ポーズ中 または 変形画面(mpShapeUI)を開いている間は、ゲーム世界(移動床など)の時間を止める
	const bool isPaused = (mpPauseUI != nullptr && mpPauseUI->IsEnabled());
	const bool isShapeEditing = (mpShapeUI != nullptr && mpShapeUI->IsEnabled());
	SetWorldTimeScale((isPaused || isShapeEditing) ? 0.0f : 1.0f);

	UpdateRestartInput(deltaTime);

	// 更新処理
	Scene::Update(deltaTime);
	UpdateItems();
	UpdatePlayerEffects(deltaTime, isPaused, isShapeEditing);
	UpdateDangerEffect();
	UpdateHappyEffect();
	UpdateMissPenalty();

	if (mpGoal != nullptr && mpGoal->IsPlayerTouching())
		EnterClear();
}

void GameScene::EnterClear()
{
	ItemRecord::GetInstance().Commit(Master::mpSceneManager->GetStageNumber(), mPickedItems);

	// クリア演出に入ったらPlayer用エフェクトをすべて消す
	m_dangerEffect.Clear();
	m_happyEffect.Clear();
	m_sleepEffect.Clear();

	// ゴールしたプレイヤーの位置を取得
	const VECTOR playerPosition = mpPlayer->GetModule<Transform>()->GetPosition();

	// プレイヤーを中心にゴール演出を開始
	mClearSequence.Start(GetCamera(), playerPosition);

	if (mpPauseUI != nullptr)
		mpPauseUI->SetEnabled(false);
	if (mpRespawnUI != nullptr)
		mpRespawnUI->SetEnabled(false);

	mState = PlayState::Clearing;
}

void GameScene::UpdateCleared(float deltaTime)
{
	// ゴールボイスが終わったらBGMを流す
	if (!SoundManager::GetInstance()->IsPlayingSE(SoundManager::SOUND_SE::SE_GOALVOICE))
		SoundManager::GetInstance()->PlayBGM(SoundManager::SOUND_BGM::BGM_Title, false);

	// 時間倍率はゴール演出終了時に0にしてあるので、世界は止まったまま
	Scene::Update(deltaTime);
}

void GameScene::Draw()
{
	// 背景（最背面なので一番最初に描く）
	if (mpBackgroundUI != nullptr)
	{
		mpBackgroundUI->DrawBackground();
	}

	Scene::Draw();	// フレーム(GameFrameUI)もUIとしてここで描かれる

	// Effekseer版Dangerエフェクトを描画
	m_dangerEffect.Draw();

	// Effekseer版Happyエフェクトを描画
	m_happyEffect.Draw();

	// Effekseer版Sleepエフェクトを描画
	m_sleepEffect.Draw();

	// Effekseer版ゴールエフェクトを最前面に描画
	m_goalEffect.Draw();
}

void GameScene::UpdateShapeInput()
{
	if (mpShapeUI != nullptr
		&& InputManager::GetInstance().GetButtonDown(Button::Shape)
		&& !GetUIManager()->FindUI<PauseUI>()->IsEnabled())
		mpShapeUI->OnShapeInput();
}

void GameScene::UpdateHappyEffect()
{
	if (mpPlayer == nullptr || mpGoal == nullptr)
	{
		m_happyEffect.Clear();
		return;
	}

	auto* playerTransform = mpPlayer->GetModule<Transform>();
	auto* goalTransform = mpGoal->GetModule<Transform>();
	if (playerTransform == nullptr || goalTransform == nullptr)
	{
		m_happyEffect.Clear();
		return;
	}

	const VECTOR playerPos = playerTransform->GetPosition();
	const VECTOR goalPos = goalTransform->GetPosition();
	const float dx = playerPos.x - goalPos.x;
	const float dy = playerPos.y - goalPos.y;

	constexpr float happyDistSq = EffectHappy::HAPPY_DISTANCE * EffectHappy::HAPPY_DISTANCE;
	const bool isNearGoal = (dx * dx + dy * dy <= happyDistSq);

	m_happyEffect.Update(isNearGoal, playerPos.x, playerPos.y);
}

void GameScene::UpdateDangerEffect()
{

	if (mpPlayer == nullptr)

	{

		m_dangerEffect.Clear();

		return;

	}

	// Player の座標を取得（private な CalculateCenterOfMass を使わず Transform から取得）

	auto* playerTransform = mpPlayer->GetModule<Transform>();

	if (playerTransform == nullptr)

	{

		m_dangerEffect.Clear();

		return;

	}

	const VECTOR playerPos = playerTransform->GetPosition();

	// ステージ上の全 Spike との距離をチェック（sqrt を省くため2乗で比較）

	bool isNearSpike = false;

	constexpr float dangerDistSq = EffectDanger::DANGER_DISTANCE * EffectDanger::DANGER_DISTANCE;

	for (auto* spike : GetObjectManager()->FindObjects<Spike>())

	{

		auto* transform = spike->GetModule<Transform>();

		if (transform == nullptr)

			continue;

		const VECTOR spikePos = transform->GetPosition();

		const float dx = playerPos.x - spikePos.x;

		const float dy = playerPos.y - spikePos.y;

		if (dx * dx + dy * dy <= dangerDistSq)

		{

			isNearSpike = true;

			break;

		}

	}

	m_dangerEffect.Update(isNearSpike, playerPos.x, playerPos.y);

}


void GameScene::UpdateItems()
{
	for (auto* item : GetObjectManager()->FindObjects<Item>())
	{
		if (!item->IsPicked())
			continue;

		const int slot = item->GetSlot();
		if (slot < 0 || slot >= GameConfig::MAX_ITEMS_PER_STAGE)
			continue;

		if (mPickedItems[slot])		// 取得済みなら何もしない
			continue;

		// ここに来るのは「取った最初のフレーム」だけ
		mPickedItems[slot] = true;
		mpItemHUD->SetPicked(slot);

		if (mpBackgroundUI != nullptr)
			mpBackgroundUI->SetPicked(slot);
	}
}

void GameScene::UpdateMissPenalty()
{
	if (mpPlayer == nullptr)
		return;

	// Easy でも毎フレーム受け取って消しておく(フラグが溜まったままにならないように)
	const bool missed = mpPlayer->ConsumeMissed();

	if (missed && GameProgress::GetDifficulty() == Difficulty::Normal)
		ResetPickedItems();
}

void GameScene::ResetPickedItems()
{
	// 何も取っていなければ何もしない
	bool hasPicked = false;
	for (bool isPicked : mPickedItems)
		hasPicked = hasPicked || isPicked;
	if (!hasPicked)
		return;

	mPickedItems.fill(false);

	// ステージ上のアイテムを元の場所に戻す
	for (auto* item : GetObjectManager()->FindObjects<Item>())
		item->ResetPick();

	// 表示(ポーズ画面のHUD・背景の料理)も未取得に戻す
	if (mpItemHUD != nullptr)
		mpItemHUD->ResetPicked();
	if (mpBackgroundUI != nullptr)
		mpBackgroundUI->ResetPicked();
}

void GameScene::UpdatePauseInput()
{
	if (mpPauseUI != nullptr
		&& InputManager::GetInstance().GetButtonDown(Button::Pause)
		&& !GetUIManager()->FindUI<PlayerShapeUI>()->IsEnabled())
		mpPauseUI->OnPauseInput();
}

void GameScene::UpdateRestartInput(float deltaTime)
{
	if (mpRespawnUI == nullptr || mpPlayer == nullptr)
		return;

	// ポーズ中は表示しない
	mpRespawnUI->SetEnabled(!mpPauseUI->IsEnabled());

	// ポーズ中・変形UIを開いている間は受け付けない
	const bool canRestart = !mpPauseUI->IsEnabled() && !mpPlayer->IsShapeEditing();
	const bool isHeld = canRestart && InputManager::GetInstance().GetButton(Button::Restart);

	if (mpRespawnUI->UpdateHold(isHeld, deltaTime))
		mpPlayer->Respawn();
}

void GameScene::UpdatePlayerEffects(float deltaTime, bool isPaused, bool isShapeEditing)
{
	if (mpPlayer == nullptr)
	{
		m_dangerEffect.Clear();
		m_happyEffect.Clear();
		m_sleepEffect.Clear();
		return;
	}

	auto* playerTransform = mpPlayer->GetModule<Transform>();
	if (playerTransform == nullptr)
	{
		m_dangerEffect.Clear();
		m_happyEffect.Clear();
		m_sleepEffect.Clear();
		return;
	}

	const VECTOR playerPos = playerTransform->GetPosition();

	// 一番近いSpikeとの距離の2乗を求める
	float minSpikeDistSq = (std::numeric_limits<float>::max)();
	for (auto* spike : GetObjectManager()->FindObjects<Spike>())
	{
		auto* transform = spike->GetModule<Transform>();
		if (transform == nullptr)
			continue;

		const VECTOR spikePos = transform->GetPosition();
		const float dx = playerPos.x - spikePos.x;
		const float dy = playerPos.y - spikePos.y;
		const float distSq = dx * dx + dy * dy;

		if (distSq < minSpikeDistSq)
		{
			minSpikeDistSq = distSq;
		}
	}

	// Goalとの距離の2乗を求める
	float goalDistSq = (std::numeric_limits<float>::max)();
	if (mpGoal != nullptr)
	{
		if (auto* goalTransform = mpGoal->GetModule<Transform>())
		{
			const VECTOR goalPos = goalTransform->GetPosition();
			const float dx = playerPos.x - goalPos.x;
			const float dy = playerPos.y - goalPos.y;
			goalDistSq = dx * dx + dy * dy;
		}
	}

	constexpr float dangerDistSq = EffectDanger::DANGER_DISTANCE * EffectDanger::DANGER_DISTANCE;
	constexpr float happyDistSq = EffectHappy::HAPPY_DISTANCE * EffectHappy::HAPPY_DISTANCE;

	const bool isNearSpike = (minSpikeDistSq <= dangerDistSq);
	const bool isNearGoal = (goalDistSq <= happyDistSq);

	// 複数同時に出さないよう、一番近い1つだけを選ぶ
	bool showDanger = false;
	bool showHappy = false;

	if (isNearSpike && isNearGoal)
	{
		if (minSpikeDistSq <= goalDistSq)
			showDanger = true;
		else
			showHappy = true;
	}
	else if (isNearSpike)
	{
		showDanger = true;
	}
	else if (isNearGoal)
	{
		showHappy = true;
	}

	m_dangerEffect.Update(showDanger, playerPos.x, playerPos.y);
	m_happyEffect.Update(showHappy, playerPos.x, playerPos.y);

	// DangerやHappyが表示中、またはポーズや変形画面中はSleepを出さない
	if (showDanger || showHappy || isPaused || isShapeEditing)
	{
		m_sleepEffect.Clear();
		return;
	}

	// キーまたはパッドの入力があれば操作中とみなす
	const auto& input = InputManager::GetInstance();
	const bool hasAnyInput =
		(CheckHitKeyAll(DX_CHECKINPUT_KEY | DX_CHECKINPUT_PAD) != 0)
		|| (input.GetAxis(Axis::Rotation) != 0.0f)
		|| (input.GetAxis(Axis::MoveX) != 0.0f)
		|| (input.GetAxis(Axis::MoveY) != 0.0f);

	m_sleepEffect.Update(hasAnyInput, deltaTime, playerPos.x, playerPos.y);
}

void GameScene::UpdateClearing(float deltaTime)
{
	// ゴール演出中はゲーム世界を停止する
	SetWorldTimeScale(0.0f);

	// ゴール時のカメラズーム演出を更新する
	mClearSequence.Update(deltaTime);

	// ゴール演出が終了したか確認する
	if (mClearSequence.IsFinished())
	{
		// ゲーム世界を停止する
		SetWorldTimeScale(0.0f);

		// ゲームBGMを停止する
		SoundManager::GetInstance()->StopBGM();

		// クリアボイスを再生する
		SoundManager::GetInstance()->PlaySE(
			SoundManager::SOUND_SE::SE_GOALVOICE
		);

		// Effekseer版ゴールエフェクトを画面中央で再生する
		m_goalEffect.Trigger(
			ScreenConfig::SCREEN_WIDTH / 2.0f,
			ScreenConfig::SCREEN_HEIGHT / 2.0f
		);

		// ポーズUIを非表示にする
		if (mpPauseUI != nullptr)
			mpPauseUI->SetEnabled(false);

		// リスポーンUIを非表示にする
		if (mpRespawnUI != nullptr)
			mpRespawnUI->SetEnabled(false);

		// クリアUIを表示する
		if (mpClearUI != nullptr)
			mpClearUI->Show(mPickedItems);

		// クリア後の状態へ切り替える
		mState = PlayState::Cleared;
	}

	// UI・エフェクト・カメラシェイクなどを更新する
	Scene::Update(deltaTime);
}

void GameScene::Finalize()
{
	m_goalEffect.Clear();
	m_dangerEffect.Clear();
	m_happyEffect.Clear();
	m_sleepEffect.Clear();

	// GameFrameUIの解放はUIManagerが担当するので、参照だけ手放す
	mpGameFrameUI = nullptr;

	Scene::Finalize();
}