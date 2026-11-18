#include "Player.h"

/* memo :
* Player の「当たり判定」部分。Player の当たり判定はすべてここの QueryPose() を通る。
*
* ■ QueryPose(位置, 角度, 形状, ブロック一覧)
*   入力 : 仮に置く位置・角度・形状と、当たり判定の相手(CollectBlocks で集めたもの)
*   出力 : 接触のリスト(どのセルが・何に・どれだけめり込み・床/壁/天井のどれか)
*   実際の Player は動かさない(「置いてみたらどうなるか」を調べるだけ)。
*
* ■ ブロック一覧は呼び出し側で1回だけ集める
*   CollectBlocks は全オブジェクトを走査するので、1回の処理(回転1回・押し戻し1回など)の中では
*   最初に1回だけ集めて、QueryPose に何度でも渡して使い回す。
*
* ■ 使っている場所
*   PlayerRotation.cpp : 回転してよいか (TestCollisionAtPose)
*   PlayerShape.cpp    : 変形してよいか (TestCollisionAtPose)
*   PlayerGround.cpp   : 毎フレームの押し戻し / 接地プローブ
*   外部ギミック       : CanMoveBy / IsStandingOn
*/

// -- Module -- //
#include "Collider.h"
#include "Transform.h"

// -- ステージブロックの取得用 -- //
#include "Master.h"
#include "SceneManager.h"
#include "Scene.h"
#include "ObjectManager.h"

#include "CollisionManager.h"
#include "GameConfig.h"
#include "PlayerMath.h"

#include <DxLib.h>
#include <cmath>
#include <vector>

namespace
{
	constexpr int N = GameConfig::PLAYER_PIECE_SIZE;
	const VECTOR HALF_CELL_SIZE = VGet(GameConfig::CELL_SIZE / 2.0f, GameConfig::CELL_SIZE / 2.0f, 0.0f);
	constexpr float DIAGONAL_NORMAL_MIN = 0.3f;	// 法線のx・yが両方これより大きければ「斜め」(真横・真上ではない)

	// ブロックの左右どちらかの面に、隣のブロックがぴったりくっついているか
	// (くっついている面 = 床や壁の「つなぎ目」で、本来は外から触れない面)
	bool HasNeighborBlock(const Box2D& _blockBox, float _side, const std::vector<BlockBox>& _blocks)
	{
		constexpr float SAME_POSITION_EPS = 1.0f;
		const float expectedX = _blockBox.position.x + _side * (_blockBox.half.x * 2.0f);

		for (const BlockBox& other : _blocks)
		{
			if (std::fabsf(other.mBox.position.x - expectedX) <= SAME_POSITION_EPS &&
				std::fabsf(other.mBox.position.y - _blockBox.position.y) <= SAME_POSITION_EPS)
				return true;
		}
		return false;
	}

	// セルの一番下(_bottom = true)/ 一番上の頂点のx座標。同じ高さが2つあれば(平ら)その中点
	float ExtremeVertexX(const Box2D& _box, bool _bottom)
	{
		constexpr float SAME_HEIGHT_EPS = 0.5f;

		VECTOR vertices[4];
		_box.GetVertices(vertices);

		float extremeY = vertices[0].y;
		for (const VECTOR& v : vertices)
		{
			if (_bottom ? (v.y > extremeY) : (v.y < extremeY))	// 画面座標はyが下向き
				extremeY = v.y;
		}

		float sumX = 0.0f;
		int count = 0;
		for (const VECTOR& v : vertices)
		{
			if (std::fabsf(v.y - extremeY) <= SAME_HEIGHT_EPS)
			{
				sumX += v.x;
				++count;
			}
		}
		return sumX / static_cast<float>(count);
	}

	// 床 / 天井 として、真上 / 真下に押し出す接触を書き込む
	void SetVerticalContact(const Box2D& _cellBox, const Box2D& _blockBox, bool _isFloor, BodyContact& _contact)
	{
		const float cellExtentY = _cellBox.ProjectToAxis(VGet(0.0f, 1.0f, 0.0f));	// 中心から上下端までの高さ

		if (_isFloor)
		{
			// 床 : セルの下端がブロック上面にめり込んだ量
			const float blockTop = _blockBox.position.y - _blockBox.half.y;
			_contact.mfPenetration = (_cellBox.position.y + cellExtentY) - blockTop;
			_contact.mvCorrection = VGet(0.0f, -_contact.mfPenetration, 0.0f);
			_contact.mbIsFloor = true;
		}
		else
		{
			// 天井 : セルの上端がブロック下面にめり込んだ量
			const float blockBottom = _blockBox.position.y + _blockBox.half.y;
			_contact.mfPenetration = blockBottom - (_cellBox.position.y - cellExtentY);
			_contact.mvCorrection = VGet(0.0f, _contact.mfPenetration, 0.0f);
			_contact.mbIsCeiling = true;
		}
	}

