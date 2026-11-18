#pragma once
#include "GameObject.h"

class Player;

/* memo :
* Player の1マス分の「見た目」と「センサー」。
*   ・描画 (Graphic)
*   ・他オブジェクトに Player として検知されるための当たり判定 (Collider / Tag::PLAYER_CELL)
* 座標・有効状態は Player が毎フレーム設定する。
* Player 自身の衝突処理(押し戻し・接地)には使われない(Player::QueryPose が担当)。
*/
class PlayerPiece : public GameObject
{
public:
	PlayerPiece(Player* _player);
	~PlayerPiece() = default;

	void Init() override;
	void InitComponent() override;

	// -- 有効状態のアクセサ -- //
	void SetActive(bool _active);
	bool IsActive() const { return mbIsActive; }

	Player* GetPlayer() const { return mpPlayer; }

protected:
	const char* GetModelFilename() const override
	{
		return "Resource/GameObject/Player/Tofu.png";
	}
private:
	Player* mpPlayer = nullptr;	// ハブとなるPlayerのポインタ

	bool mbIsActive = false;	// Pieceの有効状態を示す
};