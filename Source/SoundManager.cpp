#include "SoundManager.h"
#include "DxLib.h"

SoundManager* SoundManager::sInstance = nullptr;

SoundManager* SoundManager::GetInstance()
{
	if (sInstance == nullptr)
	{
		sInstance = new SoundManager();
		sInstance->Initialize();
	}
	return sInstance;
}


SoundManager::SoundManager()
	: mnNowPlayingBgm((SOUND_BGM)-1)   // 初期状態は何も再生されない状態
	, mnNowPlayingSe((SOUND_SE)-1)     // 初期状態は何も再生されない状態
	, mIsPaused(false)
{

}

SoundManager::~SoundManager()
{

}

void SoundManager::Initialize()
{
	// BGM の読み込み
	//LoadBGM(SOUND_BGM::
	

	// SE の読み込み
	LoadSE(SOUND_SE::SE_DECIDE, "Resource/Sound/SE/Click.mp3");		// クリック(通常)
	LoadSE(SOUND_SE::SE_SELECTUI, "Resource/Sound/SE/ClickUI.mp3");			// クリック(UI)
	LoadSE(SOUND_SE::SE_ITEM_GETTING, "Resource/Sound/SE/ItemGet.mp3");			// クリック(UI)
	LoadSE(SOUND_SE::SE_RESPAWN, "Resource/Sound/SE/Respawn.mp3");			// クリック(UI)
	
	LoadSE(SOUND_SE::SE_GOAL, "Resource/Sound/Jingle/Clear.mp3");				// クリアジングル
	LoadSE(SOUND_SE::SE_GOALVOICE, "Resource/Sound/SE/ClearVoice.mp3");				// クリアSE

	LoadBGM(SOUND_BGM::BGM_Title, "Resource/Sound/BGM/Menu.mp3");	// BGM(Title&StageSelect)
	LoadBGM(SOUND_BGM::BGM_Game, "Resource/Sound/BGM/Cooking.mp3");					// BGM(Game)
}

void SoundManager::Finalize()
{
	// BGMの破棄
	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		DeleteSoundMem(it->second);
	}

	// SEの破棄
	for (auto it = mnSeHandleList.begin(); it != mnSeHandleList.end(); it++)
	{
		DeleteSoundMem(it->second);
	}

	if (sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}
}

void SoundManager::PlayBGM(SOUND_BGM bgm, bool isTop)
{
	// 現在再生されているBGMと同じ、かつ、最初からの再生でないのであれば return する
	if (mnNowPlayingBgm == bgm && !isTop)
	{
		return;
	}

	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		// 一致した種類のBGMがあれば
		if (it->first == bgm)
		{
			// BGMをループ再生
			PlaySoundMem(it->second, DX_PLAYTYPE_LOOP, isTop);
			// 現在の再生種類を更新
			mnNowPlayingBgm = bgm;

			break;
		}
	}
}

void SoundManager::PlaySE(SOUND_SE se, float pitchRate)
{
	for (auto it = mnSeHandleList.begin(); it != mnSeHandleList.end(); it++)
	{
		if (it->first == se)
		{
			// -- 前回変更した音程をリセット -- //
			ResetFrequencySoundMem(it->second);

			// -- 元の再生周波数を取得 -- //
			const int baseFrequency = GetFrequencySoundMem(it->second);

			// -- 元の周波数を基準に音程を変更 -- //
			const int newFrequency =
				static_cast<int>(baseFrequency * pitchRate);

			SetNextPlayFrequencySoundMem(
				newFrequency,
				it->second
			);

			// -- SEを再生 -- //
			PlaySoundMem(it->second, DX_PLAYTYPE_BACK);

			mnNowPlayingSe = se;

			break;
		}
	}
}

