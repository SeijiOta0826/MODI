#pragma once
#include <DxLib.h>

#include <string>
#include <vector>

#include "KitchenToolUI.h"	// KitchenToolCircle

class UIManager;

// 器具1つ分の配置データ(TitleSceneなどで表として並べる)
struct KitchenToolDesc
{
	std::string filePath;					// 画像のパス
	VECTOR position;						// 初期位置(スクリーン座標, 画像の中心)
	float scale;							// 拡縮(縦横共通)
	float angle;							// 初期角度(ラジアン, 時計回りが正)
	std::vector<KitchenToolCircle> circles;	// 当たり判定(元画像のピクセル単位)
};

/* memo :
* 机の上の調理器具(KitchenToolUI)をまとめて動かすクラス。
* TitleSceneが値として持ち、Update()を毎フレーム呼ぶ。
*
*   1. 入力   … ドラッグ(掴む / 動かす / 離して投げる)と、カーソルが通過したときの小さな押し出し
*   2. 移動   … 各器具の速度で位置を進める
*   3. 衝突   … 器具どうしの重なりを押し戻し、跳ね返りの力積を与える
*   4. 画面端 … 画面の外へ出ないように押し戻して跳ね返す
*
* 描画の前後関係 :
*   mToolsの並び順 = 下から上の順。掴んだ器具は一番上(末尾)へ移動し、レイヤーを振り直す。
*/
class KitchenToolTable
{
public:
	KitchenToolTable() = default;
	~KitchenToolTable() = default;

	// 器具を生成して登録する(UIの所有権はUIManager)
	KitchenToolUI* AddTool(UIManager* _uiManager, const KitchenToolDesc& _desc);

	// 器具に使うレイヤーの開始番号(器具は _baseLayer, _baseLayer+1, ... を使う)
	void SetBaseLayer(int _baseLayer) { mnBaseLayer = _baseLayer; }

	// _canStartInput : 新しく掴む / カーソルで押すのを許可するか(ボタンの上などでは false)
	void Update(float _deltaTime, bool _canStartInput);

	bool IsDragging() const { return mpDragged != nullptr; }

	void Clear();

private:
	void UpdateInput(float _deltaTime, bool _canStartInput);
	void ApplyHoverPush(const VECTOR& _mouse, const VECTOR& _mouseVelocity);
	void ResolveCollisions();
	void ResolvePair(KitchenToolUI* _a, KitchenToolUI* _b);
	void ResolveScreenEdges(KitchenToolUI* _tool);

	KitchenToolUI* FindTopToolAt(const VECTOR& _point);	// 一番上に描かれている器具を探す
	void BringToFront(KitchenToolUI* _tool);
	void ApplyLayers();

private:
	std::vector<KitchenToolUI*> mTools;	// 下 → 上 の順
	KitchenToolUI* mpDragged = nullptr;

	int mnBaseLayer = 10;

	// -- マウスの前回位置(速度の計算用) -- //
	VECTOR mvPreviousMouse = VGet(0.0f, 0.0f, 0.0f);
	bool mbHasPreviousMouse = false;
};
