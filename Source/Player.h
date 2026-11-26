#pragma once
#include "GameObject.h"
#include "PlayerController.h"
#include "Box2D.h"
#include "GameConfig.h"

#include <array>
#include <vector>

class PlayerPiece;
class Transform;
class Gravity;

// -- 形状マスク : [column][row] の3×3。true = ブロックあり -- //
using PlayerShapeMask = std::array<
	std::array<bool, GameConfig::PLAYER_PIECE_SIZE>,
	GameConfig::PLAYER_PIECE_SIZE>;

// -- 形状変更の結果 -- //
enum class ShapeChangeResult
{
	Success,		// 変形成功
	BaseRemoved,	// 基盤ブロックが外されている
	Disconnected,	// ブロックがひとつの塊になっていない
	Overlapping,	// 追加したブロックが地形にめり込む
};

// -- 当たり判定の相手(ステージブロック)と、その箱 -- //
struct BlockBox {
	GameObject* mpObject = nullptr;
	Box2D mBox{};
};

// -- 外側のフチ(隣にセルが無い辺)。エフェクトを外向きに出すときに使う -- //
struct OuterEdge {
	VECTOR mvPosition = VGet(0.0f, 0.0f, 0.0f);	// フチの中点(ワールド)
	float mfOutwardAngle = 0.0f;					// 外向きの角度(ラジアン)
};

// -- 1つの接触 : どのセルが・何に・どう当たったか -- //
struct BodyContact {
	VECTOR mvCellWorldPos = VGet(0.0f, 0.0f, 0.0f);	// 当たったセルの中心(ワールド)
	// 床を支えている範囲(セルの一番低いところの左端・右端)。接地プローブの床接触でだけ設定
	//   平らに置かれている : 下の辺の左端〜右端 / 傾いている : 角の1点(左端 = 右端)
	VECTOR mvSupportLeft = VGet(0.0f, 0.0f, 0.0f);
	VECTOR mvSupportRight = VGet(0.0f, 0.0f, 0.0f);
	GameObject* mpOther = nullptr;						// 当たった相手(ステージブロック)
	VECTOR mvCorrection = VGet(0.0f, 0.0f, 0.0f);		// 押し出しに必要な移動量
	float mfPenetration = 0.0f;							// めり込み量
	bool mbIsFloor = false;		// 下から支えられている
	bool mbIsWall = false;		// 横から当たっている
	bool mbIsCeiling = false;	// 上から当たっている
};

/* memo :
* プレイヤー本体。形状(3×3マスク)と回転軸を自分で持ち、
* 地形との当たり判定・押し戻し・接地をすべて自分で行う(キネマティック方式)。
* ※キネマティック : 物理演算に任せず、自分で位置を決めて自分で衝突を直す方式
*
* PlayerPiece は「描画」と「他オブジェクト(StageBlockなど)に検知されるためのセンサー」だけを担当する。
* Player の衝突処理は PlayerPiece に依存しない。
*
* Player の座標 = 回転軸セル の座標。各セルは「回転軸セルからの相対座標」で表す。
*
* ■ 処理の置き場所
*   Player.cpp          : 生成 / 毎フレームの流れ / セル座標・重心・外側のフチ / Pieceの同期 / リスポーン
*   PlayerCollision.cpp : 姿勢クエリ(QueryPose) / 床・壁・天井の分類 / 外部向けの判定API
*   PlayerGround.cpp    : 押し戻し(反復解決) / 接地プローブ / 倒れ込み
*   PlayerRotation.cpp  : 回転(重心まわり・横ずらし・床キック・床への吸着) / 回転軸の選び直し
*   PlayerShape.cpp     : 形状の設定 / 変形UIからの形状変更
*
* ■ 当たり判定はすべて QueryPose() の1本
*   「この位置・角度・形状で置いたら、地形とどう当たるか」を返す関数。
*   回転テスト・形状変更テスト・毎フレームの押し戻し・接地判定・外部ギミック(CanMoveBy)が
*   全部これを使うので、判定の食い違い(回転できたのに押し戻しでガタつく等)が起きない。
*   ステージブロックの一覧(CollectBlocks)は、1回の処理の中で1回だけ集めて使い回す。
*
* ■ 1フレームの流れ
*   1. Update()                 : 入力(回転) → 重力で落下 → Pieceの同期
*   2. EndCollisionResolution() : 押し戻し(QueryPoseを数回繰り返す)
*                                 → 接地プローブ(少し下で QueryPose) → 接地 / 倒れ込み → Pieceの同期
*/
class Player : public GameObject
{
public:
	Player(const VECTOR& initPos);

