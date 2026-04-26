#include "GameUtil.h"
#include "GameWorld.h"
#include "BlackHole.h"
#include "GameObjectType.h"

// Tunable physics for the black hole — chosen to feel powerful but bounded
namespace {
	const float DESTROY_RADIUS  = 14.0f;   // asteroids inside this disappear
	const float PULL_RADIUS     = 90.0f;   // asteroids inside this get pulled toward the hole
	const float PULL_STRENGTH   = 80.0f;   // accel applied per second toward the hole

	// Visual radii — drawn in the BlackHole's local space (mScale = 1, so 1 unit = 1 world unit)
	const float HORIZON_RADIUS  = 8.0f;    // solid black centre
	const float DISK_RADIUS     = 13.0f;   // bright orange accretion ring
	const float HALO_RADIUS     = 22.0f;   // outermost faint glow
}

BlackHole::BlackHole()
	: GameObject("BlackHole"), mTimeToLive(LIFESPAN_MS), mAnimationTime(0.0f)
{
}

BlackHole::~BlackHole() {}

void BlackHole::Update(int t)
{
	// Drive the spiral animation (used by Render to rotate the arms)
	mAnimationTime += t / 1000.0f;

	// Lifespan countdown — flag for removal once it expires
	mTimeToLive -= t;
	if (mTimeToLive <= 0)
	{
		if (mWorld) mWorld->FlagForRemoval(GetThisPtr());
		return;
	}

	// Each frame: walk every game object, pull asteroids inwards, and consume
	// any that have crossed the destroy radius (event horizon)
	if (mWorld)
	{
		const float dt              = t / 1000.0f;
		const float destroyRadiusSq = DESTROY_RADIUS * DESTROY_RADIUS;
		const float pullRadiusSq    = PULL_RADIUS    * PULL_RADIUS;

		const GameObjectList& objs = mWorld->GetGameObjects();
		for (GameObjectList::const_iterator it = objs.begin(); it != objs.end(); ++it)
		{
			shared_ptr<GameObject> obj = *it;
			if (obj->GetType() != GameObjectType("Asteroid")) continue;

			GLVector3f delta = mPosition - obj->GetPosition();
			float dsqr = delta.lengthSqr();

			if (dsqr <= destroyRadiusSq)
			{
				// Crossed the event horizon — gone. mKilledByBullet stays false
				// so Asteroids::OnObjectRemoved correctly awards no points.
				mWorld->FlagForRemoval(obj);
				continue;
			}

			if (dsqr < pullRadiusSq && dsqr > 0.0f)
			{
				// Apply acceleration toward the hole; closer asteroids feel pull longer
				float d = sqrtf(dsqr);
				GLVector3f dir = delta / d;
				obj->SetVelocity(obj->GetVelocity() + dir * (PULL_STRENGTH * dt));
			}
		}
	}

	// No call to GameObject::Update — the black hole is stationary and has no
	// sprite/wrapping needs. Skipping it also avoids any unwanted side effects.
}

void BlackHole::Render(void)
{
	// Save and restore every GL state we touch so the rest of the scene
	// (textured sprites, lighting on the spaceship, line widths) is unaffected.
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// 1. Outermost faint halo — barely visible orange glow
	glColor4f(1.0f, 0.45f, 0.05f, 0.18f);
	glLineWidth(1.0f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 64; i++)
	{
		float a = 2.0f * (float)M_PI * i / 64.0f;
		glVertex2f(HALO_RADIUS * cosf(a), HALO_RADIUS * sinf(a));
	}
	glEnd();

	// 2. Bright accretion disk ring (the visible "edge" of the swirl)
	glColor3f(1.0f, 0.55f, 0.0f);
	glLineWidth(2.5f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 64; i++)
	{
		float a = 2.0f * (float)M_PI * i / 64.0f;
		glVertex2f(DISK_RADIUS * cosf(a), DISK_RADIUS * sinf(a));
	}
	glEnd();

	// 3. Hot inner ring (yellow-white) just outside the event horizon
	glColor3f(1.0f, 0.95f, 0.4f);
	glLineWidth(1.8f);
	glBegin(GL_LINE_LOOP);
	for (int i = 0; i < 48; i++)
	{
		float a = 2.0f * (float)M_PI * i / 48.0f;
		glVertex2f((HORIZON_RADIUS + 1.0f) * cosf(a), (HORIZON_RADIUS + 1.0f) * sinf(a));
	}
	glEnd();

	// 4. Animated spiral arms — four arms rotating with mAnimationTime, fading
	//    as they trail outward from the event horizon. This is the "sucking" cue.
	const int ARMS  = 4;
	const int STEPS = 28;
	glLineWidth(1.5f);
	for (int arm = 0; arm < ARMS; arm++)
	{
		// Each arm starts evenly spaced around the disk; mAnimationTime spins them
		float armBase = mAnimationTime * 2.5f + arm * (2.0f * (float)M_PI / ARMS);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < STEPS; i++)
		{
			float r     = HORIZON_RADIUS + i * 0.55f;   // grow outward
			float angle = armBase + i * 0.18f;          // logarithmic-ish spiral
			float alpha = 1.0f - (float)i / (float)STEPS;
			glColor4f(1.0f, 0.6f + i * 0.012f, 0.1f, alpha);
			glVertex2f(r * cosf(angle), r * sinf(angle));
		}
		glEnd();
	}

	// 5. Solid black event horizon, drawn LAST so it covers the inner ends of
	//    the spiral arms, giving the impression they're being sucked into a pit
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(0.0f, 0.0f);
	for (int i = 0; i <= 32; i++)
	{
		float a = 2.0f * (float)M_PI * i / 32.0f;
		glVertex2f(HORIZON_RADIUS * cosf(a), HORIZON_RADIUS * sinf(a));
	}
	glEnd();

	glPopAttrib();
}
