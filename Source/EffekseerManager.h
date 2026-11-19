#pragma once

#include "ScreenConfig.h"
#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include <string>
#include <unordered_map>
#include <vector>

// =============================================================================
// ■ EffekseerManager クラス（Effekseer 1.7対応・2Dエフェクト本体管理クラス）
//
// -----------------------------------------------------------------------------
// 【1. このクラスの役割】
//   ・Effekseer 2D の起動（Initialize）、毎フレームの更新（Update）、終了（Finalize）
//   ・.efkefc ファイルの安全な読み込みとキャッシュ（同じファイルを2回読み込まない）
//   ・各エフェクトクラス（EffectClick.cpp / EffectFall.cpp など）から呼ばれる
//     再生（Play）・描画（DrawEffect）・座標変更（SetPosition）・停止（Stop）の窓口
//   ※ 個別のエフェクトの処理はこのクラスには書かず、別々の Effect〇〇.cpp に書きます。
//
// -----------------------------------------------------------------------------
// 【2. Effekseer 1.7 ツールでのエフェクト作成＆保存手順（重要！）】
//   手順①：エフェクトに使いたい画像（.png）を、先に以下のフォルダへ入れる。
//           [ MODI / Resource / Effect / Effekseer / Texture / ]
//   手順②：Effekseer 1.7 ツールを起動し、画面左下の [3D ▼] を [2D] に切り替える。
//   手順③：手順①の Texture フォルダから画像を読み込んでエフェクトを作成する。
//   手順④：「ファイル」→「名前を付けて保存」で以下のフォルダに保存する。
//           [ MODI / Resource / Effect / Effekseer / 〇〇.efkefc ]
//           （※注意：必ず Effekseer 1.7 で保存すること。1.8 で保存すると読み込めません）
//
// -----------------------------------------------------------------------------
// 【3. ゲーム本体（Game.cpp / Scene.cpp）で呼ぶ場所（すでに設定済み）】
//   ・Game.cpp の InitializeDxLib() 内：
//       DxLib_Init() の前に   SetUseDirect3DVersion(DX_DIRECT3D_11);
//       DxLib_Init() の直後に EffekseerManager::GetInstance().Initialize();
//   ・Game.cpp の ~Game() 内：
//       DxLib_End() の直前に  EffekseerManager::GetInstance().Finalize();
//   ・Scene.cpp の Scene::UpdateOverlay() 内：
//       EffekseerManager::GetInstance().Update();
//
// -----------------------------------------------------------------------------
// 【4. 新しいエフェクト（別ファイルの Effect〇〇.cpp）を作るときの基本テンプレート】
//   ① コンストラクタ（生成時に再生を開始して番号 m_handle を受け取る）：
//        m_handle = EffekseerManager::GetInstance().Play(EFFECT_PATH, fX, fY, SCALE);
//
//   ② OnUpdate（毎フレーム再生中かチェックし、終わっていたら Kill() で消す）：
//        if (!EffekseerManager::GetInstance().IsPlaying(m_handle)) { Kill(); return; }
//        // ※カメラ揺れやキャラ移動に追従させる場合はここで SetPosition を呼ぶ：
//        // EffekseerManager::GetInstance().SetPosition(m_handle, screenX, screenY);
//
//   ③ Draw（自分の番号 m_handle のエフェクトを描画する）：
//        EffekseerManager::GetInstance().DrawEffect(m_handle);
//
//   ④ デストラクタ（途中でオブジェクトが消された場合にエフェクトも止める）：
//        if (EffekseerManager::GetInstance().IsPlaying(m_handle)) {
//            EffekseerManager::GetInstance().Stop(m_handle);
//        }
// =============================================================================
class EffekseerManager final
{
public:
    // --- 定数定義（マジックナンバーの排除） ---
    static constexpr int   MAX_PARTICLES = 8000; // 画面に同時表示できる最大パーティクル数
    static constexpr int   INVALID_HANDLE = -1;   // エラー・無効を表すハンドル番号
    static constexpr int   EFFEKSEER_OK = 0;    // Effekseerで「正常」「再生中」を表す戻り値
    static constexpr int   MAX_SUPPORTED_VER = 1710; // Effekseer 1.7 が対応する最大ファイルバージョン番号
    static constexpr float DEFAULT_SCALE = 1.0f; // 標準の拡大率（等倍）
    static constexpr float DEFAULT_ANGLE = 0.0f; // 標準の回転角度（0ラジアン）
    static constexpr float ZERO_VALUE = 0.0f; // 2D描画時の固定Z座標・X/Y軸回転の0値

public:
    // ■ シングルトンインスタンス（唯一の管理オブジェクト）を取得する
    //   説明     : どこからでも EffekseerManager::GetInstance().関数名() で呼び出せます。
    //   呼び出し例 : EffekseerManager::GetInstance().Play("Resource/Effect/Effekseer/Click.efkefc", x, y, 25.0f);
    static EffekseerManager& GetInstance() noexcept
    {
        static EffekseerManager instance;
        return instance;
    }

