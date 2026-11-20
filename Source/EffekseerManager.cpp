#include "EffekseerManager.h"
#include <cstdio>
#include <cstring>
#include <fstream>

namespace
{
    // 画面左上（printfDx）と Visual Studio の出力ウィンドウの両方にエラーを出すヘルパー
    void LogEffekseerError(const char* message, const char* detail = nullptr)
    {
        if (detail != nullptr)
        {
            printfDx("【Effekseerエラー】%s: %s\n", message, detail);
            char buf[512];
            std::snprintf(buf, sizeof(buf), "[EffekseerError] %s: %s\n", message, detail);
            OutputDebugStringA(buf);
        }
        else
        {
            printfDx("【Effekseerエラー】%s\n", message);
            char buf[512];
            std::snprintf(buf, sizeof(buf), "[EffekseerError] %s\n", message);
            OutputDebugStringA(buf);
        }
    }

}

bool EffekseerManager::Initialize()
{
    if (m_isInitialized)
    {
        return true;
    }

    if (Effekseer_Init(MAX_PARTICLES) == INVALID_HANDLE)
    {
        return false;
    }

    // フルスクリーン切り替え時などにリソースが消えるのを防ぐ（公式サンプル必須設定）
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);
    Effekseer_SetGraphicsDeviceLostCallbackFunctions();

    // 2D画面サイズと3D用Zバッファを有効化する
    Effekseer_Set2DSetting(
        static_cast<int>(ScreenConfig::SCREEN_WIDTH),
        static_cast<int>(ScreenConfig::SCREEN_HEIGHT)
    );
    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    // 標準のファイル読み込みクラスを設定し、同階層の Texture フォルダ(.png)を確実に読み込めるようにする
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

void EffekseerManager::Finalize()
{
    if (!m_isInitialized)
    {
        return;
    }

    m_resourceHandles.clear();
    m_effects.clear();

    Effkseer_End();
    m_isInitialized = false;
    m_updatedSinceLastDraw = false;
}

void EffekseerManager::Update()
{
    if (!m_isInitialized)
    {
        return;
    }

    UpdateEffekseer2D();
    m_updatedSinceLastDraw = true;
}

int EffekseerManager::Load(const std::string& filePath, float loadScale)
{
    if (!m_isInitialized)
    {
        LogEffekseerError("Initialize() が呼ばれていません！Game.cpp を確認してください");
        return INVALID_HANDLE;
    }

    if (filePath.empty())
    {
        LogEffekseerError("ファイルパスが空です");
        return INVALID_HANDLE;
    }

    // すでに読み込み済みのファイルならキャッシュを返す
    const auto it = m_resourceHandles.find(filePath);
    if (it != m_resourceHandles.end())
    {
        return it->second;
    }

    // -------------------------------------------------------------------------
    // 1. C++標準のファイルストリームで .efk / .efkefc を直接メモリに読み込む
    // -------------------------------------------------------------------------
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        LogEffekseerError("ファイルが見つかりません（パスと配置場所を確認してください）", filePath.c_str());
        return INVALID_HANDLE;
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize < 16)
    {
        LogEffekseerError("ファイルが空か壊れています", filePath.c_str());
        return INVALID_HANDLE;
    }

    std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    // 先頭4文字が "EFKE" (.efkefc) または "SKFE" (.efk) でなければ非対応形式
    if (std::memcmp(buffer.data(), "EFKE", 4) != 0 && std::memcmp(buffer.data(), "SKFE", 4) != 0)
    {
        LogEffekseerError(".efk / .efkefc 形式ではありません", filePath.c_str());
        return INVALID_HANDLE;
    }

    // ファイル内部の "SKFE" チャンクを探してバージョン番号を調べる
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
        LogEffekseerError("実行用データ(SKFE)が含まれていません", filePath.c_str());
        return INVALID_HANDLE;
    }

    // Effekseer 1.7 (最大1710) より新しい 1.8用ファイル (1800など) の場合はここで止めて教える
    if (effectVersion > MAX_SUPPORTED_VER)
    {
        char msg[256];
        std::snprintf(msg, sizeof(msg), "1.8用のファイル(Ver:%d)です！Effekseer 1.7で保存し直してください", effectVersion);
        LogEffekseerError(msg, filePath.c_str());
        return INVALID_HANDLE;
    }

    // -------------------------------------------------------------------------
    // 2. LoadEffekseerEffect 内部の nullptr クラッシュを回避しつつ、
    //    第2引数相当の loadScale（拡大率）を指定してエフェクトを生成する
    // -------------------------------------------------------------------------
    auto manager2d = GetEffekseer2DManager();
    auto setting = (manager2d != nullptr) ? manager2d->GetSetting() : nullptr;
    if (setting == nullptr)
    {
        LogEffekseerError("2DManagerが未初期化です（DirectX11設定を確認してください）");
        return INVALID_HANDLE;
    }

    // テクスチャ（同階層の Texture フォルダ）を探す基準フォルダパス（例: "Resource/Effect/Effekseer"）を作る
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

    // メモリバッファ版の Effect::Create（第4引数が LoadEffekseerEffect の第2引数と同じ拡大率）
    ::Effekseer::EffectRef effect = ::Effekseer::Effect::Create(
        setting,
        buffer.data(),
        static_cast<int32_t>(buffer.size()),
        loadScale,
        materialDir.empty() ? nullptr : materialDir.c_str()
    );

    if (effect == nullptr)
    {
        char msg[256];
        std::snprintf(msg, sizeof(msg), "エフェクト生成に失敗しました (Ver:%d)", effectVersion);
        LogEffekseerError(msg, filePath.c_str());
        return INVALID_HANDLE;
    }

    const int resourceHandle = static_cast<int>(m_effects.size());
    m_effects.push_back(effect);
    m_resourceHandles.emplace(filePath, resourceHandle);

  

    return resourceHandle;
}

