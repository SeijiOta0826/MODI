#pragma once
#include "Module.h"

#include <DxLib.h>	// VECTOR用
#include <string>	// ファイル名用

class Graphic : public Module
{
public:
	Graphic(std::string _filename);
	~Graphic() = default;

	void Initialize() override;

	void Draw() override;

	// スプライトシートの切り出し範囲を設定する
	void SetSourceRect(int _x, int _y, int _width, int _height);

private:
	int mnGraphHandle = -1;
	VECTOR mvSize = VGet(0.0f, 0.0f, 0.0f);

	bool mbUseSourceRect = false;
	int mnSrcX = 0;
	int mnSrcY = 0;
	int mnSrcWidth = 0;
	int mnSrcHeight = 0;
};