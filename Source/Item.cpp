#include "Item.h"

// -- Itemデータ -- //
#include "ItemTable.h"

// -- Manager / Camera -- //
#include "Master.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"

// -- Module -- //
#include "Transform.h"
#include "Graphic.h"
#include "Collider.h"

#include "SoundManager.h"

// -- Config -- //
#include "GameConfig.h"

#include <DxLib.h>
#include <algorithm>
#include <cmath>

void Item::InitComponent() {
	AddModule<Transform>();
	AddModule<Graphic>(ItemTable::GetFilePath(mStage, mSlot));
	auto collider = AddModule<Collider>();
	collider->SetHalfSize(VGet(GameConfig::CELL_SIZE / 4.0f, 25.0f, 0.0f));
}

void Item::Init() {
	SetTag(Tag::ITEM);

	if (ItemTable::Find(mStage, mSlot) == nullptr) {
		Destroy();
		return;
	}
	GetModule<Collider>()->AddCollisionTag(Tag::PLAYER_CELL);
}

void Item::Update(float _deltaTime) {
	if (_deltaTime <= 0.0f)		// Pause中は止まる
		return;

	switch (mState) {
	case State::Idle:    UpdateIdle(_deltaTime);    break;
	case State::Picking: UpdatePicking(_deltaTime); break;
	}
	GameObject::Update(_deltaTime);
}

void Item::Draw() {
	if (mState == State::Idle) {
		DrawAura();
	}
	GameObject::Draw();
}

void Item::UpdateIdle(float _deltaTime) {
	// ふわふわ上下に浮かせて「拾えるアイテム」であることを伝える
	if (auto transform = GetModule<Transform>()) {
		VECTOR pos = transform->GetPosition();
		if (!mHasBaseY) {
			mBaseY = pos.y;
			mHasBaseY = true;
		}
		mFloatTimer += _deltaTime;
		pos.y = mBaseY + std::sinf(mFloatTimer * FLOAT_SPEED) * FLOAT_AMPLITUDE;
		transform->SetPosition(pos);
	}

	auto collider = GetModule<Collider>();
	if (!collider || !collider->IsColliding(Tag::PLAYER_CELL))
		return;

	mState = State::Picking;	// IsPicked() が true になる
	mTimer = 0.0f;
	collider->SetEnabled(false);

	//SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_ITEM_GETTING);
}

void Item::UpdatePicking(float _deltaTime) {
	mTimer += _deltaTime;
	const float rate = (std::min)(mTimer / PICK_DURATION, 1.0f);

	// 前半：1.0→1.3倍 / 後半：1.3→0倍
	const float scale = (rate < 0.5f)
		? 1.0f + (rate / 0.5f) * 0.3f
		: 1.3f * (1.0f - (rate - 0.5f) / 0.5f);

	if (auto transform = GetModule<Transform>())
		transform->SetScale(VGet(scale, scale, 1.0f));

	if (rate >= 1.0f)
		Destroy();
}

void Item::DrawAura() {
	auto transform = GetModule<Transform>();
	if (transform == nullptr)
		return;

	VECTOR drawPos = transform->GetPosition();
	float zoom = 1.0f;

	if (Master::mpSceneManager != nullptr) {
		if (auto* currentScene = Master::mpSceneManager->GetCurrentScene()) {
			if (auto* camera = currentScene->GetCamera()) {
				drawPos = camera->WorldToScreen(drawPos);
				zoom = camera->GetZoom();
			}
		}
	}

	// 呼吸するようにほんのり脈動する白い光輪
	const float pulse = 1.0f + 0.08f * std::sinf(mFloatTimer * FLOAT_SPEED);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 55);
	DrawCircleAA(drawPos.x, drawPos.y, AURA_OUTER_RADIUS * zoom * pulse, 32, GetColor(255, 250, 220), TRUE);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 95);
	DrawCircleAA(drawPos.x, drawPos.y, AURA_INNER_RADIUS * zoom * pulse, 32, GetColor(255, 255, 245), TRUE);

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}