	// ================= 基本 (Player.cpp) ================= //
	void Init() override;
	void InitComponent() override;
	void Update(float _deltaTime) override;

	// リスポーンの理由(難易度Normalで、アイテムをリセットするかの判定に使う)
	enum class RespawnReason
	{
		Restart,	// リスタートキー長押しでのやり直し
		Miss,		// ミキサー / 奈落への落下
	};
	void Respawn(RespawnReason _reason = RespawnReason::Restart);

	// 前回呼んでから「ミス」でリスポーンしたか(呼ぶとフラグは消える)
	bool ConsumeMissed();
	void UpdatePieceTransforms();	// 全Pieceのワールド座標を Player の姿勢に合わせる(外部から動かしたときも呼ぶ)

	// ================= 回転 (PlayerRotation.cpp) ================= //
	// めり込まない姿勢が見つかれば回転して true。見つからなければ何もせず false
	bool TryRotateWithShift(float _rotateDirection);

	// ================= 衝突・接地 (PlayerGround.cpp) ================= //
	void EndCollisionResolution() override;	// 押し戻し → 接地/倒れ込み
	bool IsGrounded() const { return mbIsGrounded; }

	// ================= 外部ギミック向けの判定 (PlayerCollision.cpp) ================= //
	bool IsStandingOn(const GameObject* _object) const;	// _object の上に乗っているか
	bool CanMoveBy(const VECTOR& _offset) const;		// 今の姿勢のまま _offset だけ動かしても地形にめり込まないか

	// ================= 形状 (PlayerShape.cpp) ================= //
	void SetShape(int _shapeType);	// プリセット形状(0:ぼっち 1:T字 2:十字 3:3×3 4:2×2)

	// 変形UIから呼ぶ
	void BeginShapeEdit();		// 基盤ブロックを決定し、Playerを停止する
	void EndShapeEdit();		// 停止を解除する
	bool IsShapeEditing() const { return mbIsShapeEditing; }

	// 画面の見た目の向き(回転を打ち消した向き)での形状・基盤マス
	PlayerShapeMask GetScreenShape() const;
	void GetScreenBaseCell(int& _column, int& _row) const;
	ShapeChangeResult TryChangeShapeFromScreen(const PlayerShapeMask& _screenShape);

	// 画面基準の選択状態を渡すと、今クリックで切り替えてよいマスを返す(true = 切り替え可)
	PlayerShapeMask GetScreenToggleableMask(const PlayerShapeMask& _screenShape) const;
	// 置くとステージにめり込むマス(画面基準 / true = めり込む)
	PlayerShapeMask GetScreenTerrainBlockedMask() const;

private:
	// ----------------- 基本 (Player.cpp) ----------------- //
	void CreatePieces();
	void CheckRespawn();

	VECTOR GetCellLocalPosition(int _column, int _row) const;	// 回転軸セルからの相対座標
	VECTOR GetCellWorldPosition(int _column, int _row) const;	// 今の姿勢でのワールド座標
	VECTOR CalculateCenterOfMass() const;						// 有効セルの重心(ワールド座標)
	std::vector<OuterEdge> CollectOuterEdges() const;			// 全セルの外側のフチ(着地・変形エフェクト用)

	// ----------------- 当たり判定 (PlayerCollision.cpp) ----------------- //
	std::vector<BlockBox> CollectBlocks() const;	// 今のステージブロックの一覧

	// 指定した姿勢で置いたときの接触を全部集める
	std::vector<BodyContact> QueryPose(const VECTOR& _pos, float _rotZ, const PlayerShapeMask& _shape,
		const std::vector<BlockBox>& _blocks) const;

	// 接触リストを「回転・変形してよいか」の判断用にまとめたもの
	struct PoseCollisionResult {
		bool isColliding = false;
		float maxPenetration = 0.0f;					// 一番深いめり込み量
		VECTOR correction = VGet(0.0f, 0.0f, 0.0f);		// 押し出しに必要な移動量
		bool hasFloorCollision = false;
		bool hasWallCollision = false;
		bool hasCeilingCollision = false;
	};
	PoseCollisionResult TestCollisionAtPose(const VECTOR& _pos, float _rotZ, const PlayerShapeMask& _shape,
		const std::vector<BlockBox>& _blocks) const;

