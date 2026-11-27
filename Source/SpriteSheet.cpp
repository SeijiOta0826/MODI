#include "SpriteSheet.h"
#include "ResourceManager.h"
#include "Master.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include <algorithm>
#include <DxLib.h>

// 初期化処理
SpriteSheet::SpriteSheet(
    float fX,
    float fY,
    const DivGraphData* pData,
    float fFrameDuration,
    bool bIsLoop,
    double dScale
)
    : EffectBase(fX, fY, INFINITE_LIFE)
    , m_pDivData(pData)
    , m_fFrameDuration(fFrameDuration)
    , m_bIsLoop(bIsLoop)
    , m_dScale(dScale)
{
    // 画像データが無ければ即消滅させる
    if (m_pDivData == nullptr || m_pDivData->allNum <= 0)
    {
        m_bIsDead = true;
        return;
    }

    // ループしない場合は「総コマ数 × 1コマ秒数」を寿命にする
    if (!m_bIsLoop)
    {
        m_fLifeTime = static_cast<float>(m_pDivData->allNum) * m_fFrameDuration;
    }
}

// 描画処理
void SpriteSheet::Draw() const
{
    if (m_bIsDead || m_pDivData == nullptr)
        return;

    // 現在のコマ画像を取得
    const int graphHandle = m_pDivData->divHandleList[GetCurrentFrameIndex()];
    if (graphHandle == INVALID_HANDLE)
        return;

    // 色とブレンドモードを適用
    if (HasCustomColor())
    {
        SetDrawBright(m_nRed, m_nGreen, m_nBlue);
    }

    const int blendMode = m_bIsAddBlend ? DX_BLENDMODE_ADD : DX_BLENDMODE_ALPHA;
    SetDrawBlendMode(blendMode, m_nAlpha);

    // 描画座標と拡大率
    float drawX = m_fX;
    float drawY = m_fY;
    double drawScale = m_dScale;

    // カメラを使用する場合
    if (mbUseCamera && Master::mpSceneManager != nullptr)
    {
        auto* currentScene = Master::mpSceneManager->GetCurrentScene();

        if (currentScene != nullptr)
        {
            auto* camera = currentScene->GetCamera();

            if (camera != nullptr)
            {
                const VECTOR worldPosition =
                    VGet(m_fX, m_fY, 0.0f);

                const VECTOR screenPosition =
                    camera->WorldToScreen(worldPosition);

                drawX = screenPosition.x;
                drawY = screenPosition.y;

                // カメラのズームを画像サイズにも反映
                drawScale *= camera->GetZoom();
            }
        }
    }

    // 中心指定の有無で描画関数を切り替え
    if (HasCustomPivot())
    {
        DrawRotaGraph2F(
            drawX,
            drawY,
            m_fPivotX,
            m_fPivotY,
            drawScale,
            m_dAngle,
            graphHandle,
            TRUE
        );
    }
    else
    {
        DrawRotaGraphF(
            drawX,
            drawY,
            drawScale,
            m_dAngle,
            graphHandle,
            TRUE
        );
    }

    // 描画設定を元に戻す
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, MIN_COLOR_VALUE);
    if (HasCustomColor())
    {
        SetDrawBright(MAX_COLOR_VALUE, MAX_COLOR_VALUE, MAX_COLOR_VALUE);
    }
}

// 経過時間から現在のコマ番号を算出
int SpriteSheet::GetCurrentFrameIndex() const
{
    const int elapsedFrames = static_cast<int>(m_fElapsedTime / m_fFrameDuration);

    // ループ時は先頭コマへ戻す
    if (m_bIsLoop)
    {
        return elapsedFrames % m_pDivData->allNum;
    }

    // 非ループ時は最終コマで止める
    return std::clamp(elapsedFrames, 0, m_pDivData->allNum - 1);
}

// デフォルト色（白）から変更されているか
bool SpriteSheet::HasCustomColor() const
{
    return (m_nRed != MAX_COLOR_VALUE) || (m_nGreen != MAX_COLOR_VALUE) || (m_nBlue != MAX_COLOR_VALUE);
}

// 中心座標が個別指定されているか
bool SpriteSheet::HasCustomPivot() const
{
    return (m_fPivotX >= 0.0f) && (m_fPivotY >= 0.0f);
}