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
	case State::Hidden:  break;
	}
	GameObject::Update(_deltaTime);
}

void Item::Draw() {
	if (mState == State::Idle) {
		//DrawAura();
	}
	GameObject::Draw();
}

void Item::UpdateIdle(float _deltaTime) {
	auto collider = GetModule<Collider>();
	if (!collider || !collider->IsColliding(Tag::PLAYER_CELL))
		return;

	mState = State::Picking;	// IsPicked() が true になる
	mTimer = 0.0f;
	collider->SetEnabled(false);

	if (auto transform = GetModule<Transform>())
		mvBaseScale = transform->GetScale();

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
		transform->SetScale(VGet(mvBaseScale.x * scale, mvBaseScale.y * scale, mvBaseScale.z));

	// memo : 以前は Destroy() していたが、難易度Normalでミスしたときに戻せるよう、消さずに隠すだけにした
	if (rate >= 1.0f)
	{
		mState = State::Hidden;
		if (auto graphic = GetModule<Graphic>())
			graphic->SetEnabled(false);
	}
}

void Item::ResetPick()
{
	mState = State::Idle;	// IsPicked() が false に戻る
	mTimer = 0.0f;

	if (auto transform = GetModule<Transform>())
		transform->SetScale(mvBaseScale);
	if (auto graphic = GetModule<Graphic>())
		graphic->SetEnabled(true);
	if (auto collider = GetModule<Collider>())
		collider->SetEnabled(true);
}