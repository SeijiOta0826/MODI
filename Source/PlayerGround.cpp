#include "Player.h"

/* memo :
* Player の「押し戻し・接地」部分。
*
* ■ 押し戻し (ResolvePenetration)
*   QueryPose で今の姿勢のめり込みを調べ、押し出す。これを最大 MAX_RESOLVE_ITERATIONS 回繰り返す。
*   (1回で押し切ると行き過ぎて逆にめり込むことがあるので、少しずつ押して確認し直す = 反復解決)
*
* ■ 接地プローブ (UpdateGroundContacts)
*   押し戻しが終わるとめり込みはほぼ0になり、「床に触れている」かどうかが分からなくなる。
*   そこで、少しだけ下(GROUND_PROBE_DISTANCE)にずらした位置で QueryPose し、
*   床に当たれば「その床に乗っている」とみなす。
*
* ■ 接地・倒れ込み
*   接地プローブで見つけた床接触の「支えている範囲」から判断する。
*   (平らに置かれたセルは下の辺全体、傾いたセルは角の1点で床を支えている)
*     床に触れていない       → 空中
*     重心が支えの範囲内     → 安定して接地(90度単位にほぼ揃っていれば、床に触れている点を中心にきっちり揃える)
*     重心が支えの範囲外     → 倒れる側の支えの端を支点に少し倒す(毎フレーム繰り返して倒れ込む)
*
* ■ 倒れ込みの行き過ぎ防止
*   1回に TOPPLE_SPEED_DEG ずつ倒すと、つり合う角度(例 : L字が2つの角で立つ135度)を飛び越して
*   反対側に倒れ、また戻る…を繰り返してガタガタ震える(ジッター)。
*   そこで、倒した先で地形にめり込むなら「ぶつかる直前の角度」を二分探索で探してそこで止める。
*   次のフレームで2点目の支えが見つかり、安定して接地する。
*   少しも倒れられない(何かにもたれている)ときは、そこで止まっているとみなして接地扱いにする。
*/

// -- Module -- //
#include "Transform.h"
#include "Gravity.h"

#include "GameConfig.h"
#include "PlayerMath.h"

// -- Effect -- //
#include "Master.h"
#include "SceneManager.h"
#include "Scene.h"
#include "EffectManager.h"
#include "EffectFall.h"

#include <DxLib.h>
#include <cmath>
#include <cfloat>
#include <algorithm>	// std::sort
#include <iterator>		// std::begin, std::end

namespace
{
	constexpr float RIGHT_ANGLE = DX_PI_F / 2.0f;		// 90度
	constexpr float NEARLY_FLAT_TOLERANCE = 0.08f;		// 90度単位から何ラジアン以内なら「平ら」とみなすか
	constexpr float MULTI_SUPPORT_WIDTH = GameConfig::CELL_SIZE * 0.7f;	// これ以上離れた2点で支えていれば安定

	constexpr int MAX_RESOLVE_ITERATIONS = 4;		// 押し戻しを繰り返す最大回数
	constexpr float GROUND_PROBE_DISTANCE = 2.0f;	// 接地プローブで下にずらす距離

	constexpr int TOPPLE_SEARCH_ITERATIONS = 6;			// 倒れ込みで「ぶつかる直前」を探す二分探索の回数
	constexpr float TOPPLE_PENETRATION_TOLERANCE = 1.0f;	// 倒れ込みで許すめり込みの増加量(px)
	constexpr float MIN_TOPPLE_STEP = 0.1f * DX_PI_F / 180.0f;	// これより小さくしか倒れられないなら「倒れられない」
	const VECTOR HALF_CELL_SIZE = VGet(GameConfig::CELL_SIZE / 2.0f, GameConfig::CELL_SIZE / 2.0f, 0.0f);

	// 角度が 90度単位(0, 90, 180, 270度)にほぼ一致しているか
	bool IsNearlyFlat(float _rotZ)
	{
		float modAngle = std::fmodf(std::fabsf(_rotZ), RIGHT_ANGLE);
		if (modAngle < 0.0f)
			modAngle += RIGHT_ANGLE;

		return modAngle < NEARLY_FLAT_TOLERANCE || modAngle > (RIGHT_ANGLE - NEARLY_FLAT_TOLERANCE);
	}

