#ifndef __SPACESHIP_H__
#define __SPACESHIP_H__

#include "GameUtil.h"
#include "GameObject.h"
#include "Shape.h"

class Spaceship : public GameObject
{
public:
	Spaceship();
	Spaceship(GLVector3f p, GLVector3f v, GLVector3f a, GLfloat h, GLfloat r);
	Spaceship(const Spaceship& s);
	virtual ~Spaceship(void);

	virtual void Update(int t);
	virtual void Render(void);

	virtual void Thrust(float t);
	virtual void Rotate(float r);
	virtual void Shoot(void);

	void SetSpaceshipShape(shared_ptr<Shape> spaceship_shape) { mSpaceshipShape = spaceship_shape; }
	void SetThrusterShape(shared_ptr<Shape> thruster_shape) { mThrusterShape = thruster_shape; }
	void SetBulletShape(shared_ptr<Shape> bullet_shape) { mBulletShape = bullet_shape; }

	// Activates invulnerability for the given duration in milliseconds
	void ActivateInvulnerability(int duration_ms);
	virtual bool IsInvulnerable() const override { return mInvulnerable; }

	bool CollisionTest(shared_ptr<GameObject> o);
	void OnCollision(const GameObjectList &objects);

	// --- Upgrade system ---------------------------------------------------
	// Each component can be upgraded up to MAX_UPGRADE_LEVEL times.
	// Gun   = lower fire-rate cooldown, Speed = stronger thrust,
	// Brake = passive velocity damping (ship coasts to a stop faster).
	static const int MAX_UPGRADE_LEVEL = 5;

	void ResetUpgrades();
	void UpgradeGun()   { if (mGunLevel   < MAX_UPGRADE_LEVEL) ++mGunLevel;   }
	void UpgradeSpeed() { if (mSpeedLevel < MAX_UPGRADE_LEVEL) ++mSpeedLevel; }
	void UpgradeBrake() { if (mBrakeLevel < MAX_UPGRADE_LEVEL) ++mBrakeLevel; }

	int GetGunLevel()   const { return mGunLevel;   }
	int GetSpeedLevel() const { return mSpeedLevel; }
	int GetBrakeLevel() const { return mBrakeLevel; }

private:
	float mThrust;

	// Invulnerability state — set via ActivateInvulnerability(), counted down each Update()
	bool mInvulnerable;
	int  mInvulnTimer;   // milliseconds remaining

	// Upgrade state
	int mGunLevel;
	int mSpeedLevel;
	int mBrakeLevel;
	int mFireCooldown;   // ms remaining before the next shot is allowed

	shared_ptr<Shape> mSpaceshipShape;
	shared_ptr<Shape> mThrusterShape;
	shared_ptr<Shape> mBulletShape;
};

#endif