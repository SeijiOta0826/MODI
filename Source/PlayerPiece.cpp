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

	// 初期状態は単体タイル(マスク0)を設定する
	SetAutoTileMask(0);
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

void PlayerPiece::SetAutoTileMask(int _mask)
{
	auto graphic = GetModule<Graphic>();
	if (!graphic)
		return;

	const int clampedMask = _mask & 0x0F;
	const int srcX = (clampedMask % TILE_COLS) * TILE_SIZE;
	const int srcY = (clampedMask / TILE_COLS) * TILE_SIZE;
	graphic->SetSourceRect(srcX, srcY, TILE_SIZE, TILE_SIZE);
}