	// 各方向(+X, -X, +Y, -Y)で一番大きい押し戻しを採用して合計する
	// (同じ方向の押し戻しを足し合わせると押し戻しすぎるため)
	VECTOR CombineCorrections(const std::vector<BodyContact>& _contacts)
	{
		float minX = 0.0f, maxX = 0.0f;
		float minY = 0.0f, maxY = 0.0f;

		for (const BodyContact& contact : _contacts)
		{
			const VECTOR& c = contact.mvCorrection;
			if (c.x > maxX) maxX = c.x;
			if (c.x < minX) minX = c.x;
			if (c.y > maxY) maxY = c.y;
			if (c.y < minY) minY = c.y;
		}

		return VGet(maxX + minX, maxY + minY, 0.0f);
	}

	// セルが床を支えている範囲(一番低いところの左端・右端)を _contact に書き込む
	//   ほぼ平ら(90度単位からのズレが小さい) : 下の辺全体(低い頂点2つ)
	//   傾いている                           : 一番低い角の1点
	void SetSupportRange(float _rotZ, BodyContact& _contact)
	{
		VECTOR vertices[4];
		Box2D::Make(_contact.mvCellWorldPos, HALF_CELL_SIZE, _rotZ).GetVertices(vertices);

		// 低い順(yが大きい順。画面座標はyが下向き)に並べる
		std::sort(std::begin(vertices), std::end(vertices),
			[](const VECTOR& _a, const VECTOR& _b) { return _a.y > _b.y; });

		const int supportCount = IsNearlyFlat(_rotZ) ? 2 : 1;

		_contact.mvSupportLeft = vertices[0];
		_contact.mvSupportRight = vertices[0];
		for (int i = 1; i < supportCount; ++i)
		{
			if (vertices[i].x < _contact.mvSupportLeft.x)  _contact.mvSupportLeft = vertices[i];
			if (vertices[i].x > _contact.mvSupportRight.x) _contact.mvSupportRight = vertices[i];
		}
	}

	// 全接触の支えの端のうち、x が _x に一番近い点
	VECTOR FindClosestSupportPoint(const std::vector<BodyContact>& _contacts, float _x)
	{
		VECTOR closest = _contacts[0].mvSupportLeft;
		float closestDist = FLT_MAX;

		for (const auto& contact : _contacts)
		{
			for (const VECTOR& point : { contact.mvSupportLeft, contact.mvSupportRight })
			{
				const float dist = std::fabsf(point.x - _x);
				if (dist < closestDist)
				{
					closestDist = dist;
					closest = point;
				}
			}
		}
		return closest;
	}
}

// ============================================================ //
//  衝突処理 (全オブジェクトの Update が終わった後に呼ばれる)
// ============================================================ //
void Player::EndCollisionResolution()
{
	GameObject::EndCollisionResolution();

	// ブロックの一覧はこの処理の間は変わらないので、1回だけ集めて使い回す
	const std::vector<BlockBox> blocks = CollectBlocks();

	// 変形UIを開いている間も、めり込みの押し戻しだけは行う
	// (万が一外力で押されてもステージブロックをすり抜けないように)
	ResolvePenetration(blocks);

	// 変形UIを開いている間は接地・倒れ込み回転は行わない
	if (!mbIsShapeEditing)
	{
		UpdateGroundContacts(blocks);
		ProcessGroundAndTopple(blocks);
	}

	// 接地・倒れ込みは mShape から計算するので、Pieceの同期は最後に1回だけでよい
	UpdatePieceTransforms();
}

// ============================================================ //
//  押し戻し (反復解決)
// ============================================================ //
void Player::ResolvePenetration(const std::vector<BlockBox>& _blocks)
{
	const float rotZ = mpTransform->GetRotation().z;

	for (int i = 0; i < MAX_RESOLVE_ITERATIONS; ++i)
	{
		const VECTOR pos = mpTransform->GetPosition();
		VECTOR correction = CombineCorrections(QueryPose(pos, rotZ, mShape, _blocks));

		// 不感帯 : ごく小さい押し戻しは無視(細かい振動を防ぐ)
		if (std::fabsf(correction.x) < GameConfig::PENETRATION_DEADZONE)
			correction.x = 0.0f;
		if (std::fabsf(correction.y) < GameConfig::PENETRATION_DEADZONE)
			correction.y = 0.0f;

		if (correction.x == 0.0f && correction.y == 0.0f)
			break;	// めり込みが解消された

		// 減衰 : 押し戻しすぎ(行き過ぎて逆にめり込む)を防ぐ。残りは次の繰り返しで押す
		mpTransform->SetPosition(VAdd(pos, VScale(correction, GameConfig::COLLISION_DAMPING)));
	}
}