    // コピー・ムーブ禁止（シングルトンを安全に保つため）
    EffekseerManager(const EffekseerManager&) = delete;
    EffekseerManager& operator=(const EffekseerManager&) = delete;
    EffekseerManager(EffekseerManager&&) = delete;
    EffekseerManager& operator=(EffekseerManager&&) = delete;

    // =========================================================================
    // ■ ゲーム起動時・終了時・毎フレームの基本関数
    // =========================================================================

    // ■ Effekseer 2Dシステムの初期化
    //   説明     : ゲーム起動時、Game.cpp の DxLib_Init() 直後に1回だけ呼び出します。
    //   戻り値   : 成功したら true、失敗したら false
    //   呼び出し例 : EffekseerManager::GetInstance().Initialize();
    bool Initialize();

    // ■ Effekseer 2Dシステムの終了処理と全エフェクト解放
    //   説明     : ゲーム終了時、Game.cpp の DxLib_End() 直前に1回だけ呼び出します。
    //   呼び出し例 : EffekseerManager::GetInstance().Finalize();
    void Finalize();

    // ■ 再生中エフェクト全体の時間・アニメーション更新
    //   説明     : Scene.cpp の Scene::UpdateOverlay() 内で毎フレーム1回呼び出します。
    //   呼び出し例 : EffekseerManager::GetInstance().Update();
    void Update();

    // =========================================================================
    // ■ 各エフェクトの .cpp（EffectClick.cpp 等）から呼ぶ操作関数
    // =========================================================================

    // ■ エフェクトファイル（.efkefc）を読み込んでキャッシュする
    //   説明     : Play() の内部で自動的に呼ばれるため、普段は直接呼ばなくてOKです。
    //              1回読み込んだファイルは記憶されるため、2回目以降は高速に返ります。
    //   引数     : filePath = ".efkefc" ファイルのパス
    //   戻り値   : 読み込んだエフェクトのリソース番号（失敗時は INVALID_HANDLE = -1）
    //   呼び出し例 : EffekseerManager::GetInstance().Load("Resource/Effect/Effekseer/Click.efkefc");
    int Load(const std::string& filePath);

    // ■ 指定した2D座標にエフェクトを再生し、再生番号（playingHandle）を返す
    //   説明     : 各エフェクトクラスのコンストラクタで呼び出し、戻り値を m_handle に保存します。
    //   引数     : filePath = ".efkefc" ファイルのパス
    //              x, y     = 画面上の表示座標（ピクセル単位）
    //              scale    = 表示サイズの拡大倍率（省略時は 1.0f。2Dでは 15.0f?30.0f 前後が目安）
    //              angleRad = Z軸の回転角度（ラジアン単位。省略時は 0.0f）
    //   戻り値   : 再生中のエフェクト固有の番号（失敗時は INVALID_HANDLE = -1）
    //   呼び出し例1（通常再生） : m_handle = EffekseerManager::GetInstance().Play(EFFECT_PATH, fX, fY, SCALE);
    //   呼び出し例2（角度付き） : m_handle = EffekseerManager::GetInstance().Play(EFFECT_PATH, fX, fY, SCALE, fOutwardAngleRad);
    int Play(const std::string& filePath, float x, float y, float scale = DEFAULT_SCALE, float angleRad = DEFAULT_ANGLE);

