#pragma once

#include <string>
#include <vector>
#include "DxLib.h"

class SoundManager
{
public:     // enum, struct などの定義

	// BGM の種類
	enum SOUND_BGM
	{

		BGM_Title = 0,          // タイトルBGM	
		BGM_Game = 1,           // ゲームBGM

	};

	// SE の種類
	enum SOUND_SE
	{
		SE_DECIDE = 0,          // 決定音
		SE_SELECTUI,
		SE_ITEM_GETTING,
		SE_RESPAWN,
		SE_GOAL,
		SE_GOALVOICE,

	};

	static SoundManager* GetInstance();

private:

	static SoundManager* sInstance;

public:
	// コンストラクタ
	SoundManager();
	// デストラクタ
	~SoundManager();

	// 初期化
	void Initialize();
	// 終了処理
	void Finalize();

public:     // サウンド再生系

	// BGM再生
	// SOUND_BGM ... 鳴らしたいBGMの種類
	// isTop ... 最初から再生するかどうか
	// bool isTop = true -> デフォルト引数と呼ばれる書き方。関数を呼び出す際に渡されなくても認識される
	void PlayBGM(SOUND_BGM bgm, bool isTop = true);

	// SE再生
	// se        ... 鳴らしたいSEの種類
	// pitchRate ... 音程の倍率
	void PlaySE(SOUND_SE se, float pitchRate = 1.0f);

	// BGMの読み込み
	void LoadBGM(SOUND_BGM bgm, std::string filename);

	// SEの読み込み
	void LoadSE(SOUND_SE se, std::string silename);

	// BGMの停止
	void StopBGM();

	// SEが再生中かの判定
	bool IsPlayingSE(SOUND_SE se);
	bool IsPlayingBGM(SOUND_BGM bgm);

public:     // 音量調節(ConfigUIから使う)

	static constexpr int VOLUME_LEVEL_MAX = 10;	// 音量の段階数(0 = 無音 ～ 10 = 最大)

	// 音量を段階で設定する(範囲外の値は 0～VOLUME_LEVEL_MAX に収める)
	void SetBGMVolume(int level);
	void SetSEVolume(int level);

	int GetBGMVolume() const { return mnBgmVolumeLevel; }
	int GetSEVolume() const { return mnSeVolumeLevel; }

private:
	static int ClampVolumeLevel(int level);
	static int ToDxVolume(int level);		// 段階 → DxLibの音量(0～255)
	void ApplyBGMVolume();					// 読み込み済みの全BGMに音量を反映
	void ApplySEVolume();					// 読み込み済みの全SEに音量を反映

private:     // メンバ変数
	SOUND_BGM mnNowPlayingBgm;   // 現在再生されているBGMの種類
	SOUND_SE mnNowPlayingSe;     // 現在再生されているSEの種類

	std::vector<std::pair<SOUND_BGM, int>> mnBgmHandleList;   // 読みこんだBGMハンドルのリスト
	std::vector<std::pair<SOUND_SE, int>> mnSeHandleList;     // 読み込んだSEハンドルのリスト

	int mGameBGMHandle;

	bool mIsPaused = false;

	int mnBgmVolumeLevel = VOLUME_LEVEL_MAX;	// BGMの音量(段階)
	int mnSeVolumeLevel = VOLUME_LEVEL_MAX;		// SEの音量(段階)
};