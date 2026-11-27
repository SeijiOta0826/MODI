#include "KitchenToolUI.h"

#include <cmath>
#include <algorithm>

namespace
{
	constexpr float PI = 3.14159265f;

	// -- 質量 -- //
	constexpr float DENSITY = 1.0f / 10000.0f;	// 円の面積 → 質量 の係数(大きい器具ほど重くなる)

	// -- 摩擦(机の上を滑る減速) -- //
	constexpr float LINEAR_FRICTION = 3.5f;		// 1秒あたりの速度の減り方(大きいほどすぐ止まる)
	constexpr float ANGULAR_FRICTION = 4.5f;	// 1秒あたりの回転の減り方
	constexpr float STOP_SPEED = 2.0f;			// これより遅ければ完全に止める(px/秒)
	constexpr float STOP_ANGULAR_SPEED = 0.02f;	// これより遅ければ回転を止める(rad/秒)

	// -- ドラッグ・投げ -- //
	constexpr float DRAG_VELOCITY_SMOOTH = 0.35f;	// ドラッグ速度の平滑化(マウスのガタつきを抑える)
	constexpr float MAX_THROW_SPEED = 2500.0f;		// 投げたときの最高速度(px/秒)
	constexpr float THROW_SPIN_RATE = 0.35f;		// 端を持って投げたときの回転の付きやすさ
	constexpr float MAX_THROW_SPIN = 8.0f;			// 投げたときの最大角速度(rad/秒)

	// 2Dの外積(z成分)。「a から見て b がどちら回りにあるか」と「回転への効き具合」を表す
	float Cross(const VECTOR& _a, const VECTOR& _b)
	{
		return _a.x * _b.y - _a.y * _b.x;
	}

	// スコープの間だけ描画モードをバイリニアにする(抜けると自動で元に戻る)
	// LoadUIParts.cpp と同じ RAII(生成で設定 / 破棄で後始末)の形
	class ScopedBilinear
	{
	public:
		ScopedBilinear() : mnPrevMode(GetDrawMode()) { SetDrawMode(DX_DRAWMODE_BILINEAR); }
		~ScopedBilinear() { SetDrawMode(mnPrevMode); }
	private:
		int mnPrevMode;
	};
}

void KitchenToolUI::Draw()
{
	ScopedBilinear bilinear;
	UI::Draw();
}

void KitchenToolUI::SetupBody(const std::vector<KitchenToolCircle>& _circles)
{
	mCircles = _circles;

	// アンカーを左上にして「Transformの座標 = スクリーン座標」にする
	GetTransform().SetAnchor(VGet(0.0f, 0.0f, 0.0f));

	// -- 質量と慣性モーメントを円から計算 -- //
	// 慣性モーメント : 回転のしにくさ。中心から遠くに重さがあるほど大きい
	//   円1つ分 = 質量 × (中心からの距離² + 半径² / 2)
	const float scale = GetScale();
	float mass = 0.0f;
	float inertia = 0.0f;
	for (const auto& circle : mCircles)
	{
		const float r = circle.radius * scale;
		const float cx = circle.x * scale;
		const float cy = circle.y * scale;

		const float circleMass = PI * r * r * DENSITY;
		mass += circleMass;
		inertia += circleMass * (cx * cx + cy * cy + r * r * 0.5f);
	}

	mfMass = (std::max)(mass, 0.01f);
	mfInverseMass = 1.0f / mfMass;
	mfInverseInertia = 1.0f / (std::max)(inertia, 0.01f);
}

VECTOR KitchenToolUI::RotateLocal(const VECTOR& _local)
{
	const float angle = GetAngle();
	const float c = std::cos(angle);
	const float s = std::sin(angle);

	// DrawRotaGraph3と同じ向き(画面上で時計回りが正)の回転
	return VGet(
		_local.x * c - _local.y * s,
		_local.x * s + _local.y * c,
		0.0f
	);
}

VECTOR KitchenToolUI::GetCircleWorldCenter(size_t _index)
{
	const auto& circle = mCircles[_index];
	const float scale = GetScale();

	const VECTOR local = VGet(circle.x * scale, circle.y * scale, 0.0f);
	return VAdd(GetPosition(), RotateLocal(local));
}

float KitchenToolUI::GetCircleWorldRadius(size_t _index)
{
	return mCircles[_index].radius * GetScale();
}

bool KitchenToolUI::HitTest(const VECTOR& _point)
{
	if (!IsEnabled())
		return false;

	for (size_t i = 0; i < mCircles.size(); ++i)
	{
		const VECTOR diff = VSub(_point, GetCircleWorldCenter(i));
		const float radius = GetCircleWorldRadius(i);

		// 平方根を取らずに「距離² ≦ 半径²」で比べる(結果は同じで計算が軽い)
		if (VSquareSize(diff) <= radius * radius)
			return true;
	}
	return false;
}

