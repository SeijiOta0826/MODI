#pragma once
#include <DxLib.h>

#include "KitchenToolTable.h"	// KitchenToolDesc

/* memo :
* 調理器具の「カタログ」。
* 器具ごとに変わらない情報(画像パス・当たり判定の円・基準の大きさ)をここに1か所だけ書き、
* シーン側は「どの器具を・どこに・どの向きで置くか」だけを書けばよいようにする。
*
* 使い方 :
*   const KitchenToolPlacement placement = { KitchenToolType::Otama, VGet(150.0f, 560.0f, 0.0f), -0.1f, 0.8f };
*   mKitchenTools.AddTool(GetUIManager(), KitchenToolCatalog::MakeDesc(placement));
*
* 画像を差し替えたとき :
*   KitchenToolCatalog.cpp の表(画像パス・基準の拡縮・円)だけを直せば、全シーンに反映される。
*/

enum class KitchenToolType
{
	MeasuringCup,		// 計量カップ
	WoodenSpatula,		// 木べら
	Otama,				// おたま
	MeasuringSpoon,		// 計量スプーン
	FryingPan,			// フライパン
	KitchenKnife,		// 包丁
	CookingChopsticks,	// 菜箸

	Max
};

// シーンごとの配置(位置・向き・大きさの倍率)
struct KitchenToolPlacement
{
	KitchenToolType type;
	VECTOR position;		// スクリーン座標(画像の中心)
	float angle;			// ラジアン(時計回りが正)
	float sizeRate = 1.0f;	// 基準の大きさ(タイトル画面の大きさ)に対する倍率
};

namespace KitchenToolCatalog
{
	// 配置から、KitchenToolTable::AddTool() に渡すデータを作る
	KitchenToolDesc MakeDesc(const KitchenToolPlacement& _placement);
}
