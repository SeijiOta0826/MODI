#pragma once

#include "ScreenConfig.h"
#include <DxLib.h>

// =============================================================================
// ■ Camera クラス（2Dカメラの位置・揺れの反映・座標変換を行うクラス）
//
// 【基本的な使い方】
//   ① カメラを動かしたいとき
//      例：m_camera.SetPosition(playerX, playerY);
//
//   ② カメラシェイク（CameraShakeクラス）の揺れを反映したいとき（毎フレーム）
//      例：m_camera.SetShakeOffset(m_cameraShake.GetOffset());
//
//   ③ 画像やエフェクトを描画するとき
//      ・2D画像を描くとき   → VECTOR drawPos = m_camera.WorldToScreen(worldPos);
//      ・Effekseerと同期時 → m_camera.Apply(); の直後に Effekseer_Sync3DSetting();
// =============================================================================
class Camera final
{
public:
    // --- 初期設定の定数（DxLibの内部マクロと被らない命名） ---
    static constexpr float CENTER_X = ScreenConfig::SCREEN_WIDTH * 0.5f;
    static constexpr float CENTER_Y = ScreenConfig::SCREEN_HEIGHT * 0.5f;
    static constexpr float CAMERA_Z = -1000.0f;
    static constexpr float TARGET_Z = 0.0f;
    static constexpr float NEAR_DISTANCE = 1.0f;
    static constexpr float FAR_DISTANCE = 5000.0f;
    static constexpr float VIEW_SIZE = ScreenConfig::SCREEN_HEIGHT;
    static constexpr float DEFAULT_ZOOM = 1.0f;
    static constexpr float MIN_ZOOM = 0.01f;
    static constexpr float ZERO_VALUE = 0.0f;

public:
    Camera();
    ~Camera() = default;

    Camera(const Camera&) = default;
    Camera& operator=(const Camera&) = default;
    Camera(Camera&&) noexcept = default;
    Camera& operator=(Camera&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ■ カメラを初期状態（画面中央・揺れなし・ズーム等倍）に戻す
    //   呼び出し例 : m_camera.Initialize();
    // -------------------------------------------------------------------------
    void Initialize();

    // -------------------------------------------------------------------------
    // ■ カメラの現在位置と揺れをDxLib本体にセットする（描画前やEffekseer同期前に呼ぶ）
    //   呼び出し例 : m_camera.Apply();
    // -------------------------------------------------------------------------
    void Apply() const;

    // =========================================================================
    // カメラの基本座標（揺れていない本来の位置）
    // =========================================================================

    // ■ カメラの中心位置を変更する
    //   呼び出し例 : m_camera.SetPosition(960.0f, 540.0f);
    void SetPosition(const VECTOR& position) noexcept { m_position = position; }
    void SetPosition(float x, float y) noexcept;

    // ■ カメラの本来の中心位置を取得する（揺れを含まない座標）
    [[nodiscard]] const VECTOR& GetPosition() const noexcept { return m_position; }

    // =========================================================================
    // 揺れのズレ設定（※CameraShakeクラスから計算結果を受け取る用）
    // =========================================================================

    // ■ カメラシェイクの揺れ幅をセットする
    //   呼び出し例 : m_camera.SetShakeOffset(m_cameraShake.GetOffset());
    void SetShakeOffset(const VECTOR& offset) noexcept { m_shakeOffset = offset; }
    void SetShakeOffset(float offsetX, float offsetY) noexcept;

    // ■ 複数の揺れを重ねて足したいときに使う
    void AddShakeOffset(const VECTOR& offset) noexcept;

    // ■ 揺れ幅を (0, 0, 0) にリセットする
    //   呼び出し例 : m_camera.ResetShakeOffset();
    void ResetShakeOffset() noexcept { m_shakeOffset = VGet(ZERO_VALUE, ZERO_VALUE, ZERO_VALUE); }

    // ■ 現在セットされている揺れ幅を取得する
    [[nodiscard]] const VECTOR& GetShakeOffset() const noexcept { return m_shakeOffset; }

    // =========================================================================
    // 最終座標・座標変換（※描画やエフェクトの位置計算で使う用）
    // =========================================================================

    // ■ 「本来の位置 + 揺れ幅」を合算した最終的なカメラ座標を取得する
    [[nodiscard]] VECTOR GetFinalPosition() const noexcept;

    // ■ 2D描画時に足すだけでカメラ移動＆揺れが反映される「画面のズレ量」を取得する
    //   呼び出し例 : VECTOR offset = m_camera.GetDrawOffset();
    [[nodiscard]] VECTOR GetDrawOffset() const noexcept;

    // ■ ゲーム内の座標（ワールド座標）を、画面に描く座標（スクリーン座標）に変換する
    //   呼び出し例 : VECTOR drawPos = m_camera.WorldToScreen(playerPos);
    [[nodiscard]] VECTOR WorldToScreen(const VECTOR& worldPos) const noexcept;

    // ■ 画面上の座標（マウスカーソル位置など）を、ゲーム内の座標に変換する
    //   呼び出し例 : VECTOR worldMouse = m_camera.ScreenToWorld(mousePos);
    [[nodiscard]] VECTOR ScreenToWorld(const VECTOR& screenPos) const noexcept;

    // =========================================================================
    // ズーム設定
    // =========================================================================

    // ■ カメラのズーム倍率を変更する（1.0f が通常、2.0f で2倍拡大）
    //   呼び出し例 : m_camera.SetZoom(1.5f);
    void SetZoom(float zoom) noexcept;
    [[nodiscard]] float GetZoom() const noexcept { return m_zoom; }

private:
    VECTOR m_position = VGet(CENTER_X, CENTER_Y, ZERO_VALUE);       // 本来のカメラ中心座標
    VECTOR m_shakeOffset = VGet(ZERO_VALUE, ZERO_VALUE, ZERO_VALUE);   // シェイクによるズレ量
    float  m_zoom = DEFAULT_ZOOM;                               // ズーム倍率（1.0fが標準）
};