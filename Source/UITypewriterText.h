#pragma once
#include <string>

class UI;

/* memo :
* 各UIにコンポジットする「文字送り」の要素。
* Initialize()を呼んだUIは、UITextの描画が「全文表示」から「文字送り表示」に切り替わる。
* (呼ばなければ今まで通り全文表示。UIGraphicと同じオプトイン方式)
*
* // -- 使い方 -- //
*	1. UIのInit()内で GetTypewriter().Initialize(this) を呼ぶ
*	2. GetText().SetText() で全文を設定したあと Start() で先頭から文字送り開始
*	3. 更新はUI::Update()内で自動で行われる(フレーム単位)
*
* // -- 注意 -- //
*	表示する文字列そのものはUITextが持つ(このクラスは「何バイト目まで見せるか」だけを管理)
*	文字コードはShift-JIS(マルチバイト文字セット)前提
*/
class UITypewriterText
{
public:
	UITypewriterText() = default;
	~UITypewriterText() = default;

	void Initialize(UI* _pUI);	// 呼ぶと文字送りが有効になる
	void Update();

	void Start();	// 先頭から文字送りをやり直す
	void Skip();	// 全文表示へスキップ（キー入力時用）

	std::string GetVisibleText() const;	// 描画する分の文字列を返す

	// -- 有効状態のアクセサ -- //
	void SetActive(bool _active) { mbActive = _active; }
	bool IsActive() const { return mbActive && mpUI != nullptr; }

	void SetFrameInterval(int _interval) { mnFrameInterval = _interval; }
	bool IsFinished() const { return mbIsFinished; }

private:
	static constexpr size_t MULTI_BYTE_SIZE = 2;	// 全角文字(Shift-JIS)のバイト数

	UI* mpUI = nullptr;			// 付与先のUI

	bool mbActive = false;			// 既定は無効(Initialize()で有効になる)
	size_t mnVisibleByteCount = 0;	// 表示対象のバイト数
	int mnFrameInterval = 4;		// 次の文字が出るまでの待機フレーム
	int mnFrameTimer = 0;			// タイマー
	bool mbIsFinished = false;		// 文字送りが終わったかどうか
};
