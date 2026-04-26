#ifndef __BLACKHOLE_H__
#define __BLACKHOLE_H__

#include "GameObject.h"

// A short-lived gravitational well that pulls in nearby asteroids and consumes
// any that cross the event horizon. Spawned by the player at a random world
// position when they spend 110 points (F key). Self-removes from the world
// after its time-to-live expires.
class BlackHole : public GameObject
{
public:
	BlackHole();
	virtual ~BlackHole();

	virtual void Update(int t);
	virtual void Render(void);

	// How many milliseconds the black hole stays active before despawning
	static const int LIFESPAN_MS = 5000;

private:
	int   mTimeToLive;     // ms remaining
	float mAnimationTime;  // accumulated seconds — drives the spiral arm rotation
};

#endif