	static float MaxPenetration(const std::vector<BodyContact>& _contacts);	// 接触の中で一番深いめり込み量
	static bool IsPoseAcceptable(const PoseCollisionResult& _result);	// 許容範囲のめり込みか
	static bool CanFloorKick(const PoseCollisionResult& _result);		// 床だけに当たっているか

	// ----------------- 回転 (PlayerRotation.cpp) ----------------- //
	void ApplyRotatedPose(const VECTOR& _pos, float _rotZ);	// 回転後の姿勢を確定
	// 回したことで床から浮いた分だけ下げた位置(床に触れるまで)。下に床が無ければ _pos のまま
	VECTOR SnapDownToFloor(const VECTOR& _pos, float _rotZ, const std::vector<BlockBox>& _blocks) const;
	void RecalculateRotationPivot();	// 有効セルのいずれか一つを回転軸(=Playerの原点)にする

	// ----------------- 押し戻し・接地 (PlayerGround.cpp) ----------------- //
	void ResolvePenetration(const std::vector<BlockBox>& _blocks);		// めり込みを数回に分けて解消する
	void UpdateGroundContacts(const std::vector<BlockBox>& _blocks);	// 接地プローブで床接触を集める

	void ProcessGroundAndTopple(const std::vector<BlockBox>& _blocks);
	bool IsStableOnGround() const;
	void SnapRotationIfNearlyFlat();	// 90度単位からの微小なズレを消す
	bool Topple(const std::vector<BlockBox>& _blocks);	// 支点まわりに少し倒す。ふさがっていて倒れられなければ false
	void SetGroundedState(bool _grounded);	// 接地フラグ + 重力の接地状態

	// ----------------- 形状 (PlayerShape.cpp) ----------------- //
	void ApplyLocalShape(const PlayerShapeMask& _shape);
	int GetQuarterTurns() const;			// 現在の回転が90度の何回分か(0〜3)
	void SelectBaseCell();					// 基盤ブロックの決定
	static bool IsConnected(const PlayerShapeMask& _shape, int _startColumn, int _startRow);
	void CacheTerrainBlockedCells();	// 置くと地形にめり込むマスを記録(変形開始時に1回)
	bool CanToggleLocalCell(const PlayerShapeMask& _shape, int _column, int _row) const;

private:
	PlayerController mController;

	// -- よく使うModule(InitComponentで追加したものを覚えておく) -- //
	Transform* mpTransform = nullptr;
	Gravity* mpGravity = nullptr;

	// -- 形状(Player基準の向き)。当たり判定・重心はすべてこれを元に計算する -- //
	PlayerShapeMask mShape{};

	// -- 見た目・センサー用のPiece。mShape に合わせて有効/無効を切り替えるだけ -- //
	PlayerPiece* mPieces[GameConfig::PLAYER_PIECE_SIZE][GameConfig::PLAYER_PIECE_SIZE]{};	// [column][row]

	// -- 接地プローブで見つけた床接触(毎フレーム更新) -- //
	std::vector<BodyContact> mGroundContacts;

	VECTOR mvRespawnPos = VGet(0.0f, 0.0f, 0.0f);	// リスポーン座標
	bool mbMissed = false;							// ミスでリスポーンしたか(GameSceneが ConsumeMissed() で受け取る)

	// -- 接地・倒れ込み -- //
	bool mbIsGrounded = false;		// 安定して接地しているか
	float mfInputCooldown = 0.0f;	// 回転入力の直後、倒れ込みを止めておく残り時間

	// -- 形状変更 -- //
	bool mbIsShapeEditing = false;	// 変形UIを開いているか(開いている間は停止)
	int mnBaseColumn = 1;			// 基盤ブロックのマス(Player基準)
	int mnBaseRow = 1;
	PlayerShapeMask mTerrainBlockedCells{};	// 置くと地形にめり込むマス(Player基準)

	// -- 回転軸セル(Player基準のマス) -- //
	int mnPivotColumn = 1;
	int mnPivotRow = 1;
};
