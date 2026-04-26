#include <stdlib.h>
#include "GameUtil.h"
#include "Asteroid.h"
#include "BoundingShape.h"

Asteroid::Asteroid(void) : GameObject("Asteroid"), mKilledByBullet(false)
{
	mAngle = rand() % 360;
	mRotation = 0; // rand() % 90;
	mPosition.x = rand() / 2;
	mPosition.y = rand() / 2;
	mPosition.z = 0.0;
	mVelocity.x = 10.0 * cos(DEG2RAD*mAngle);
	mVelocity.y = 10.0 * sin(DEG2RAD*mAngle);
	mVelocity.z = 0.0;
}

Asteroid::~Asteroid(void)
{
}

bool Asteroid::CollisionTest(shared_ptr<GameObject> o)
{
	if (GetType() == o->GetType()) return false;
	// Don't collide with a ship that has invulnerability active — the ring protects it
	if (o->IsInvulnerable()) return false;
	if (mBoundingShape.get() == NULL) return false;
	if (o->GetBoundingShape().get() == NULL) return false;
	return mBoundingShape->CollisionTest(o->GetBoundingShape());
}

void Asteroid::OnCollision(const GameObjectList& objects)
{
	// Check whether a bullet is among the colliders.
	// If so this is a laser kill and should award points; a ship collision should not.
	for (GameObjectList::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		if ((*it)->GetType() == GameObjectType("Bullet"))
		{
			mKilledByBullet = true;
			break;
		}
	}
	mWorld->FlagForRemoval(GetThisPtr());
}
