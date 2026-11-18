#include "PlayerPiece.h"

// Module関係
#include "Transform.h"
#include "Graphic.h"
#include "Collider.h"

#include "GameConfig.h"

PlayerPiece::PlayerPiece(Player* _player)
	: GameObject()
{
	// -- ハブのPlayerを設定 -- //
	mpPlayer = _player;
}

void PlayerPiece::Init()
{
	// -- タグ設定 -- //
	SetTag(Tag::PLAYER_CELL);
}

void PlayerPiece::InitComponent()
{
	// -- Module追加 -- //
	AddModule<Transform>();
	AddModule<Graphic>(GetModelFilename());
	AddModule<Collider>();

	// -- センサー用の当たり判定 -- //
	// (Pieceからは何も判定しない。StageBlockなど相手側が Tag::PLAYER_CELL を指定して検知する)
	auto collider = GetModule<Collider>();
	collider->SetHalfSize(VGet(GameConfig::CELL_SIZE / 2.0f, GameConfig::CELL_SIZE / 2.0f, 0.0f));
}

void PlayerPiece::SetActive(bool _active)
{
	mbIsActive = _active;

	if (auto transform = GetModule<Transform>())
		transform->SetEnabled(_active);
	
	if (auto graphic = GetModule<Graphic>())
		graphic->SetEnabled(_active);

	if (auto collider = GetModule<Collider>())
		collider->SetEnabled(_active);
}
