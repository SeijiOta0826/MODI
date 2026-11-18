#include "Player.h"

/* memo :
* Player の「回転」部分。
*
* ■ TryRotateWithShift の考え方
*   回転の中心は「重心」(見た目の真ん中で回るので直感的)。
*   回す前に「回した後の姿勢」で当たり判定(TestCollisionAtPose → QueryPose)をして、めり込むなら回さない。
*   試す姿勢は次の順番:
*     候補1 : 回転方向へ少し横にずらして回す(転がるように進むため)
*     候補2 : その場で回す(壁際でずらせないとき用)
*   それぞれ
*     (a) そのまま置けるならOK
*     (b) 床だけに当たっているなら、押し上げて置けるか試す(床キック)
*   全部ダメなら回転キャンセル。
*
* ■ 床への吸着 (SnapDownToFloor)
*   床の上で回すと、形によっては下の角が持ち上がって床から浮く。
*   重力で落ちるまで待つと、回し続けている間プカプカ浮いて見えるので、
*   床の上で回したときは、回した直後に「床に触れるところ」まで下げる。
*   (下に床が無い = 段差の端などでは下げない。そのあとは重力に任せる)
*/

// -- Module -- //
#include "Transform.h"

#include "GameConfig.h"
#include "PlayerMath.h"

#include <DxLib.h>
#include <cmath>
#include <cfloat>

namespace
{
	constexpr float SNAP_DOWN_MAX = GameConfig::CELL_SIZE * 0.25f;	// 床への吸着で下げる最大距離(これより下に床が無ければ下げない)
	constexpr float SNAP_DOWN_TOLERANCE = 0.5f;						// 吸着で許すめり込みの増加量(px)
	constexpr int SNAP_DOWN_SEARCH_ITERATIONS = 8;					// 床に触れる高さを探す二分探索の回数
}

// ============================================================ //
//  回転
// ============================================================ //
bool Player::TryRotateWithShift(float _rotateDirection)
{
	if (_rotateDirection == 0.0f)
		return false;

	const float step = _rotateDirection * PlayerMath::DegToRad(GameConfig::ROTATION_STEP_DEG);
	const float candidateAngle = mpTransform->GetRotation().z + step;

	// 重心を中心に回したときの Player の原点
	const VECTOR com = CalculateCenterOfMass();
	const VECTOR rotatedPos = VAdd(com, PlayerMath::RotateOffset(VSub(mpTransform->GetPosition(), com), step));

	const VECTOR candidates[2] = {
		VAdd(rotatedPos, VGet(_rotateDirection * GameConfig::ROTATION_SHIFT_X, 0.0f, 0.0f)),	// 候補1 : 横にずらす
		rotatedPos																				// 候補2 : その場
	};

	// 回す前に床の上にいたか(床の上なら、回したあと床に吸着させる)
	const bool wasOnFloor = !mGroundContacts.empty();

	// 何回か判定するので、同じブロック一覧を使い回す
	const std::vector<BlockBox> blocks = CollectBlocks();

	for (const VECTOR& testPos : candidates)
	{
		const PoseCollisionResult result = TestCollisionAtPose(testPos, candidateAngle, mShape, blocks);

		VECTOR acceptedPos = testPos;
		bool isAccepted = false;

		// (a) そのまま置ける
		if (IsPoseAcceptable(result))
		{
			isAccepted = true;
		}
		// (b) 床キック : 床だけに当たっているなら押し上げて置けるか試す
		else if (CanFloorKick(result))
		{
			acceptedPos = VAdd(testPos, result.correction);
			isAccepted = IsPoseAcceptable(TestCollisionAtPose(acceptedPos, candidateAngle, mShape, blocks));
		}

		if (!isAccepted)
			continue;

		if (wasOnFloor)
			acceptedPos = SnapDownToFloor(acceptedPos, candidateAngle, blocks);

		ApplyRotatedPose(acceptedPos, candidateAngle);
		return true;
	}

	// どの候補もめり込む → 回転キャンセル
	return false;
}