void SoundManager::LoadBGM(SOUND_BGM bgm, std::string filename)
{
	bool check = false;   // 重複して読み込んでいるかどうか
	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		if (it->first == bgm)
		{
			check = true;
			break;
		}
	}

	// 重複して読み込まれていたら何もしない
	if (check)
	{
		return;
	}

	// ファイル読み込み
	int handle = LoadSoundMem(filename.c_str());
	if (handle == -1)
	{
		return;   // 読み込み失敗していたら何もしない
	}

	// 今の音量設定を反映しておく(後から読み込んだBGMも同じ音量で鳴るように)
	ChangeVolumeSoundMem(ToDxVolume(mnBgmVolumeLevel), handle);

	// 読み込んだハンドルをリストに追加
	mnBgmHandleList.push_back(std::pair<SOUND_BGM, int>(bgm, handle));
}

void SoundManager::LoadSE(SOUND_SE se, std::string filename)
{
	bool check = false;   // 重複して読み込んでいるかどうか
	for (auto it = mnSeHandleList.begin(); it != mnSeHandleList.end(); it++)
	{
		if (it->first == se)
		{
			check = true;
			break;
		}
	}

	// 重複して読み込まれていたら何もしない
	if (check)
	{
		return;
	}

	// ファイル読み込み
	int handle = LoadSoundMem(filename.c_str());
	if (handle == -1)
	{
		return;   // 読み込み失敗していたら何もしない
	}

	// 今の音量設定を反映しておく
	ChangeVolumeSoundMem(ToDxVolume(mnSeVolumeLevel), handle);

	// 読み込んだハンドルをリストに追加
	mnSeHandleList.push_back(std::pair<SOUND_SE, int>(se, handle));
}

void SoundManager::StopBGM()
{
	for (auto it = mnBgmHandleList.begin(); it != mnBgmHandleList.end(); it++)
	{
		if (it->first == mnNowPlayingBgm)
		{
			// BGMが再生されているかどうか
			if (CheckSoundMem(it->second))
			{
				StopSoundMem(it->second);   // BGM停止
				break;
			}
		}
	}
}

bool SoundManager::IsPlayingSE(SOUND_SE se)
{
	for (const auto& pair : mnSeHandleList)
	{
		if (pair.first == se)
		{
			return CheckSoundMem(pair.second) == 1;
		}
	}
	return false;
}

bool SoundManager::IsPlayingBGM(SOUND_BGM bgm)
{
	for (const auto& pair : mnBgmHandleList)
	{
		if (pair.first == bgm)
		{
			// CheckSoundMem … 再生中なら1、停止中なら0、エラーなら-1を返す
			return CheckSoundMem(pair.second) == 1;
		}
	}
	return false;   // 読み込まれていないBGMは「再生していない」扱い
}

// ------------------------------------------------------------
// 音量調節
// ------------------------------------------------------------
void SoundManager::SetBGMVolume(int level)
{
	mnBgmVolumeLevel = ClampVolumeLevel(level);
	ApplyBGMVolume();
}

void SoundManager::SetSEVolume(int level)
{
	mnSeVolumeLevel = ClampVolumeLevel(level);
	ApplySEVolume();
}

int SoundManager::ClampVolumeLevel(int level)
{
	if (level < 0)
		return 0;
	if (level > VOLUME_LEVEL_MAX)
		return VOLUME_LEVEL_MAX;
	return level;
}

int SoundManager::ToDxVolume(int level)
{
	// ChangeVolumeSoundMem は 0(無音) ～ 255(最大)
	return 255 * level / VOLUME_LEVEL_MAX;
}

void SoundManager::ApplyBGMVolume()
{
	// memo : ChangeVolumeSoundMem は再生中の音にもすぐ効き、以降の再生にも残り続ける
	for (const auto& pair : mnBgmHandleList)
	{
		ChangeVolumeSoundMem(ToDxVolume(mnBgmVolumeLevel), pair.second);
	}
}

void SoundManager::ApplySEVolume()
{
	for (const auto& pair : mnSeHandleList)
	{
		ChangeVolumeSoundMem(ToDxVolume(mnSeVolumeLevel), pair.second);
	}
}
