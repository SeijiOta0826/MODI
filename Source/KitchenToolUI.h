#pragma once
#include "UI.h"

#include <vector>

/* memo :
* タイトル画面の机の上に置かれた「調理器具」のUI。
* 見た目は通常のUIと同じ(UIGraphicで描画)だが、簡易的な2D物理の状態を持つ。
*
*   ・当たり判定は「複数の円」の組み合わせで表す(コンパウンド形状)
*     → 包丁や菜箸のような細長い形も、円を並べるだけでそれらしく判定できる
*   ・速度 / 角速度 を持ち、Integrate()で位置と角度を進める
*   ・ドラッグ中は「マウスに追従する重さ無限大の物体」として扱う
*
* 入力の判定や器具どうしの衝突は KitchenToolTable がまとめて行う。
* (このクラスは「1つの器具が自分の状態をどう変えるか」だけを担当する)
*
* 座標について :
*   SetupBody()でアンカーを(0,0)にするので、Transformの座標 = スクリーン座標(画像の中心)になる。
*   角度はTransformのrotation.z(ラジアン, 時計回りが正)をそのまま使う。
*/

// 当たり判定の円(元画像のピクセル単位, 画像中心からの位置)
struct KitchenToolCircle
{
	float x;
	float y;
	float radius;
};

class KitchenToolUI : public UI
{
public:
	KitchenToolUI() = default;
	~KitchenToolUI() = default;

	// 縮小表示してもギザギザにならないよう、バイリニア補間で描画する
	void Draw() override;

	// 画像の読み込み・拡縮の設定をした後に呼ぶ(質量などは拡縮後の大きさで計算する)
	void SetupBody(const std::vector<KitchenToolCircle>& _circles);

	// -- 位置・角度 -- //
	VECTOR GetPosition() { return GetTransform().GetPosition(); }
	void SetPosition(const VECTOR& _position) { GetTransform().SetPosition(_position); }
	void Translate(const VECTOR& _move) { SetPosition(VAdd(GetPosition(), _move)); }
	float GetAngle() { return GetTransform().GetRotation().z; }
	void SetAngle(float _angle) { GetTransform().SetRotation(VGet(0.0f, 0.0f, _angle)); }

	// -- 当たり判定 -- //
	size_t GetCircleCount() const { return mCircles.size(); }
	VECTOR GetCircleWorldCenter(size_t _index);	// 円の中心(スクリーン座標)
	float GetCircleWorldRadius(size_t _index);	// 円の半径(拡縮後)
	bool HitTest(const VECTOR& _point);			// 点がどれかの円に入っているか

	// -- 物理 -- //
	void Integrate(float _deltaTime);									// 速度で位置・角度を進め、摩擦で減速させる
	void ApplyImpulse(const VECTOR& _impulse, const VECTOR& _worldPoint);	// 指定点に力積を加える(並進 + 回転)
	VECTOR GetPointVelocity(const VECTOR& _worldPoint);					// 指定点の速度(回転による速度も含む)

	VECTOR GetVelocity() const { return mvVelocity; }
	void SetVelocity(const VECTOR& _velocity) { mvVelocity = _velocity; }
	float GetAngularVelocity() const { return mfAngularVelocity; }
	void SetAngularVelocity(float _angularVelocity) { mfAngularVelocity = _angularVelocity; }

	// ドラッグ中は0(= 重さ無限大。ぶつかっても押し返されない)
	float GetInverseMass() const { return mbDragged ? 0.0f : mfInverseMass; }
	float GetInverseInertia() const { return mbDragged ? 0.0f : mfInverseInertia; }
	float GetMass() const { return mfMass; }

	// -- ドラッグ -- //
	void BeginDrag(const VECTOR& _mouse);					// 掴んだ点を覚える
	void UpdateDrag(const VECTOR& _mouse, float _deltaTime);	// 掴んだ点がマウスに来るよう移動
	void EndDrag();											// 手を離す(その時の速度で投げる)
	bool IsDragged() const { return mbDragged; }

private:
	float GetScale() { return GetTransform().GetScale().x; }	// 縦横同じ倍率で拡縮する前提
	VECTOR RotateLocal(const VECTOR& _local);					// ローカル座標(拡縮後)を現在の角度で回転

private:
	std::vector<KitchenToolCircle> mCircles;

	VECTOR mvVelocity = VGet(0.0f, 0.0f, 0.0f);	// 速度(px/秒)
	float mfAngularVelocity = 0.0f;				// 角速度(ラジアン/秒)

	float mfMass = 1.0f;
	float mfInverseMass = 1.0f;		// 1 / 質量(計算で割り算を避けるため逆数で持つ)
	float mfInverseInertia = 1.0f;	// 1 / 慣性モーメント(回りにくさの逆数)

	bool mbDragged = false;
	VECTOR mvGrabLocal = VGet(0.0f, 0.0f, 0.0f);	// 掴んだ点(器具の中心から, 回転前)
	VECTOR mvDragVelocity = VGet(0.0f, 0.0f, 0.0f);	// ドラッグ中の速度(平滑化済み)
};
