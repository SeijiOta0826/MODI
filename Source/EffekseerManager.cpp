#include "EffekseerManager.h"
#include <cstring>
#include <fstream>

// =============================================================================
// ■ Effekseer 2D の初期化処理
// =============================================================================
bool EffekseerManager::Initialize()
{
    // すでに初期化済みなら何もしない
    if (m_isInitialized)
    {
        return true;
    }

    // 1. Effekseer本体を初期化する（失敗した場合は DirectX11 設定が抜けている可能性が高い）
    if (Effekseer_Init(MAX_PARTICLES) == INVALID_HANDLE)
    {
        printfDx("【Effekseerエラー】Effekseer_Init に失敗しました。SetUseDirect3DVersion(DX_DIRECT3D_11) を確認してください。\n");
        return false;
    }

    // 2. ウインドウ／フルスクリーン切り替え時などにエフェクト画像が消えるのを防ぐ（公式サンプル必須設定）
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
    Effekseer_SetGraphicsDeviceLostCallbackFunctions();

    // 3. 2D画面の解像度（1920×1080 等）を Effekseer の2Dカメラに設定し、3D用Zバッファを有効化する
    Effekseer_Set2DSetting(
        static_cast<int>(ScreenConfig::SCREEN_WIDTH),
        static_cast<int>(ScreenConfig::SCREEN_HEIGHT)
    );
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    // 4. 標準ファイル読み込み（DefaultFileInterface）を設定し、Textureフォルダ内の .png を確実に読み込ませる
    auto manager2d = GetEffekseer2DManager();
    auto renderer2d = GetEffekseer2DRenderer();
    if (manager2d != nullptr)
    {
        auto setting = manager2d->GetSetting();
        if (setting != nullptr)
        {
            auto defaultFile = ::Effekseer::MakeRefPtr<::Effekseer::DefaultFileInterface>();
            setting->SetEffectLoader(::Effekseer::Effect::CreateEffectLoader(defaultFile));
            if (renderer2d != nullptr)
            {
                setting->SetTextureLoader(renderer2d->CreateTextureLoader(defaultFile));
                setting->SetModelLoader(renderer2d->CreateModelLoader(defaultFile));
                setting->SetMaterialLoader(renderer2d->CreateMaterialLoader(defaultFile));
            }
        }
    }

    m_isInitialized = true;
    m_updatedSinceLastDraw = false;
    return true;
}

// =============================================================================
// ■ Effekseer 2D の終了処理
// =============================================================================
void EffekseerManager::Finalize()
{
    if (!m_isInitialized)
    {
        return;
    }

    // Effkseer_End() を呼ぶ前に、保持しているエフェクトの参照（EffectRef）をすべて解放する
    m_resourceHandles.clear();
    m_effects.clear();

    // Effekseer本体を終了する（※関数名は公式ライブラリの仕様で "Effkseer_End"）
    Effkseer_End();
    m_isInitialized = false;
    m_updatedSinceLastDraw = false;
}

// =============================================================================
// ■ 毎フレームの全体更新処理
// =============================================================================
void EffekseerManager::Update()
{
    if (!m_isInitialized)
    {
        return;
    }

    // 再生中の全2Dエフェクトのフレーム（時間）を1ステップ進める
    UpdateEffekseer2D();
    m_updatedSinceLastDraw = true;
}

