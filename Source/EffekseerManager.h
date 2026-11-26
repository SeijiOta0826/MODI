#pragma once

#include "ScreenConfig.h"
#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include <string>
#include <unordered_map>
#include <vector>

// =============================================================================
// ■ EffekseerManager クラス（Effekseer 1.7対応・本体管理と読み込み専用クラス）
// =============================================================================
// 【全体の流れ】
//   1. Game::InitializeDxLib() : SetUseDirect3DVersion(DX_DIRECT3D_11) の後に DxLib_Init()
//   2. Game::Game()            : EffekseerManager::GetInstance().Initialize()
//   3. Game::UpdateLogic()     : EffekseerManager::GetInstance().Update() （毎フレーム UpdateEffekseer2D()）
//   4. 各 Effect〇〇 クラス     : Load(path, 拡大率) → Play(path, x, y) → DrawEffect(handle)
//   5. Game::~Game()           : DxLib_End() の直前に EffekseerManager::GetInstance().Finalize()
// =============================================================================
class EffekseerManager final
{
public:
    // --- 定数定義 ---
    static constexpr int   MAX_PARTICLES = 8000; // 最大パーティクル数
    static constexpr int   INVALID_HANDLE = -1;   // 無効なハンドル番号
    static constexpr int   EFFEKSEER_OK = 0;    // 正常・再生中を表す値
    static constexpr int   MAX_SUPPORTED_VER = 1710; // Effekseer 1.7 が対応する最大ファイルバージョン
    static constexpr float DEFAULT_SCALE = 1.0f; // 標準の拡大率
    static constexpr float DEFAULT_ANGLE = 0.0f; // 標準の角度（ラジアン）
    static constexpr float ZERO_VALUE = 0.0f;

public:
    // シングルトンインスタンスを取得する
    // 呼び出し例 : auto& effekseer = EffekseerManager::GetInstance();
    static EffekseerManager& GetInstance() noexcept
    {
        static EffekseerManager instance;
        return instance;
    }

    EffekseerManager(const EffekseerManager&) = delete;
    EffekseerManager& operator=(const EffekseerManager&) = delete;
    EffekseerManager(EffekseerManager&&) = delete;
    EffekseerManager& operator=(EffekseerManager&&) = delete;

    // -------------------------------------------------------------------------
    // ゲーム起動時・終了時・毎フレームの基本関数
    // -------------------------------------------------------------------------

    // Effekseer を初期化する（Game::Game() で 1 回だけ呼ぶ）
    // 呼び出し例 : EffekseerManager::GetInstance().Initialize();
    bool Initialize();

    // Effekseer を終了しリソースを解放する（Game::~Game() の DxLib_End() 直前で呼ぶ）
    // 呼び出し例 : EffekseerManager::GetInstance().Finalize();
    void Finalize();

    // 再生中エフェクトを 1 フレーム進める（毎フレーム Update で 1 回呼ぶ）
    // 呼び出し例 : EffekseerManager::GetInstance().Update();
    void Update();

    // -------------------------------------------------------------------------
    // 各エフェクトの .cpp から呼ぶ操作関数
    // -------------------------------------------------------------------------

    // .efk / .efkefc を拡大率付きで読み込む（同じパスはキャッシュを再利用）
    // 失敗時は -1 (INVALID_HANDLE) を返し、画面左上と出力ウィンドウに原因を表示する
    // 呼び出し例 : int res = EffekseerManager::GetInstance().Load("Resource/Effect/Effekseer/Click/Click.efkefc", 40.0f);
    int Load(const std::string& filePath, float loadScale = DEFAULT_SCALE);

    // エフェクトを 2D 座標 (x, y) で再生し、再生中ハンドルを返す
    // 内部で SetPosPlayingEffekseer2DEffect(handle, x, y, 0.0f) を自動で呼ぶ
    // 呼び出し例 : int h = EffekseerManager::GetInstance().Play(EFFECT_PATH, posX, posY);
    int Play(const std::string& filePath, float x, float y, float scale = DEFAULT_SCALE, float angleRad = DEFAULT_ANGLE);

    // 指定した 1 つの再生中エフェクトを描画する（Effect〇〇::Draw() から呼ぶ）
    // 呼び出し例 : EffekseerManager::GetInstance().DrawEffect(m_handle);
    void DrawEffect(int playingHandle) const;

    // 再生中の 2D エフェクトを一括描画する（背景・キャラ・UI描画の後、ScreenFlip() の前で呼ぶ用）
    // 呼び出し例 : EffekseerManager::GetInstance().DrawAll();
    void DrawAll() const;

    // 再生中のエフェクトを停止する
    // 呼び出し例 : EffekseerManager::GetInstance().Stop(m_handle);
    void Stop(int playingHandle);

    // 再生中のエフェクトの 2D 座標を変更する
    // 呼び出し例 : EffekseerManager::GetInstance().SetPosition(m_handle, x, y);
    void SetPosition(int playingHandle, float x, float y);

    // 再生中のエフェクトの拡大率を変更する
    // 呼び出し例 : EffekseerManager::GetInstance().SetScale(m_handle, 2.0f);
    void SetScale(int playingHandle, float scale);

    // 再生中のエフェクトの Z 軸回転角度（ラジアン）を変更する
    // 呼び出し例 : EffekseerManager::GetInstance().SetRotation(m_handle, angleRad);
    void SetRotation(int playingHandle, float angleRad);

    // 再生中のエフェクトの再生速度を変更する（1.0f が通常速度）
    // 呼び出し例 : EffekseerManager::GetInstance().SetSpeed(m_handle, 1.5f);
    void SetSpeed(int playingHandle, float speed);

    // 指定ハンドルがまだ再生中なら true、終了していれば false を返す
    // 呼び出し例 : if (!EffekseerManager::GetInstance().IsPlaying(m_handle)) { Kill(); }
    [[nodiscard]] bool IsPlaying(int playingHandle) const;

private:
    EffekseerManager() = default;
    ~EffekseerManager() = default;

    bool m_isInitialized = false;
    mutable bool m_updatedSinceLastDraw = false;
    std::unordered_map<std::string, int> m_resourceHandles;
    std::vector<::Effekseer::EffectRef>  m_effects;
};