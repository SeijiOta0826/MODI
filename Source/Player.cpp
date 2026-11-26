#include "Player.h"

/* memo :
* Player の「基本」部分。
*   生成(Init) → 毎フレームの更新(Update) → セル座標・重心・外側のフチ → Pieceの同期 → リスポーン
* 当たり判定・回転・接地・形状変更は別ファイル(Player.h の memo 参照)。
*/

// -- Module -- //
#include "Transform.h"
#include "Gravity.h"

// -- Piece生成用 -- //
#include "Master.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ObjectManager.h"
#include "PlayerPiece.h"

#include "ScreenConfig.h"
#include "GameConfig.h"
#include "PlayerMath.h"

// -- Sound / Effect / Camera -- //
#include "SoundManager.h"
#include "EffectManager.h"
#include "EffectRespawn.h"
#include "CameraShake.h"

#include <DxLib.h>

namespace
{
	constexpr int N = GameConfig::PLAYER_PIECE_SIZE;
}

namespace
{
	// リスポーン時（トゲ接触・落下時）のカメラシェイク設定
	constexpr float RESPAWN_SHAKE_DURATION = 0.30f; // 揺れる時間（0.3秒）
	constexpr float RESPAWN_SHAKE_POWER = 12.0f; // 揺れの強さ（12ピクセル）
}

Player::Player(const VECTOR& initPos)
	:GameObject()
{
	mvRespawnPos = initPos;
}

void Player::Init()
{
	SetTag(Tag::PLAYER);
	mpTransform->SetPosition(mvRespawnPos);

	mController.Initialize(this);

	CreatePieces();
	SetShape(0);	// 初期形状 : ぼっち(Pieceの同期もここで行われる)
}

void Player::InitComponent()
{
	// AddModule の戻り値を覚えておき、毎回 GetModule で探さずに済むようにする
	mpTransform = AddModule<Transform>();
	mpGravity = AddModule<Gravity>();
}

void Player::CreatePieces()
{
	auto scene = Master::mpSceneManager ? Master::mpSceneManager->GetCurrentScene() : nullptr;
	auto objectManager = scene ? scene->GetObjectManager() : nullptr;
	if (!objectManager)
		return;

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			mPieces[c][r] = objectManager->CreateObject<PlayerPiece>(this);
		}
	}
}

// ============================================================ //
//  毎フレームの更新
// ============================================================ //
void Player::Update(float _deltaTime)
{
	// 変形UIを開いている間は停止(基盤ブロックを動かさないため)
	if (_deltaTime <= 0.0f || mbIsShapeEditing)
		return;

	if (mfInputCooldown > 0.0f)
		mfInputCooldown -= _deltaTime;

	CheckRespawn();

	mController.Update();			// 入力 → 回転
	GameObject::Update(_deltaTime);	// 重力などのModule

	// このあと CollisionManager が Piece の当たり判定を見るので、ここで位置を合わせておく
	UpdatePieceTransforms();
}

// ============================================================ //
//  セル座標・重心・外側のフチ
// ============================================================ //
VECTOR Player::GetCellLocalPosition(int _column, int _row) const
{
	return VGet(
		static_cast<float>(_column - mnPivotColumn) * GameConfig::CELL_SIZE,
		static_cast<float>(_row - mnPivotRow) * GameConfig::CELL_SIZE,
		0.0f
	);
}

VECTOR Player::GetCellWorldPosition(int _column, int _row) const
{
	return PlayerMath::LocalToWorld(
		mpTransform->GetPosition(),
		mpTransform->GetRotation().z,
		GetCellLocalPosition(_column, _row));
}

VECTOR Player::CalculateCenterOfMass() const
{
	VECTOR sum = VGet(0.0f, 0.0f, 0.0f);
	int activeCount = 0;

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			if (!mShape[c][r])
				continue;

			sum = VAdd(sum, GetCellWorldPosition(c, r));
			++activeCount;
		}
	}

	// 有効セルが無いときは Player の座標
	if (activeCount == 0)
		return mpTransform->GetPosition();

	return VScale(sum, 1.0f / static_cast<float>(activeCount));
}

