#pragma once
#include <string>

class UI;
class UIGraphic
{
public:
	UIGraphic() = default;
	~UIGraphic() = default;

	void Initialize(UI* _pUI, std::string _graphicFileName);

	void Draw();

	// 画像の明るさ (0〜255, 255 = 元の色のまま)
	void SetBright(int _bright) { mnBright = _bright; }
	int GetBright() const { return mnBright; }

	// 画像の不透明度 (0〜255, 255 = 不透明)
	void SetAlpha(int _alpha) { mnAlpha = _alpha; }
	int GetAlpha() const { return mnAlpha; }

private:
	int mnGraphHandle = -1;
	int mnBright = 255;
	int mnAlpha = 255;

	UI* mpUI;
};