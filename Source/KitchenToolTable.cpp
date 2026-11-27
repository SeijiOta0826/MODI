#include "KitchenToolTable.h"

#include "UIManager.h"
#include "InputManager.h"
#include "ScreenConfig.h"

#include <algorithm>
#include <cmath>

namespace
{
	// -- 衝突 -- //
	constexpr int COLLISION_ITERATIONS = 3;			// 1フレームに衝突判定を繰り返す回数(多いほど重なりにくい)
	constexpr float RESTITUTION = 0.35f;			// 器具どうしの反発係数(0 = 跳ねない / 1 = 完全に跳ねる)
	constexpr float POSITION_CORRECTION = 0.8f;		// 重なりを1回で押し戻す割合
	constexpr float PENETRATION_SLOP = 0.5f;		// これ以下のめり込みは許す(押し戻しのガタつき防止)

	// -- 画面端 -- //
	constexpr float WALL_RESTITUTION = 0.4f;		// 画面端での反発係数
	constexpr float WALL_SPIN_DAMPING = 0.7f;		// 画面端に当たったときの回転の残り具合
	constexpr float EDGE_OVERHANG_RATE = 0.6f;		// 当たり判定の円が画面外へはみ出してよい割合(旧背景のように端で器具が切れて見える)

	// -- カーソル通過時の押し出し -- //
	constexpr float HOVER_MIN_MOUSE_SPEED = 60.0f;	// これより遅いカーソルの動きでは押さない(px/秒)
	constexpr float HOVER_PUSH_RATE = 0.02f;		// カーソル速度の何割を器具の速度に足すか
	constexpr float HOVER_MAX_DELTA = 25.0f;		// 1回で足す速度の上限(px/秒)
	constexpr float HOVER_MAX_TOOL_SPEED = 120.0f;	// 器具がこれより速いときは押さない(押し続けて飛んでいかないように)
	constexpr float HOVER_SPIN_RATE = 0.5f;			// 押す点を中心へ寄せる割合(1 = カーソル位置 / 0 = 中心で回転なし)

	float Cross(const VECTOR& _a, const VECTOR& _b)
	{
		return _a.x * _b.y - _a.y * _b.x;
	}
}

KitchenToolUI* KitchenToolTable::AddTool(UIManager* _uiManager, const KitchenToolDesc& _desc)
{
	auto tool = _uiManager->CreateUI<KitchenToolUI>();
	tool->LoadUIGraph(_desc.filePath);
	tool->SetEnabled(true);
	tool->GetTransform().SetScale(VGet(_desc.scale, _desc.scale, 0.0f));

	// 拡縮を決めてから物理の設定(質量が拡縮後の大きさで計算されるため)
	tool->SetupBody(_desc.circles);
	tool->SetPosition(_desc.position);
	tool->SetAngle(_desc.angle);

	mTools.push_back(tool);
	ApplyLayers();

	return tool;
}

void KitchenToolTable::Update(float _deltaTime, bool _canStartInput)
{
	if (_deltaTime <= 0.0f)
		return;

	// 1. 入力
	UpdateInput(_deltaTime, _canStartInput);

	// 2. 移動
	for (auto tool : mTools)
		tool->Integrate(_deltaTime);

	// 3. 衝突(何度か繰り返すと、3つ以上が重なったときも落ち着きやすい)
	for (int i = 0; i < COLLISION_ITERATIONS; ++i)
		ResolveCollisions();

	// 4. 画面端
	for (auto tool : mTools)
		ResolveScreenEdges(tool);
}

void KitchenToolTable::Clear()
{
	// UIの解放はUIManagerが行うので、ここでは参照を手放すだけ
	mTools.clear();
	mpDragged = nullptr;
	mbHasPreviousMouse = false;
}