    // ■ 指定した再生番号のエフェクト1つを画面に描画する
    //   説明     : 各エフェクトクラスの Draw() の中で呼び出します。
    //              背面エフェクト（EffectSteamなど）と前面エフェクト（EffectClickなど）の
    //              描画順を EffectManager 側で正しく制御できるように1個単位で描画します。
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //   呼び出し例 : EffekseerManager::GetInstance().DrawEffect(m_handle);
    void DrawEffect(int playingHandle) const;

    // ■ 指定した再生番号のエフェクトを強制停止する
    //   説明     : ループ再生エフェクトを止めたいときや、各エフェクトクラスのデストラクタで呼び出します。
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //   呼び出し例 : EffekseerManager::GetInstance().Stop(m_handle);
    void Stop(int playingHandle);

    // ■ 再生中のエフェクトの2D座標をリアルタイムに変更する
    //   説明     : プレイヤーに追従するエフェクト（光沢・感情UI）や、
    //              カメラシェイク（画面揺れ）に合わせてエフェクトの位置を動かすときに OnUpdate() で呼び出します。
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //              x, y          = 新しい画面上の座標（ピクセル単位）
    //   呼び出し例 : EffekseerManager::GetInstance().SetPosition(m_handle, screenPos.x, screenPos.y);
    void SetPosition(int playingHandle, float x, float y);

    // ■ 再生中のエフェクトの大きさ（拡大倍率）をリアルタイムに変更する
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //              scale         = 新しい拡大倍率
    //   呼び出し例 : EffekseerManager::GetInstance().SetScale(m_handle, 30.0f);
    void SetScale(int playingHandle, float scale);

    // ■ 再生中のエフェクトの回転角度（ラジアン）をリアルタイムに変更する
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //              angleRad      = 新しい回転角度（ラジアン単位）
    //   呼び出し例 : EffekseerManager::GetInstance().SetRotation(m_handle, angleRad);
    void SetRotation(int playingHandle, float angleRad);

    // ■ 再生中のエフェクトの再生スピードを変更する
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //              speed         = 再生速度倍率（1.0f = 標準、2.0f = 倍速、0.5f = スロー）
    //   呼び出し例 : EffekseerManager::GetInstance().SetSpeed(m_handle, 1.5f);
    void SetSpeed(int playingHandle, float speed);

    // ■ 指定した再生番号のエフェクトがまだ再生中かどうかを調べる
    //   説明     : 各エフェクトクラスの OnUpdate() で毎フレーム確認し、
    //              false（再生終了）になったら Kill() を呼んでエフェクトオブジェクトを消滅させます。
    //   引数     : playingHandle = Play() が返した再生番号（m_handle）
    //   戻り値   : 再生中なら true、再生終了（または無効な番号）なら false
    //   呼び出し例 : if (!EffekseerManager::GetInstance().IsPlaying(m_handle)) { Kill(); }
    [[nodiscard]] bool IsPlaying(int playingHandle) const;

private:
    EffekseerManager() = default;
    ~EffekseerManager() = default;

    bool m_isInitialized = false;                  // Initialize() が完了しているか
    mutable bool m_updatedSinceLastDraw = false;   // 描画前に UpdateEffekseer2D() が呼ばれたか
    std::unordered_map<std::string, int> m_resourceHandles; // ファイルパス → m_effects の添字キャッシュ
    std::vector<::Effekseer::EffectRef>  m_effects;         // 読み込んだエフェクトデータの保持リスト
};