// =============================================================================
// ■ エフェクトファイル（.efkefc）の安全な読み込み処理
// =============================================================================
int EffekseerManager::Load(const std::string& filePath)
{
    if (!m_isInitialized)
    {
        printfDx("【Effekseerエラー】Initialize() が呼ばれていません！Game.cpp を確認してください。\n");
        return INVALID_HANDLE;
    }

    if (filePath.empty())
    {
        return INVALID_HANDLE;
    }

    // すでに一度読み込んだファイルなら、キャッシュ（記憶）から即座に番号を返す
    const auto it = m_resourceHandles.find(filePath);
    if (it != m_resourceHandles.end())
    {
        return it->second;
    }

    // -------------------------------------------------------------------------
    // 1. C++標準のファイルストリームで .efkefc ファイルを検査しながらメモリに読み込む
    //    （※ファイルが無い場合やバージョン違いの場合に、例外クラッシュせず画面左上に理由を出すため）
    // -------------------------------------------------------------------------
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        printfDx("【Effekseerエラー】ファイルが見つかりません: %s\n", filePath.c_str());
        return INVALID_HANDLE;
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize < 16)
    {
        printfDx("【Effekseerエラー】ファイルが空か壊れています: %s\n", filePath.c_str());
        return INVALID_HANDLE;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    // 先頭4文字が "EFKE" または "SKFE" でなければ Effekseer の実行用ファイルではない
    if (std::memcmp(buffer.data(), "EFKE", 4) != 0 && std::memcmp(buffer.data(), "SKFE", 4) != 0)
    {
        printfDx("【Effekseerエラー】.efkefc形式ではありません: %s\n", filePath.c_str());
        return INVALID_HANDLE;
    }

    // ファイル内部の "SKFE" チャンクを探して、保存された Effekseer のバージョン番号を調べる
    int effectVersion = -1;
    for (size_t i = 0; i + 8 <= buffer.size(); ++i)
    {
        if (std::memcmp(&buffer[i], "SKFE", 4) == 0)
        {
            std::memcpy(&effectVersion, &buffer[i + 4], sizeof(int));
            break;
        }
    }

    if (effectVersion < 0)
    {
        printfDx("【Effekseerエラー】実行用データ(SKFE)が含まれていません: %s\n", filePath.c_str());
        return INVALID_HANDLE;
    }

    // Effekseer 1.7（最大1710）より新しい 1.8用ファイル（1800など）の場合はクラッシュ前に止めて教える
    if (effectVersion > MAX_SUPPORTED_VER)
    {
        printfDx("【Effekseerエラー】1.8用のファイル(Ver:%d)です！Effekseer 1.7で保存し直してください: %s\n", effectVersion, filePath.c_str());
        return INVALID_HANDLE;
    }

    // -------------------------------------------------------------------------
    // 2. LoadEffekseerEffect 内部の nullptr アクセス違反バグを回避するため、
    //    上で読み込んだメモリバッファ（buffer）から直接 Effect::Create を呼んで生成する
    // -------------------------------------------------------------------------
    auto manager2d = GetEffekseer2DManager();
    auto setting = (manager2d != nullptr) ? manager2d->GetSetting() : nullptr;
    if (setting == nullptr)
    {
        printfDx("【Effekseerエラー】2DManagerが未初期化です（DirectX11設定を確認してください）\n");
        return INVALID_HANDLE;
    }

    // .efkefc が置かれているフォルダ（例: "Resource/Effect/Effekseer"）を切り出し、
    // その中にある Texture フォルダから .png 画像を探せるように基準パス（materialDir）を渡す
    std::u16string materialDir;
    const size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        const std::string dirStr = filePath.substr(0, lastSlash);
        for (unsigned char c : dirStr)
        {
            materialDir.push_back(static_cast<char16_t>(c));
        }
    }

    // メモリバッファ版の Effect::Create は失敗時も絶対にクラッシュせず nullptr を返す
    ::Effekseer::EffectRef effect = ::Effekseer::Effect::Create(
        setting,
        buffer.data(),
        static_cast<int32_t>(buffer.size()),
        DEFAULT_SCALE,
        materialDir.empty() ? nullptr : materialDir.c_str()
    );

    if (effect == nullptr)
    {
        printfDx("【Effekseerエラー】エフェクト生成に失敗しました (Ver:%d): %s\n", effectVersion, filePath.c_str());
        return INVALID_HANDLE;
    }

    // 読み込み成功！リストに保存して次回から使い回せるようにする
    const int resourceHandle = static_cast<int>(m_effects.size());
    m_effects.push_back(effect);
    m_resourceHandles.emplace(filePath, resourceHandle);
    return resourceHandle;
}