// ============================================================ //
//  接地プローブ
// ============================================================ //
void Player::UpdateGroundContacts(const std::vector<BlockBox>& _blocks)
{
	mGroundContacts.clear();

	const float rotZ = mpTransform->GetRotation().z;
	const VECTOR probeOffset = VGet(0.0f, GROUND_PROBE_DISTANCE, 0.0f);
	const VECTOR probePos = VAdd(mpTransform->GetPosition(), probeOffset);

	for (BodyContact& contact : QueryPose(probePos, rotZ, mShape, _blocks))
	{
		if (!contact.mbIsFloor)
			continue;

		// プローブでずらした分を戻して、実際のセル位置にする
		contact.mvCellWorldPos = VSub(contact.mvCellWorldPos, probeOffset);
		SetSupportRange(rotZ, contact);
		mGroundContacts.push_back(contact);
	}
}

// ============================================================ //
//  接地・倒れ込み
// ============================================================ //
void Player::ProcessGroundAndTopple(const std::vector<BlockBox>& _blocks)
{
	// -- 空中 -- //
	if (mGroundContacts.empty())
	{
		SetGroundedState(false);
		return;
	}

	// -- 安定して接地 -- //
	if (IsStableOnGround())
	{
		SetGroundedState(true);
		SnapRotationIfNearlyFlat();
		return;
	}

	// -- 不安定(重心が支えの外) -- //
	// 回転入力の直後は倒れ込みを止める(入力と倒れ込みが打ち消し合わないように)
	if (mfInputCooldown > 0.0f)
	{
		SetGroundedState(false);
		return;
	}

	// 倒れる先がふさがっていて少しも倒れられない = 何かにもたれて止まっている → 接地扱い
	const bool hasToppled = Topple(_blocks);
	SetGroundedState(!hasToppled);
}

bool Player::IsStableOnGround() const
{
	// 支えている範囲(x方向) : 全接触の支えの左端〜右端
	float minSupportX = FLT_MAX;
	float maxSupportX = -FLT_MAX;
	for (const auto& contact : mGroundContacts)
	{
		if (contact.mvSupportLeft.x < minSupportX) minSupportX = contact.mvSupportLeft.x;
		if (contact.mvSupportRight.x > maxSupportX) maxSupportX = contact.mvSupportRight.x;
	}

	// 重心が支えの範囲内か
	const VECTOR com = CalculateCenterOfMass();
	const bool isComInsideSupport =
		(com.x >= minSupportX - GameConfig::STABLE_GROUND_THRESHOLD_X) &&
		(com.x <= maxSupportX + GameConfig::STABLE_GROUND_THRESHOLD_X);

	if (!isComInsideSupport)
		return false;

	// 平らに置かれている or 離れた2点以上で支えている
	const bool isNearlyFlat = IsNearlyFlat(mpTransform->GetRotation().z);
	const bool isMultiPointSupport = (maxSupportX - minSupportX) >= MULTI_SUPPORT_WIDTH;

	return isNearlyFlat || isMultiPointSupport;
}

void Player::SnapRotationIfNearlyFlat()
{
	const float rotZ = mpTransform->GetRotation().z;

	if (!IsNearlyFlat(rotZ))
		return;

	const float snappedRotZ = std::roundf(rotZ / RIGHT_ANGLE) * RIGHT_ANGLE;
	const float delta = snappedRotZ - rotZ;
	if (delta == 0.0f)
		return;

	// 床に触れている点(重心の真下に一番近いもの)を中心に回す。
	// Playerの原点を中心に回すと、支えている点が床に沈んだり浮いたりして横にずれていくため
	const VECTOR pivot = FindClosestSupportPoint(mGroundContacts, CalculateCenterOfMass().x);
	const VECTOR offsetFromPivot = VSub(mpTransform->GetPosition(), pivot);

	mpTransform->SetPosition(VAdd(pivot, PlayerMath::RotateOffset(offsetFromPivot, delta)));
	mpTransform->SetRotation(VGet(0.0f, 0.0f, snappedRotZ));
}