	// SATの結果を「床 / 天井 / 壁」に分類し、押し出し量と一緒に _contact へ書き込む。
	// ※SAT(分離軸判定) : 2つの箱が重なっているか・どの向きにどれだけ押し出せばよいかを調べる方法
	//
	// 例外 : 床ブロック同士のつなぎ目
	//   傾いたセルの角がつなぎ目に少し入ると、SATは「横向きの法線 = 壁」を返してしまう。
	//   横向きの法線でも、その面に隣のブロックがくっついている(=つなぎ目)なら、
	//   セルの位置に応じて床 / 天井として扱い、押し出しも真上 / 真下で計算する。
	//   ※本物の段差の側面には隣のブロックが無いので、従来どおり「壁」になる
	//
	// 例外 : セルの角が、ブロックの平らな面(上面 / 下面)に刺さっている
	//   SATは「めり込みが一番浅い向き」を選ぶ。45度近く傾いたセルの角が床に刺さると、
	//   床の面の向き(真上)より、セルの斜めの面の向きのほうが浅く見えるので、斜めの法線を返す。
	//   すると押し出しが斜めになり、横に滑っていく。
	//   セルの角(一番下 / 一番上の点)がブロックの上面 / 下面の幅の中にあるなら、
	//   ブロックの面の向き(真上 / 真下)で押し出す。
	void ClassifyContact(
		const Box2D& _cellBox,
		const Box2D& _blockBox,
		const CollisionInfo& _info,
		const std::vector<BlockBox>& _blocks,
		BodyContact& _contact)
	{
		_contact.mfPenetration = _info.penetration;
		_contact.mvCorrection = VScale(_info.normal, _info.penetration);

		const float cellCenterY = _cellBox.position.y;
		const float blockTop = _blockBox.position.y - _blockBox.half.y;
		const float blockBottom = _blockBox.position.y + _blockBox.half.y;

		const bool isSideNormal = std::fabsf(_info.normal.x) > 0.5f;
		if (isSideNormal)
		{
			// 法線はブロック→セル の向き。normal.x < 0 なら セル はブロックの左側
			const float side = (_info.normal.x < 0.0f) ? -1.0f : 1.0f;

			if (HasNeighborBlock(_blockBox, side, _blocks))
			{
				if (cellCenterY <= blockTop)
				{
					SetVerticalContact(_cellBox, _blockBox, true, _contact);
					return;
				}
				if (cellCenterY >= blockBottom)
				{
					SetVerticalContact(_cellBox, _blockBox, false, _contact);
					return;
				}
			}
		}

		// -- 例外 : セルの角がブロックの上面 / 下面に刺さっている -- //
		const bool isDiagonalNormal =
			std::fabsf(_info.normal.x) > DIAGONAL_NORMAL_MIN && std::fabsf(_info.normal.y) > DIAGONAL_NORMAL_MIN;
		if (isDiagonalNormal)
		{
			const float blockLeft = _blockBox.position.x - _blockBox.half.x;
			const float blockRight = _blockBox.position.x + _blockBox.half.x;

			if (cellCenterY <= blockTop)
			{
				const float lowestX = ExtremeVertexX(_cellBox, true);
				if (lowestX >= blockLeft && lowestX <= blockRight)
				{
					SetVerticalContact(_cellBox, _blockBox, true, _contact);
					return;
				}
			}
			else if (cellCenterY >= blockBottom)
			{
				const float highestX = ExtremeVertexX(_cellBox, false);
				if (highestX >= blockLeft && highestX <= blockRight)
				{
					SetVerticalContact(_cellBox, _blockBox, false, _contact);
					return;
				}
			}
		}

		// 通常 : SATの法線で分類
		_contact.mbIsFloor = _info.normal.y < -0.5f;
		_contact.mbIsCeiling = _info.normal.y > 0.5f;
		_contact.mbIsWall = isSideNormal;
	}
}