// =============================================================================
// ■ エフェクトの再生開始処理
// =============================================================================
int EffekseerManager::Play(const std::string& filePath, float x, float y, float scale, float angleRad)
{
    if (!m_isInitialized)
    {
        printfDx("【Effekseerエラー】Initialize() が呼ばれていません！Game.cpp を確認してください。\n");
        return INVALID_HANDLE;
    }

    // まだ読み込んでいないファイルならここで自動的に Load() される
    const int resourceHandle = Load(filePath);
    if (resourceHandle < 0 || resourceHandle >= static_cast<int>(m_effects.size()))
    {
        return INVALID_HANDLE;
    }

    auto manager2d = GetEffekseer2DManager();
    if (manager2d == nullptr)
    {
        return INVALID_HANDLE;
    }

    // 1. まず原点 (0, 0, 0) でエフェクトインスタンスを生成する
    const int playingHandle = manager2d->Play(m_effects[resourceHandle], ZERO_VALUE, ZERO_VALUE, ZERO_VALUE);
    if (playingHandle == INVALID_HANDLE)
    {
        return INVALID_HANDLE;
    }

    // 2. SetPosPlayingEffekseer2DEffect を使って2D画面の座標 (x, y) を設定する
    //    （※Effekseer内部では2DのY座標がマイナス反転されるため、必ずこの関数を通して座標をセットする）
    SetPosPlayingEffekseer2DEffect(playingHandle, x, y, ZERO_VALUE);

    // 3. 拡大率（scale）とZ軸の回転角度（angleRad）を設定する
    SetScalePlayingEffekseer2DEffect(playingHandle, scale, scale, scale);
    SetRotationPlayingEffekseer2DEffect(playingHandle, ZERO_VALUE, ZERO_VALUE, angleRad);

    // 4. Play() した直後のフレームの DrawEffect() でも確実に0フレーム目のパーティクルが出るようにする
    m_updatedSinceLastDraw = false;

    return playingHandle;
}

// =============================================================================
// ■ 個別エフェクトの描画処理
// =============================================================================
void EffekseerManager::DrawEffect(int playingHandle) const
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    // Scene.cpp で Update() を呼び忘れている場合や、Play() 直後のフレームでも確実にパーティクルを生成・進行させる
    if (!m_updatedSinceLastDraw)
    {
        UpdateEffekseer2D();
        m_updatedSinceLastDraw = true;
    }

    // 再生が終了している場合は描画しない
    if (IsEffekseer2DEffectPlaying(playingHandle) != EFFEKSEER_OK)
    {
        return;
    }

    // 他の2D描画で3D用ZバッファがOFFにされていても確実に描けるようにONにする
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    // 指定された1つのエフェクト（playingHandle）だけを描画する
    DrawEffekseer2D_Begin();
    DrawEffekseer2D_Draw(playingHandle);
    DrawEffekseer2D_End();
}

// =============================================================================
// ■ エフェクトの強制停止
// =============================================================================
void EffekseerManager::Stop(int playingHandle)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    StopEffekseer2DEffect(playingHandle);
}

// =============================================================================
// ■ 再生中エフェクトの座標変更（キャラ追従・カメラシェイク追従用）
// =============================================================================
void EffekseerManager::SetPosition(int playingHandle, float x, float y)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetPosPlayingEffekseer2DEffect(playingHandle, x, y, ZERO_VALUE);
}

// =============================================================================
// ■ 再生中エフェクトの拡大率変更
// =============================================================================
void EffekseerManager::SetScale(int playingHandle, float scale)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetScalePlayingEffekseer2DEffect(playingHandle, scale, scale, scale);
}

// =============================================================================
// ■ 再生中エフェクトの回転角度変更（ラジアン）
// =============================================================================
void EffekseerManager::SetRotation(int playingHandle, float angleRad)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetRotationPlayingEffekseer2DEffect(playingHandle, ZERO_VALUE, ZERO_VALUE, angleRad);
}

// =============================================================================
// ■ 再生中エフェクトの再生スピード変更
// =============================================================================
void EffekseerManager::SetSpeed(int playingHandle, float speed)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetSpeedPlayingEffekseer2DEffect(playingHandle, speed);
}

// =============================================================================
// ■ 再生中かどうかの判定
// =============================================================================
bool EffekseerManager::IsPlaying(int playingHandle) const
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return false;
    }

    // 次のフレームの更新フェーズ（OnUpdate）に入ったので、描画前更新フラグをリセットする
    m_updatedSinceLastDraw = false;

    return IsEffekseer2DEffectPlaying(playingHandle) == EFFEKSEER_OK;
}