bool Player::Topple(const std::vector<BlockBox>& _blocks)
{
	const float rotZ = mpTransform->GetRotation().z;

	// -- 1. 支えの右端・左端 -- //
	VECTOR rightmost = mGroundContacts[0].mvSupportRight;
	VECTOR leftmost = mGroundContacts[0].mvSupportLeft;
	for (const auto& contact : mGroundContacts)
	{
		if (contact.mvSupportRight.x > rightmost.x) rightmost = contact.mvSupportRight;
		if (contact.mvSupportLeft.x < leftmost.x)   leftmost = contact.mvSupportLeft;
	}

	// -- 2. 倒れる向き : 重心が支えより右なら時計回り(+)、左なら反時計回り(-) -- //
	const VECTOR com = CalculateCenterOfMass();
	float direction = 0.0f;
	if (com.x > rightmost.x)
		direction = 1.0f;
	else if (com.x < leftmost.x)
		direction = -1.0f;
	else
		direction = (std::sinf(rotZ) >= 0.0f) ? 1.0f : -1.0f;	// 角1点の真上でつり合っている : 今傾いている向きへ

	// -- 3. 支点 : 倒れる側の支えの端(右に倒れるなら右端) -- //
	const VECTOR pivot = (direction > 0.0f) ? rightmost : leftmost;

	// -- 4. 支点まわりに _step だけ回した姿勢と、そのときのめり込み量 -- //
	const VECTOR offsetFromPivot = VSub(mpTransform->GetPosition(), pivot);
	auto posAfter = [&](float _step) {
		return VAdd(pivot, PlayerMath::RotateOffset(offsetFromPivot, _step));
	};
	auto penetrationAfter = [&](float _step) {
		return MaxPenetration(QueryPose(posAfter(_step), rotZ + _step, mShape, _blocks));
	};

	// -- 5. 回した先でめり込むなら、ぶつかる直前の角度を二分探索で探す(行き過ぎ防止) -- //
	//    今すでにあるめり込み(支点の床など)より TOLERANCE 以上深くならない範囲で一番大きく回す
	const float fullStep = direction * PlayerMath::DegToRad(GameConfig::TOPPLE_SPEED_DEG);
	const float allowedPenetration = penetrationAfter(0.0f) + TOPPLE_PENETRATION_TOLERANCE;

	float step = fullStep;
	if (penetrationAfter(fullStep) > allowedPenetration)
	{
		float okRate = 0.0f;	// ここまでは回せる(割合)
		float ngRate = 1.0f;	// ここまで回すとめり込む(割合)
		for (int i = 0; i < TOPPLE_SEARCH_ITERATIONS; ++i)
		{
			const float midRate = (okRate + ngRate) * 0.5f;
			if (penetrationAfter(fullStep * midRate) <= allowedPenetration)
				okRate = midRate;
			else
				ngRate = midRate;
		}
		step = fullStep * okRate;
	}

	if (std::fabsf(step) < MIN_TOPPLE_STEP)
		return false;	// ふさがっていて倒れられない

	// -- 6. 支点まわりに回す -- //
	mpTransform->SetPosition(posAfter(step));
	mpTransform->SetRotation(VGet(0.0f, 0.0f, rotZ + step));
	return true;
}

void Player::SetGroundedState(bool _grounded)
{
	// 空中から着地した瞬間、全セルの「外側のフチ」から外向きに水飛沫を出す
	if (!mbIsGrounded && _grounded)
	{
		auto scene = Master::mpSceneManager ? Master::mpSceneManager->GetCurrentScene() : nullptr;
		if (auto effectManager = scene ? scene->GetEffectManager() : nullptr)
		{
			for (const OuterEdge& edge : CollectOuterEdges())
				effectManager->Play<EffectFall>(edge.mvPosition.x, edge.mvPosition.y, edge.mfOutwardAngle);
		}
	}

	mbIsGrounded = _grounded;

	mpGravity->SetGrounded(_grounded);
}
