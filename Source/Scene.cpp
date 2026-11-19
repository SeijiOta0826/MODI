#include "Scene.h"
#include "ObjectManager.h"
#include "UIManager.h"
#include "CollisionManager.h"
#include "EffectManager.h"
#include "InputManager.h"
#include "Camera.h"
#include "CameraShake.h"
#include "EffekseerManager.h"

Scene::Scene()
{
	mpObjectManager = new ObjectManager();
	mpUIManager = new UIManager();
	mpCollisionManager = new CollisionManager();
	mpEffectManager = new EffectManager();
	mpCamera = new Camera();
	mpCameraShake = new CameraShake();
}

Scene::~Scene()
{
	// -- 所有するポインタの解放 -- //
	delete mpObjectManager;
	mpObjectManager = nullptr;

	delete mpUIManager;
	mpUIManager = nullptr;

	delete mpCollisionManager;
	mpCollisionManager = nullptr;

	delete mpEffectManager;
	mpEffectManager = nullptr;

	delete mpCamera;
	mpCamera = nullptr;

	delete mpCameraShake;
	mpCameraShake = nullptr;
}

void Scene::Finalize()
{
	// -- CollisionManagerに登録されているCollider群の解放 -- //
	if (mpCollisionManager != nullptr)
	{
		mpCollisionManager->Finalize();
	}

	// -- ObjectManagerの所有するObject群の解放 -- //
	if (mpObjectManager != nullptr)
	{
		mpObjectManager->Clear();
	}

	if (mpUIManager != nullptr)
	{
		mpUIManager->Clear();
	}

	// -- EffectManagerの所有するObject群の解放 -- //
	if (mpEffectManager != nullptr)
	{
		mpEffectManager->Clear();
	}

	// -- シーン終了時にカメラの揺れをリセット -- //
	if (mpCameraShake != nullptr)
	{
		mpCameraShake->Stop();
	}
	if (mpCamera != nullptr)
	{
		mpCamera->Initialize();
	}
}

void Scene::Update(float _deltaTime)
{
	UpdateWorld(_deltaTime * mfWorldTimeScale);
	UpdateOverlay(_deltaTime);
}

void Scene::UpdateWorld(float _worldDelta)
{
	// 時間が止まっている間は、移動・当たり判定・押し戻しをまとめて止める
	// (押し戻しや倒れ込みは deltaTime を使わないため、ここで止めないと動いてしまう)
	if (_worldDelta <= 0.0f)
		return;

	if (mpObjectManager != nullptr)
		mpObjectManager->Update(_worldDelta);

	if (mpCollisionManager != nullptr)
		mpCollisionManager->Update();

	if (mpObjectManager != nullptr)
		mpObjectManager->ResolveCollision();
}

void Scene::UpdateOverlay(float _realDelta)
{
	if (mpUIManager != nullptr)
		mpUIManager->Update();

	if (mpEffectManager != nullptr)
	{
		mpEffectManager->Update(_realDelta);
		mpEffectManager->PlayClickRipple();
	}

	EffekseerManager::GetInstance().Update();

	// -- カメラシェイクの計算結果をカメラに反映する（毎フレーム自動実行） -- //
	if (mpCameraShake != nullptr && mpCamera != nullptr)
	{
		mpCameraShake->Update(_realDelta);
		mpCamera->SetShakeOffset(mpCameraShake->GetOffset());
	}
}

void Scene::Draw()
{
	// 0. カメラの状態をDxLibへ反映（今後Effekseerを使うときにも自動で同期される）
	if (mpCamera != nullptr)
	{
		mpCamera->Apply();
	}

	// 1. 背面エフェクト（背景の直後・ステージやキャラの後ろに湯気を描画）
	if (mpEffectManager != nullptr)
	{
		mpEffectManager->DrawBackground();
	}

	// 2. ゲームオブジェクト（ステージブロック・プレイヤーなど）
	if (mpObjectManager != nullptr)
	{
		mpObjectManager->Draw();
	}

	// 3. UI（ポーズ画面・クリア画面など）
	if (mpUIManager != nullptr)
	{
		mpUIManager->Draw();
	}

	// 4. 前面エフェクト（ClickEffectなどを最前面に描画）
	if (mpEffectManager != nullptr)
	{
		mpEffectManager->Draw();
	}
}