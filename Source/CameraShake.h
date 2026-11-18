#pragma once

#include <DxLib.h>

// =============================================================================
// ■ CameraShake クラス（画面の揺れを計算するクラス）
//
// 【このクラスの役割】
//   「どのくらいの強さで、何秒間揺らすか」を受け取り、毎フレームの揺れ幅を計算します。
//   ※ カメラ本体（Cameraクラス）とは別クラスにして役割を分けています。
//
// 【基本的な使い方（3ステップ）】
//   ① 揺らしたい瞬間（爆発・ダメージ・着地など）に Start() を1回呼ぶ
//      例：m_cameraShake.Start(0.3f, 15.0f); // 0.3秒間、強さ15で揺らす
//
//   ② 毎フレームの Update() で揺れを更新し、Camera に揺れ幅を渡す
//      例：m_cameraShake.Update(deltaTime);
//          m_camera.SetShakeOffset(m_cameraShake.GetOffset());
//
//   ③ あとは Camera 側が揺れを含めた位置を計算してくれるので、通常通り描画するだけ！
// =============================================================================
class CameraShake final
{
public:
    // --- 定数定義（マジックナンバーの排除） ---
    static constexpr float ZERO_VALUE = 0.0f;
    static constexpr int   RAND_RESOLUTION = 10000; // 乱数の細かさ
    static constexpr float RAND_SCALE = 2.0f;  // 0.0?1.0 を -1.0?+1.0 に広げる倍率
    static constexpr float RAND_OFFSET = 1.0f;  // 中心を0にするための引き算値

public:
    CameraShake() = default;
    ~CameraShake() = default;

    CameraShake(const CameraShake&) = default;
    CameraShake& operator=(const CameraShake&) = default;
    CameraShake(CameraShake&&) noexcept = default;
    CameraShake& operator=(CameraShake&&) noexcept = default;

    // -------------------------------------------------------------------------
    // ■ 揺れを開始する（揺らしたい瞬間に1回だけ呼ぶ）
    //   duration : 揺れる時間（秒）   ※例: 0.2f なら 0.2秒間揺れる
    //   power    : 揺れの強さ（px）   ※例: 10.0f なら 最大10ピクセル揺れる
    //   呼び出し例 : m_cameraShake.Start(0.3f, 12.0f);
    // -------------------------------------------------------------------------
    void Start(float duration, float power) noexcept;

    // -------------------------------------------------------------------------
    // ■ 揺れを強制的にピタッと止める（ポーズ画面を開いた時やシーン切り替え時に呼ぶ）
    //   呼び出し例 : m_cameraShake.Stop();
    // -------------------------------------------------------------------------
    void Stop() noexcept;

    // -------------------------------------------------------------------------
    // ■ 毎フレームの揺れ計算（SceneのUpdate関数の中で毎フレーム呼ぶ）
    //   deltaTime : 1フレームの経過時間
    //   呼び出し例 : m_cameraShake.Update(deltaTime);
    // -------------------------------------------------------------------------
    void Update(float deltaTime) noexcept;

    // -------------------------------------------------------------------------
    // ■ 計算された「今のフレームの揺れ幅」を取得する（Cameraに渡すときに呼ぶ）
    //   呼び出し例 : m_camera.SetShakeOffset(m_cameraShake.GetOffset());
    // -------------------------------------------------------------------------
    [[nodiscard]] const VECTOR& GetOffset() const noexcept { return m_offset; }

    // -------------------------------------------------------------------------
    // ■ 今現在、画面が揺れている最中かどうかを調べる
    //   戻り値 : 揺れていれば true / 止まっていれば false
    //   呼び出し例 : if (m_cameraShake.IsShaking()) { ... }
    // -------------------------------------------------------------------------
    [[nodiscard]] bool IsShaking() const noexcept { return m_elapsedTime < m_duration; }

private:
    // -1.0f ? +1.0f のランダムな小数を返す内部ヘルパー関数
    [[nodiscard]] static float GetRandomNormalized() noexcept;

private:
    VECTOR m_offset = VGet(ZERO_VALUE, ZERO_VALUE, ZERO_VALUE); // 現在の揺れ幅（X, Y, Z）
    float  m_duration = ZERO_VALUE;                               // 揺れる合計時間（秒）
    float  m_elapsedTime = ZERO_VALUE;                               // 揺れ始めてからの経過時間（秒）
    float  m_power = ZERO_VALUE;                               // 揺れの最大強さ（ピクセル）
};