void KitchenToolTable::UpdateInput(float _deltaTime, bool _canStartInput)
{
	const Mouse& mouse = InputManager::GetInstance().GetMouse();
	const VECTOR mousePosition = VGet(
		static_cast<float>(mouse.GetX()),
		static_cast<float>(mouse.GetY()),
		0.0f
	);

	// -- マウスの速度(前回位置との差 / 経過時間) -- //
	VECTOR mouseVelocity = VGet(0.0f, 0.0f, 0.0f);
	if (mbHasPreviousMouse)
		mouseVelocity = VScale(VSub(mousePosition, mvPreviousMouse), 1.0f / _deltaTime);
	mvPreviousMouse = mousePosition;
	mbHasPreviousMouse = true;

	const bool isPressing = mouse.IsPress(MOUSE_INPUT_LEFT);

	// -- ドラッグ中 : 押している間は追従、離したら投げる -- //
	if (mpDragged != nullptr)
	{
		if (isPressing)
		{
			mpDragged->UpdateDrag(mousePosition, _deltaTime);
		}
		else
		{
			mpDragged->EndDrag();
			mpDragged = nullptr;
		}
		return;
	}

	if (!_canStartInput)
		return;

	// -- 押した瞬間 : カーソルの下の器具を掴む -- //
	if (mouse.IsDown(MOUSE_INPUT_LEFT))
	{
		if (auto tool = FindTopToolAt(mousePosition))
		{
			tool->BeginDrag(mousePosition);
			mpDragged = tool;
			BringToFront(tool);
			return;
		}
	}

	// -- 何も押していない : カーソルが通過した器具を少しだけ押す -- //
	if (!isPressing)
		ApplyHoverPush(mousePosition, mouseVelocity);
}

void KitchenToolTable::ApplyHoverPush(const VECTOR& _mouse, const VECTOR& _mouseVelocity)
{
	const float mouseSpeed = VSize(_mouseVelocity);
	if (mouseSpeed < HOVER_MIN_MOUSE_SPEED)
		return;

	KitchenToolUI* tool = FindTopToolAt(_mouse);
	if (tool == nullptr)
		return;

	if (VSize(tool->GetVelocity()) > HOVER_MAX_TOOL_SPEED)
		return;

	// 足したい速度(カーソルの動きの数%, 上限あり)
	VECTOR deltaVelocity = VScale(_mouseVelocity, HOVER_PUSH_RATE);
	const float deltaSpeed = VSize(deltaVelocity);
	if (deltaSpeed > HOVER_MAX_DELTA)
		deltaVelocity = VScale(deltaVelocity, HOVER_MAX_DELTA / deltaSpeed);

	// 速度の変化 × 質量 = 力積。中心寄りの点を押して、回転は控えめにする
	const VECTOR center = tool->GetPosition();
	const VECTOR pushPoint = VAdd(center, VScale(VSub(_mouse, center), HOVER_SPIN_RATE));
	tool->ApplyImpulse(VScale(deltaVelocity, tool->GetMass()), pushPoint);
}

void KitchenToolTable::ResolveCollisions()
{
	// 全ての組み合わせ(i < j)を1回ずつ調べる
	for (size_t i = 0; i < mTools.size(); ++i)
	{
		for (size_t j = i + 1; j < mTools.size(); ++j)
		{
			ResolvePair(mTools[i], mTools[j]);
		}
	}
}

void KitchenToolTable::ResolvePair(KitchenToolUI* _a, KitchenToolUI* _b)
{
	const float inverseMassA = _a->GetInverseMass();
	const float inverseMassB = _b->GetInverseMass();
	const float inverseMassSum = inverseMassA + inverseMassB;
	if (inverseMassSum <= 0.0f)
		return;	// どちらも動かせない

	for (size_t ia = 0; ia < _a->GetCircleCount(); ++ia)
	{
		for (size_t ib = 0; ib < _b->GetCircleCount(); ++ib)
		{
			// -- 円どうしの重なりを調べる -- //
			const VECTOR centerA = _a->GetCircleWorldCenter(ia);
			const VECTOR centerB = _b->GetCircleWorldCenter(ib);
			const float radiusA = _a->GetCircleWorldRadius(ia);
			const float radiusB = _b->GetCircleWorldRadius(ib);

			const VECTOR diff = VSub(centerB, centerA);
			const float distance = VSize(diff);
			const float penetration = radiusA + radiusB - distance;	// めり込み量
			if (penetration <= 0.0f)
				continue;

			// 法線 : A → B の向き(押し合う方向)
			const VECTOR normal = (distance > 0.0001f)
				? VScale(diff, 1.0f / distance)
				: VGet(1.0f, 0.0f, 0.0f);

			// 接触点 : 2つの円の境目あたり
			const VECTOR contact = VAdd(centerA, VScale(normal, radiusA - penetration * 0.5f));

			// -- ① 位置の補正 : 軽い方ほど大きく押し戻す -- //
			const float correction = (std::max)(penetration - PENETRATION_SLOP, 0.0f)
				/ inverseMassSum * POSITION_CORRECTION;
			_a->Translate(VScale(normal, -correction * inverseMassA));
			_b->Translate(VScale(normal, correction * inverseMassB));

			// -- ② 速度の補正 : ぶつかり合っているときだけ跳ね返す -- //
			const VECTOR relativeVelocity = VSub(
				_b->GetPointVelocity(contact),
				_a->GetPointVelocity(contact)
			);
			const float normalSpeed = VDot(relativeVelocity, normal);
			if (normalSpeed > 0.0f)
				continue;	// すでに離れる方向に動いている

			const VECTOR armA = VSub(contact, _a->GetPosition());
			const VECTOR armB = VSub(contact, _b->GetPosition());
			const float armCrossA = Cross(armA, normal);
			const float armCrossB = Cross(armB, normal);

			// 「押しにくさ」の合計(質量と、回転のしにくさの両方を含む)
			const float denominator = inverseMassSum
				+ armCrossA * armCrossA * _a->GetInverseInertia()
				+ armCrossB * armCrossB * _b->GetInverseInertia();

			const float impulseSize = -(1.0f + RESTITUTION) * normalSpeed / denominator;
			const VECTOR impulse = VScale(normal, impulseSize);

			_a->ApplyImpulse(VScale(impulse, -1.0f), contact);
			_b->ApplyImpulse(impulse, contact);
		}
	}
}