void KitchenToolUI::Integrate(float _deltaTime)
{
	// ドラッグ中はマウスが位置を決めるので何もしない
	if (mbDragged)
		return;

	// -- 速度で位置・角度を進める -- //
	Translate(VScale(mvVelocity, _deltaTime));
	SetAngle(GetAngle() + mfAngularVelocity * _deltaTime);

	// -- 摩擦で減速 -- //
	// exp(-k * dt) を掛けると、フレームレートが変わっても同じ減り方になる
	mvVelocity = VScale(mvVelocity, std::exp(-LINEAR_FRICTION * _deltaTime));
	mfAngularVelocity *= std::exp(-ANGULAR_FRICTION * _deltaTime);

	if (VSize(mvVelocity) < STOP_SPEED)
		mvVelocity = VGet(0.0f, 0.0f, 0.0f);
	if (std::fabs(mfAngularVelocity) < STOP_ANGULAR_SPEED)
		mfAngularVelocity = 0.0f;
}

void KitchenToolUI::ApplyImpulse(const VECTOR& _impulse, const VECTOR& _worldPoint)
{
	// 力積 : 「一瞬でどれだけ押したか」。速度の変化 = 力積 / 質量
	mvVelocity = VAdd(mvVelocity, VScale(_impulse, GetInverseMass()));

	// 中心からずれた場所を押すと回転も生まれる(てこの原理)
	const VECTOR arm = VSub(_worldPoint, GetPosition());
	mfAngularVelocity += Cross(arm, _impulse) * GetInverseInertia();
}

VECTOR KitchenToolUI::GetPointVelocity(const VECTOR& _worldPoint)
{
	// 回転している物体は、中心から離れた点ほど速く動く
	// 点の速度 = 中心の速度 + 角速度 × (中心→点 を90度回したベクトル)
	const VECTOR arm = VSub(_worldPoint, GetPosition());
	return VAdd(
		mvVelocity,
		VGet(-mfAngularVelocity * arm.y, mfAngularVelocity * arm.x, 0.0f)
	);
}

void KitchenToolUI::BeginDrag(const VECTOR& _mouse)
{
	mbDragged = true;

	// 掴んだ点を「回転前のローカル座標」で覚える
	// (回転していても、同じ場所を持ち続けられるようにするため)
	const VECTOR arm = VSub(_mouse, GetPosition());
	const float angle = GetAngle();
	const float c = std::cos(-angle);
	const float s = std::sin(-angle);
	mvGrabLocal = VGet(arm.x * c - arm.y * s, arm.x * s + arm.y * c, 0.0f);

	mvVelocity = VGet(0.0f, 0.0f, 0.0f);
	mvDragVelocity = VGet(0.0f, 0.0f, 0.0f);
	mfAngularVelocity = 0.0f;
}

void KitchenToolUI::UpdateDrag(const VECTOR& _mouse, float _deltaTime)
{
	if (!mbDragged || _deltaTime <= 0.0f)
		return;

	// 掴んだ点がマウスの位置に来るように中心を決める
	const VECTOR previous = GetPosition();
	const VECTOR next = VSub(_mouse, RotateLocal(mvGrabLocal));
	SetPosition(next);

	// このフレームの移動量から速度を求め、少しずつ反映(平滑化)
	const VECTOR frameVelocity = VScale(VSub(next, previous), 1.0f / _deltaTime);
	mvDragVelocity = VAdd(
		mvDragVelocity,
		VScale(VSub(frameVelocity, mvDragVelocity), DRAG_VELOCITY_SMOOTH)
	);

	// 衝突計算では「この速度で動いている物体」として他の器具を押す
	mvVelocity = mvDragVelocity;
	mfAngularVelocity = 0.0f;
}

void KitchenToolUI::EndDrag()
{
	if (!mbDragged)
		return;
	mbDragged = false;

	// -- 投げる速度(速すぎる場合は抑える) -- //
	VECTOR velocity = mvDragVelocity;
	const float speed = VSize(velocity);
	if (speed > MAX_THROW_SPEED)
		velocity = VScale(velocity, MAX_THROW_SPEED / speed);
	mvVelocity = velocity;

	// -- 端を持って投げたら回転させる -- //
	// 掴んだ点が進行方向へ向くように回る(紐で引っぱったときのような動き)
	const VECTOR arm = RotateLocal(mvGrabLocal);
	const float armLengthSq = VSquareSize(arm);
	if (armLengthSq > 1.0f)
	{
		const float spin = Cross(arm, velocity) / armLengthSq * THROW_SPIN_RATE;
		mfAngularVelocity = std::clamp(spin, -MAX_THROW_SPIN, MAX_THROW_SPIN);
	}
}