// ============================================================ //
//  当たり判定の相手を集める
// ============================================================ //
std::vector<BlockBox> Player::CollectBlocks() const
{
	std::vector<BlockBox> blocks;

	auto scene = Master::mpSceneManager ? Master::mpSceneManager->GetCurrentScene() : nullptr;
	auto objectManager = scene ? scene->GetObjectManager() : nullptr;
	if (!objectManager)
		return blocks;

	for (auto obj : objectManager->GetObjects())
	{
		if (!obj || obj->GetTag() != Tag::BLOCK)
			continue;

		auto blockCollider = obj->GetModule<Collider>();
		if (!blockCollider || !blockCollider->IsEnabled())
			continue;

		blocks.push_back({ obj, blockCollider->GetBox2D() });
	}

	return blocks;
}

// ============================================================ //
//  姿勢クエリ
// ============================================================ //
std::vector<BodyContact> Player::QueryPose(
	const VECTOR& _pos,
	float _rotZ,
	const PlayerShapeMask& _shape,
	const std::vector<BlockBox>& _blocks) const
{
	std::vector<BodyContact> contacts;

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			if (!_shape[c][r])
				continue;

			// その姿勢のときのセルの箱
			const VECTOR cellPos = PlayerMath::LocalToWorld(_pos, _rotZ, GetCellLocalPosition(c, r));
			const Box2D cellBox = Box2D::Make(cellPos, HALF_CELL_SIZE, _rotZ);

			for (const BlockBox& block : _blocks)
			{
				CollisionInfo info;
				if (!CollisionManager::TestBoxBox(cellBox, block.mBox, info))
					continue;

				BodyContact contact;
				contact.mvCellWorldPos = cellPos;
				contact.mpOther = block.mpObject;
				ClassifyContact(cellBox, block.mBox, info, _blocks, contact);
				contacts.push_back(contact);
			}
		}
	}

	return contacts;
}

// ============================================================ //
//  回転・変形の判断用のまとめ
// ============================================================ //
Player::PoseCollisionResult Player::TestCollisionAtPose(
	const VECTOR& _pos,
	float _rotZ,
	const PlayerShapeMask& _shape,
	const std::vector<BlockBox>& _blocks) const
{
	PoseCollisionResult result;

	for (const BodyContact& contact : QueryPose(_pos, _rotZ, _shape, _blocks))
	{
		result.isColliding = true;
		result.hasFloorCollision |= contact.mbIsFloor;
		result.hasCeilingCollision |= contact.mbIsCeiling;
		result.hasWallCollision |= contact.mbIsWall;

		if (contact.mfPenetration > result.maxPenetration)
			result.maxPenetration = contact.mfPenetration;

		// 押し出し量 : 横は一番大きいもの、縦は一番上向き(-Y)のもの
		if (std::fabsf(contact.mvCorrection.x) > std::fabsf(result.correction.x))
			result.correction.x = contact.mvCorrection.x;
		if (contact.mvCorrection.y < result.correction.y)
			result.correction.y = contact.mvCorrection.y;
	}

	return result;
}

float Player::MaxPenetration(const std::vector<BodyContact>& _contacts)
{
	float maxPenetration = 0.0f;
	for (const BodyContact& contact : _contacts)
	{
		if (contact.mfPenetration > maxPenetration)
			maxPenetration = contact.mfPenetration;
	}
	return maxPenetration;
}

bool Player::IsPoseAcceptable(const PoseCollisionResult& _result)
{
	return !_result.isColliding || _result.maxPenetration <= GameConfig::ALLOWED_PENETRATION;
}

bool Player::CanFloorKick(const PoseCollisionResult& _result)
{
	return _result.hasFloorCollision && !_result.hasCeilingCollision && !_result.hasWallCollision;
}

// ============================================================ //
//  外部ギミック向けの判定
// ============================================================ //
bool Player::IsStandingOn(const GameObject* _object) const
{
	if (_object == nullptr)
		return false;

	for (const BodyContact& contact : mGroundContacts)
	{
		if (contact.mpOther == _object)
			return true;
	}
	return false;
}

bool Player::CanMoveBy(const VECTOR& _offset) const
{
	const VECTOR movedPos = VAdd(mpTransform->GetPosition(), _offset);
	return IsPoseAcceptable(TestCollisionAtPose(movedPos, mpTransform->GetRotation().z, mShape, CollectBlocks()));
}