int EffekseerManager::Play(const std::string& filePath, float x, float y, float scale, float angleRad)
{
    if (!m_isInitialized)
    {
        LogEffekseerError("Initialize() が呼ばれていません！Game.cpp を確認してください");
        return INVALID_HANDLE;
    }

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

    // ★重要：まず (0, 0, 0) で生成し、SetPosPlayingEffekseer2DEffect を呼ぶことで
    //   Effekseer内部の2D座標系 (x, -y) に正しく変換されてクリック位置に表示される
    const int playingHandle = manager2d->Play(m_effects[resourceHandle], ZERO_VALUE, ZERO_VALUE, ZERO_VALUE);
    if (playingHandle == INVALID_HANDLE)
    {
        return INVALID_HANDLE;
    }

    SetPosPlayingEffekseer2DEffect(playingHandle, x, y, ZERO_VALUE);
    SetScalePlayingEffekseer2DEffect(playingHandle, scale, scale, scale);
    SetRotationPlayingEffekseer2DEffect(playingHandle, ZERO_VALUE, ZERO_VALUE, angleRad);

    // 再生直後の描画でも0フレーム目のパーティクルが確実に出るようにフラグをリセット
    m_updatedSinceLastDraw = false;

    return playingHandle;
}

void EffekseerManager::DrawEffect(int playingHandle) const
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }
    if (!m_updatedSinceLastDraw)
    {
        UpdateEffekseer2D();
        m_updatedSinceLastDraw = true;
    }
    if (IsEffekseer2DEffectPlaying(playingHandle) != EFFEKSEER_OK)
    {
        return;
    }
    // DXライブラリ側の3D/ライティング設定が2Dエフェクトに干渉しないようにして描画
    ClearDrawScreenZBuffer();
    SetUseLighting(FALSE);
    SetUseZBufferFlag(FALSE);
    SetWriteZBufferFlag(FALSE);
    DrawEffekseer2D();
    // DXライブラリの Direct3D 設定を復元
    RefreshDxLibDirect3DSetting();
}

void EffekseerManager::DrawAll() const
{
    if (!m_isInitialized)
    {
        return;
    }

    if (!m_updatedSinceLastDraw)
    {
        UpdateEffekseer2D();
        m_updatedSinceLastDraw = true;
    }

    ClearDrawScreenZBuffer();

    SetUseZBuffer3D(TRUE);
    SetWriteZBuffer3D(TRUE);

    DrawEffekseer2D();
}

void EffekseerManager::Stop(int playingHandle)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    StopEffekseer2DEffect(playingHandle);
}

void EffekseerManager::SetPosition(int playingHandle, float x, float y)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetPosPlayingEffekseer2DEffect(playingHandle, x, y, ZERO_VALUE);
}

void EffekseerManager::SetScale(int playingHandle, float scale)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetScalePlayingEffekseer2DEffect(playingHandle, scale, scale, scale);
}

void EffekseerManager::SetRotation(int playingHandle, float angleRad)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetRotationPlayingEffekseer2DEffect(playingHandle, ZERO_VALUE, ZERO_VALUE, angleRad);
}

void EffekseerManager::SetSpeed(int playingHandle, float speed)
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return;
    }

    SetSpeedPlayingEffekseer2DEffect(playingHandle, speed);
}

bool EffekseerManager::IsPlaying(int playingHandle) const
{
    if (!m_isInitialized || playingHandle == INVALID_HANDLE)
    {
        return false;
    }

    // 次フレームの更新フェーズに入ったので描画前更新フラグをリセット
    m_updatedSinceLastDraw = false;

    return IsEffekseer2DEffectPlaying(playingHandle) == EFFEKSEER_OK;
}