void Player::ApplyRotatedPose(const VECTOR& _pos, float _rotZ)
{
	mpTransform->SetPosition(_pos);
	mpTransform->SetRotation(VGet(0.0f, 0.0f, _rotZ));

	mfInputCooldown = GameConfig::INPUT_TOPPLE_COOLDOWN;	// 直後の倒れ込みで入力が打ち消されないように
	// ※Pieceの同期は、このあと Player::Update の最後でまとめて行う
}

// ============================================================ //
//  床への吸着
// ============================================================ //
VECTOR Player::SnapDownToFloor(const VECTOR& _pos, float _rotZ, const std::vector<BlockBox>& _blocks) const
{
	// _dy だけ下げたときの、一番深いめり込み量
	auto penetrationAt = [&](float _dy) {
		return MaxPenetration(QueryPose(VAdd(_pos, VGet(0.0f, _dy, 0.0f)), _rotZ, mShape, _blocks));
	};

	const float allowedPenetration = penetrationAt(0.0f) + SNAP_DOWN_TOLERANCE;

	// 最大まで下げてもどこにも触れない = 下に床が無い → 下げない(重力に任せる)
	if (penetrationAt(SNAP_DOWN_MAX) <= allowedPenetration)
		return _pos;

	// 床に触れる直前の高さを二分探索で探す
	float okDy = 0.0f;				// ここまでは下げられる
	float ngDy = SNAP_DOWN_MAX;		// ここまで下げるとめり込む
	for (int i = 0; i < SNAP_DOWN_SEARCH_ITERATIONS; ++i)
	{
		const float midDy = (okDy + ngDy) * 0.5f;
		if (penetrationAt(midDy) <= allowedPenetration)
			okDy = midDy;
		else
			ngDy = midDy;
	}

	return VAdd(_pos, VGet(0.0f, okDy, 0.0f));
}

// ============================================================ //
//  回転軸
// ============================================================ //
void Player::RecalculateRotationPivot()
{
	constexpr int N = GameConfig::PLAYER_PIECE_SIZE;

	// -- 1. 有効セルの重心(マス単位)を求める -- //
	float sumColumn = 0.0f;
	float sumRow = 0.0f;
	int activeCount = 0;

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			if (!mShape[c][r])
				continue;

			sumColumn += static_cast<float>(c);
			sumRow += static_cast<float>(r);
			++activeCount;
		}
	}

	if (activeCount == 0)
		return;

	const float centroidColumn = sumColumn / activeCount;
	const float centroidRow = sumRow / activeCount;

	// -- 2. 重心に一番近い有効セルを回転軸に選ぶ -- //
	//    (同じ距離なら3×3の中央に近いほう → 走査順で先のほう)
	int bestColumn = -1;
	int bestRow = -1;
	float bestDistSq = FLT_MAX;
	float bestCenterDistSq = FLT_MAX;

	for (int r = 0; r < N; ++r)
	{
		for (int c = 0; c < N; ++c)
		{
			if (!mShape[c][r])
				continue;

			const float dc = c - centroidColumn;
			const float dr = r - centroidRow;
			const float distSq = dc * dc + dr * dr;

			const float cc = static_cast<float>(c - N / 2);
			const float cr = static_cast<float>(r - N / 2);
			const float centerDistSq = cc * cc + cr * cr;

			constexpr float EPS = 0.0001f;
			const bool isCloser = distSq < bestDistSq - EPS;
			const bool isSameDist = std::fabsf(distSq - bestDistSq) <= EPS;

			if (isCloser || (isSameDist && centerDistSq < bestCenterDistSq))
			{
				bestColumn = c;
				bestRow = r;
				bestDistSq = distSq;
				bestCenterDistSq = centerDistSq;
			}
		}
	}

	// -- 3. 見た目を変えずに、Playerの原点を新しい回転軸セルの位置へ移す -- //
	//    (GetCellWorldPosition は「古い回転軸」基準で計算されるので、軸を変える前に呼ぶ)
	mpTransform->SetPosition(GetCellWorldPosition(bestColumn, bestRow));

	// -- 4. 回転軸を差し替える(各セルの相対座標は GetCellLocalPosition で自動的に変わる) -- //
	mnPivotColumn = bestColumn;
	mnPivotRow = bestRow;
}
