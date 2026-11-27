#include "KitchenToolCatalog.h"

#include <array>
#include <string>
#include <vector>

namespace
{
	const std::string KITCHEN_TOOL_DIR = "Resource/UI/Title/kitchenTools/";

	// 器具ごとに変わらない情報
	struct KitchenToolShape
	{
		std::string fileName;					// kitchenTools フォルダ内のファイル名
		float baseScale;						// 基準の拡縮(タイトル画面での大きさ)
		std::vector<KitchenToolCircle> circles;	// 当たり判定(元画像のピクセル単位, 画像中心から)
	};

	// KitchenToolType と同じ順番で並べる
	const std::array<KitchenToolShape, static_cast<size_t>(KitchenToolType::Max)> SHAPES =
	{{
		// MeasuringCup
		{ "Measuring Cup.png", 0.90f,
			{ { -55.0f, -5.0f, 185.0f }, { 150.0f, 40.0f, 60.0f }, { 172.0f, 140.0f, 50.0f } } },

		// WoodenSpatula
		{ "Wooden Spatula.png", 1.40f,
			{ { 90.0f, -100.0f, 55.0f }, { 35.0f, -40.0f, 40.0f }, { -20.0f, 20.0f, 28.0f }, { -70.0f, 70.0f, 28.0f }, { -125.0f, 122.0f, 28.0f } } },

		// Otama
		{ "Otama.png", 1.20f,
			{ { -98.0f, 156.0f, 70.0f }, { -25.0f, 95.0f, 26.0f }, { 20.0f, 10.0f, 26.0f }, { 65.0f, -75.0f, 26.0f }, { 110.0f, -160.0f, 26.0f }, { 135.0f, -205.0f, 26.0f } } },

		// MeasuringSpoon
		{ "Measuring spoon.png", 0.85f,
			{ { -180.0f, -80.0f, 45.0f }, { -60.0f, -60.0f, 50.0f }, { -130.0f, 30.0f, 60.0f }, { 0.0f, 140.0f, 95.0f }, { 80.0f, -80.0f, 40.0f }, { 150.0f, -160.0f, 65.0f } } },

		// FryingPan
		{ "flyingpan.png", 1.10f,
			{ { 27.0f, 101.0f, 126.0f }, { -40.0f, -20.0f, 22.0f }, { -65.0f, -65.0f, 24.0f }, { -95.0f, -115.0f, 24.0f }, { -125.0f, -165.0f, 24.0f }, { -150.0f, -205.0f, 24.0f } } },

		// KitchenKnife(高解像度の画像なので、拡縮を小さくして表示サイズを合わせている)
		{ "Kitchen Knife.png", 0.23f,
			{ { 949.0f, 369.0f, 176.0f }, { 549.0f, 251.0f, 176.0f }, { 203.0f, 11.0f, 207.0f }, { -183.0f, -136.0f, 203.0f }, { -572.0f, -279.0f, 178.0f }, { -954.0f, -438.0f, 176.0f } } },

		// CookingChopsticks(同上)
		{ "Cooking chopsticks.png", 0.28f,
			{ { 290.0f, -837.0f, 129.0f }, { 193.0f, -550.0f, 129.0f }, { 97.0f, -263.0f, 129.0f }, { 0.0f, 24.0f, 129.0f }, { -96.0f, 311.0f, 129.0f }, { -230.0f, 770.0f, 210.0f } } },
	}};
}

KitchenToolDesc KitchenToolCatalog::MakeDesc(const KitchenToolPlacement& _placement)
{
	const KitchenToolShape& shape = SHAPES[static_cast<size_t>(_placement.type)];

	KitchenToolDesc desc;
	desc.filePath = KITCHEN_TOOL_DIR + shape.fileName;
	desc.position = _placement.position;
	desc.scale = shape.baseScale * _placement.sizeRate;
	desc.angle = _placement.angle;
	desc.circles = shape.circles;	// 円は元画像の単位なので、拡縮が変わってもそのまま使える
	return desc;
}
