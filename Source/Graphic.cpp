#include "Graphic.h"

#include "Master.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"

#include "GameObject.h"
#include "Transform.h"

#include "GameConfig.h"

Graphic::Graphic(std::string _filename)
{
    // -- 画像ロード -- //
    mnGraphHandle =
        Master::mpResource->LoadGraphics(_filename);

    // -- 画像のサイズ取得 -- //
    int size_x;
    int size_y;
    GetGraphSize(mnGraphHandle, &size_x, &size_y);

    mvSize.x = static_cast<float>(size_x);
    mvSize.y = static_cast<float>(size_y);
}

void Graphic::Initialize()
{
    // -- GameObjectとしての拡縮値の取得 -- //
    auto transform = mpGameObject->GetModule<Transform>();
    if (transform == nullptr)
        return;

    VECTOR scale = VGet(0.0f, 0.0f, 0.0f);
    scale.x = GameConfig::CELL_SIZE / mvSize.x;
    scale.y = GameConfig::CELL_SIZE / mvSize.y;

    transform->SetScale(scale);
}

void Graphic::SetSourceRect(int _x, int _y, int _width, int _height)
{
    mbUseSourceRect = true;
    mnSrcX = _x;
    mnSrcY = _y;
    mnSrcWidth = _width;
    mnSrcHeight = _height;

    mvSize.x = static_cast<float>(_width);
    mvSize.y = static_cast<float>(_height);

    if (mpGameObject != nullptr)
    {
        if (auto transform = mpGameObject->GetModule<Transform>())
        {
            VECTOR scale = VGet(0.0f, 0.0f, 0.0f);
            scale.x = GameConfig::CELL_SIZE / mvSize.x;
            scale.y = GameConfig::CELL_SIZE / mvSize.y;
            transform->SetScale(scale);
        }
    }
}

void Graphic::Draw()
{
    auto transform = mpGameObject->GetModule<Transform>();
    if (mnGraphHandle == -1 || transform == nullptr)
        return;

    // -- カメラのズームと揺れを反映した描画座標に変換 -- //
    VECTOR drawPosition = transform->GetPosition();

    // -- 通常時はズーム倍率1.0倍 -- //
    float zoom = 1.0f;

    if (Master::mpSceneManager != nullptr)
    {
        if (auto* currentScene = Master::mpSceneManager->GetCurrentScene())
        {
            if (auto* camera = currentScene->GetCamera())
            {
                // ワールド座標を画面座標へ変換
                drawPosition = camera->WorldToScreen(drawPosition);

                // 現在のカメラズーム倍率を取得
                zoom = camera->GetZoom();
            }
        }
    }

    const auto rotation = transform->GetRotation();
    const auto scale = transform->GetScale();

    if (mbUseSourceRect)
    {
        // 指定範囲を切り出して描画する
        DrawRectRotaGraph3(
            static_cast<int>(drawPosition.x),
            static_cast<int>(drawPosition.y),
            mnSrcX,
            mnSrcY,
            mnSrcWidth,
            mnSrcHeight,
            mnSrcWidth / 2,
            mnSrcHeight / 2,
            scale.x * zoom,
            scale.y * zoom,
            rotation.z,
            mnGraphHandle,
            TRUE,
            FALSE
        );
        return;
    }

    DrawRotaGraph3(
        static_cast<int>(drawPosition.x),
        static_cast<int>(drawPosition.y),
        static_cast<int>(mvSize.x / 2.0f),
        static_cast<int>(mvSize.y / 2.0f),

        // -- GameObjectの拡縮 × カメラのズーム -- //
        scale.x * zoom,
        scale.y * zoom,

        rotation.z,
        mnGraphHandle,
        TRUE,
        FALSE
    );
}