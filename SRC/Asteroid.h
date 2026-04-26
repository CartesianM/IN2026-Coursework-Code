#ifndef __ASTEROID_H__
#define __ASTEROID_H__

#include "GameObject.h"

class Asteroid : public GameObject
{
public:
	Asteroid(void);
	~Asteroid(void);

	bool CollisionTest(shared_ptr<GameObject> o);
	void OnCollision(const GameObjectList& objects);

	// True only when a bullet was the cause of destruction — used to gate scoring
	bool WasKilledByBullet() const { return mKilledByBullet; }

private:
	bool mKilledByBullet;
};

#endif