std::vector<OuterEdge> Player::CollectOuterEdges() const
{
	std::vector<OuterEdge> edges;

	const float rotZ = mpTransform->GetRotation().z;
	const float halfCell = GameConfig::CELL_SIZE * 0.5f;

	// 4方向(上・下・左・右)のマスのずれ / セル中心からフチまでのずれ / 外向きの角度(ローカル)
	struct EdgeDir { int dc; int dr; float localX; float localY; float localAngle; };
	const EdgeDir dirs[4] = {
		{  0, -1,  0.0f, -halfCell, -DX_PI_F * 0.5f }, // 上のフチ
		{  0,  1,  0.0f,  halfCell,  DX_PI_F * 0.5f }, // 下のフチ
		{ -1,  0, -halfCell,  0.0f,  DX_PI_F        }, // 左のフチ
		{  1,  0,  halfCell,  0.0f,  0.0f           }  // 右のフチ
	};

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			if (!mShape[c][r])
				continue;

			const VECTOR cellPos = GetCellWorldPosition(c, r);

			for (const auto& dir : dirs)
			{
				// 隣にセルがあるなら内側(つなぎ目)なので出さない
				const int nc = c + dir.dc;
				const int nr = r + dir.dr;
				if (nc >= 0 && nc < N && nr >= 0 && nr < N && mShape[nc][nr])
					continue;

				// プレイヤーの回転に合わせて、フチの座標と外向きの角度を計算
				OuterEdge edge;
				edge.mvPosition = VAdd(cellPos, PlayerMath::RotateOffset(VGet(dir.localX, dir.localY, 0.0f), rotZ));
				edge.mfOutwardAngle = dir.localAngle + rotZ;
				edges.push_back(edge);
			}
		}
	}

	return edges;
}

// ============================================================ //
//  Pieceの同期 (見た目・センサーを Player の姿勢に合わせる)
// ============================================================ //
void Player::UpdatePieceTransforms()
{
	const VECTOR rotation = VGet(0.0f, 0.0f, mpTransform->GetRotation().z);

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			auto piece = mPieces[c][r];
			if (!piece || !mShape[c][r])
				continue;

			auto pieceTransform = piece->GetModule<Transform>();
			if (!pieceTransform)
				continue;

			pieceTransform->SetPosition(GetCellWorldPosition(c, r));
			pieceTransform->SetRotation(rotation);

			// 上下左右の接続状態からオートタイル用のマスクを作る
			int autoTileMask = 0;
			if (r > 0 && mShape[c][r - 1])
				autoTileMask |= 1;
			if (c < N - 1 && mShape[c + 1][r])
				autoTileMask |= 2;
			if (r < N - 1 && mShape[c][r + 1])
				autoTileMask |= 4;
			if (c > 0 && mShape[c - 1][r])
				autoTileMask |= 8;

			piece->SetAutoTileMask(autoTileMask);
		}
	}
}

// ============================================================ //
//  リスポーン
// ============================================================ //
void Player::CheckRespawn()
{
	if (mpTransform->GetPosition().y > ScreenConfig::SCREEN_HEIGHT)
		Respawn();
}

void Player::Respawn()
{
	// 形状を先に戻す(回転軸が選び直されて原点が動くので、位置はそのあとで上書きする)
	SetShape(0);
	mpTransform->SetPosition(mvRespawnPos);
	mpTransform->SetRotation(VGet(0.0f, 0.0f, 0.0f));
	UpdatePieceTransforms();

	mfInputCooldown = 0.0f;
	mGroundContacts.clear();
	SetGroundedState(false);

	UpdatePieceTransforms();

	if (auto* currentScene = Master::mpSceneManager->GetCurrentScene())
	{
		// リスポーン時のチカチカエフェクトを再生
		if (auto* effectManager = currentScene->GetEffectManager())
		{
			effectManager->Play<EffectRespawn>(this);
		}

		// トゲに当たった時・リスポーンした時に画面を揺らす！
		if (auto* cameraShake = currentScene->GetCameraShake())
		{
			cameraShake->Start(RESPAWN_SHAKE_DURATION, RESPAWN_SHAKE_POWER);
		}
	}

	SoundManager::GetInstance()->PlaySE(SoundManager::SOUND_SE::SE_RESPAWN);
}