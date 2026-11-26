// Item.h
#pragma once
#include "GameObject.h"

class Item : public GameObject {
public:
	Item(int _stage, int _slot) : mStage(_stage), mSlot(_slot) {}
	~Item() = default;

	void Init() override;
	void InitComponent() override;
	void Update(float _deltaTime) override;

	bool IsPicked() const { return mState != State::Idle; }

	// 取得前の状態に戻す(難易度Normalでミスしたときに GameScene が呼ぶ)
	void ResetPick();
	int GetSlot() const { return mSlot; }

private:
	enum class State { Idle, Picking, Hidden };	// 待機中 / 取得アニメ中 / 取得済み(非表示)

	void UpdateIdle();
	void UpdatePicking(float _deltaTime);

	static constexpr float PICK_DURATION = 0.25f;

	int mStage;
	int mSlot;
	State mState = State::Idle;
	float mTimer = 0.0f;
	VECTOR mvBaseScale = VGet(1.0f, 1.0f, 1.0f);	// 取得アニメ前の大きさ(リセットで戻すため)
};