void KitchenToolTable::ResolveScreenEdges(KitchenToolUI* _tool)
{
	// ドラッグ中はマウスの位置を優先する
	if (_tool->IsDragged())
		return;

	for (size_t i = 0; i < _tool->GetCircleCount(); ++i)
	{
		const VECTOR center = _tool->GetCircleWorldCenter(i);
		// 半径を縮めて判定する = その分だけ画面の外へはみ出せる
		const float radius = _tool->GetCircleWorldRadius(i) * (1.0f - EDGE_OVERHANG_RATE);

		VECTOR push = VGet(0.0f, 0.0f, 0.0f);
		if (center.x - radius < 0.0f)								push.x = radius - center.x;
		if (center.x + radius > ScreenConfig::SCREEN_WIDTH)			push.x = ScreenConfig::SCREEN_WIDTH - (center.x + radius);
		if (center.y - radius < 0.0f)								push.y = radius - center.y;
		if (center.y + radius > ScreenConfig::SCREEN_HEIGHT)		push.y = ScreenConfig::SCREEN_HEIGHT - (center.y + radius);

		if (push.x == 0.0f && push.y == 0.0f)
			continue;

		_tool->Translate(push);

		// 押し戻した方向へ向かう速度だけ反転させる
		VECTOR velocity = _tool->GetVelocity();
		if (push.x > 0.0f && velocity.x < 0.0f)	velocity.x = -velocity.x * WALL_RESTITUTION;
		if (push.x < 0.0f && velocity.x > 0.0f)	velocity.x = -velocity.x * WALL_RESTITUTION;
		if (push.y > 0.0f && velocity.y < 0.0f)	velocity.y = -velocity.y * WALL_RESTITUTION;
		if (push.y < 0.0f && velocity.y > 0.0f)	velocity.y = -velocity.y * WALL_RESTITUTION;
		_tool->SetVelocity(velocity);
		_tool->SetAngularVelocity(_tool->GetAngularVelocity() * WALL_SPIN_DAMPING);
	}
}

KitchenToolUI* KitchenToolTable::FindTopToolAt(const VECTOR& _point)
{
	// 末尾ほど上に描かれているので、後ろから調べる
	for (auto it = mTools.rbegin(); it != mTools.rend(); ++it)
	{
		if ((*it)->HitTest(_point))
			return *it;
	}
	return nullptr;
}

void KitchenToolTable::BringToFront(KitchenToolUI* _tool)
{
	auto it = std::find(mTools.begin(), mTools.end(), _tool);
	if (it == mTools.end())
		return;

	mTools.erase(it);
	mTools.push_back(_tool);
	ApplyLayers();
}

void KitchenToolTable::ApplyLayers()
{
	// 並び順どおりに、重ならないレイヤー番号を振る
	// (同じレイヤーだと描画順が毎フレーム入れ替わり、チラつく恐れがあるため)
	for (size_t i = 0; i < mTools.size(); ++i)
		mTools[i]->SetLayer(mnBaseLayer + static_cast<int>(i));
}
