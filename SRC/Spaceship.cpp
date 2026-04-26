#include "GameUtil.h"
#include "GameWorld.h"
#include "Bullet.h"
#include "Spaceship.h"
#include "BoundingSphere.h"

using namespace std;

// PUBLIC INSTANCE CONSTRUCTORS ///////////////////////////////////////////////

/**  Default constructor. */
Spaceship::Spaceship()
	: GameObject("Spaceship"), mThrust(0), mInvulnerable(false), mInvulnTimer(0),
	  mGunLevel(0), mSpeedLevel(0), mBrakeLevel(0), mFireCooldown(0)
{
}

/** Construct a spaceship with given position, velocity, acceleration, angle, and rotation. */
Spaceship::Spaceship(GLVector3f p, GLVector3f v, GLVector3f a, GLfloat h, GLfloat r)
	: GameObject("Spaceship", p, v, a, h, r), mThrust(0), mInvulnerable(false), mInvulnTimer(0),
	  mGunLevel(0), mSpeedLevel(0), mBrakeLevel(0), mFireCooldown(0)
{
}

/** Copy constructor. */
Spaceship::Spaceship(const Spaceship& s)
	: GameObject(s), mThrust(0), mInvulnerable(false), mInvulnTimer(0),
	  mGunLevel(0), mSpeedLevel(0), mBrakeLevel(0), mFireCooldown(0)
{
}

/** Destructor. */
Spaceship::~Spaceship(void)
{
}

// PUBLIC INSTANCE METHODS ////////////////////////////////////////////////////

/** Update this spaceship. */
void Spaceship::Update(int t)
{
	// Count down the invulnerability timer; clear the flag once it expires
	if (mInvulnerable)
	{
		mInvulnTimer -= t;
		if (mInvulnTimer <= 0)
		{
			mInvulnerable = false;
			mInvulnTimer  = 0;
		}
	}

	// Count down the fire-rate cooldown so Shoot() can fire again when it reaches 0
	if (mFireCooldown > 0)
	{
		mFireCooldown -= t;
		if (mFireCooldown < 0) mFireCooldown = 0;
	}

	// Brake upgrade — apply velocity damping each frame. Higher brake level
	// means more drag, so the ship coasts to a stop faster when not thrusting.
	if (mBrakeLevel > 0)
	{
		float dt = t / 1000.0f;
		float damp = 1.0f - mBrakeLevel * 0.3f * dt;
		if (damp < 0.0f) damp = 0.0f;
		mVelocity.x *= damp;
		mVelocity.y *= damp;
	}

	// Call parent update function
	GameObject::Update(t);
}

/** Render this spaceship. */
void Spaceship::Render(void)
{
	// Draw yellow invulnerability ring before the ship sprite so it appears behind it.
	// glPushAttrib/glPopAttrib save and restore all affected GL state (lighting, texture,
	// line width, current colour) so we don't disturb the sprite rendering that follows.
	if (mInvulnerable)
	{
		glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 0.0f);   // yellow
		glLineWidth(2.5f);

		// Draw a circle in local (pre-scale) space.
		// radius 80 local units * scale 0.1 = 8 world units — visibly outside the ship sprite
		const int   segments = 32;
		const float radius   = 80.0f;
		glBegin(GL_LINE_LOOP);
		for (int i = 0; i < segments; i++)
		{
			float angle = 2.0f * (float)M_PI * i / segments;
			glVertex2f(radius * cosf(angle), radius * sinf(angle));
		}
		glEnd();

		glPopAttrib();
	}

	if (mSpaceshipShape.get() != NULL) mSpaceshipShape->Render();

	// If ship is thrusting
	if ((mThrust > 0) && (mThrusterShape.get() != NULL)) {
		mThrusterShape->Render();
	}

	GameObject::Render();
}

/** Fire the rockets. */
void Spaceship::Thrust(float t)
{
	// Speed upgrade scales the thrust force (Lv0 = 1.0x, Lv5 = 3.0x)
	float speedMult = 1.0f + mSpeedLevel * 0.4f;
	mThrust = t * speedMult;
	// Increase acceleration in the direction of ship
	mAcceleration.x = mThrust*cos(DEG2RAD*mAngle);
	mAcceleration.y = mThrust*sin(DEG2RAD*mAngle);
}

/** Set the rotation. */
void Spaceship::Rotate(float r)
{
	mRotation = r;
}

/** Shoot a bullet. */
void Spaceship::Shoot(void)
{
	// Check the world exists
	if (!mWorld) return;
	// Enforce fire-rate cooldown — shorter cooldown at higher gun levels
	// Lv0 = 250 ms between shots, Lv5 = 50 ms (20 shots/sec)
	if (mFireCooldown > 0) return;
	int cooldown = 250 - mGunLevel * 40;
	if (cooldown < 50) cooldown = 50;
	mFireCooldown = cooldown;

	// Construct a unit length vector in the direction the spaceship is headed
	GLVector3f spaceship_heading(cos(DEG2RAD*mAngle), sin(DEG2RAD*mAngle), 0);
	spaceship_heading.normalize();
	// Calculate the point at the node of the spaceship from position and heading
	GLVector3f bullet_position = mPosition + (spaceship_heading * 4);
	// Calculate how fast the bullet should travel
	float bullet_speed = 30;
	// Construct a vector for the bullet's velocity
	GLVector3f bullet_velocity = mVelocity + spaceship_heading * bullet_speed;
	// Construct a new bullet
	shared_ptr<GameObject> bullet
		(new Bullet(bullet_position, bullet_velocity, mAcceleration, mAngle, 0, 2000));
	bullet->SetBoundingShape(make_shared<BoundingSphere>(bullet->GetThisPtr(), 2.0f));
	bullet->SetShape(mBulletShape);
	// Add the new bullet to the game world
	mWorld->AddObject(bullet);

}

bool Spaceship::CollisionTest(shared_ptr<GameObject> o)
{
	// While invulnerable, ignore all collisions — the ring protects the ship
	if (mInvulnerable) return false;
	if (o->GetType() != GameObjectType("Asteroid")) return false;
	if (mBoundingShape.get() == NULL) return false;
	if (o->GetBoundingShape().get() == NULL) return false;
	return mBoundingShape->CollisionTest(o->GetBoundingShape());
}

void Spaceship::OnCollision(const GameObjectList &objects)
{
	// Invulnerability should have been blocked at the CollisionTest stage, but
	// guard here too in case of any asymmetric detection path in the engine
	if (mInvulnerable) return;
	mWorld->FlagForRemoval(GetThisPtr());
}

/** Activates invulnerability for the given duration in milliseconds. */
void Spaceship::ActivateInvulnerability(int duration_ms)
{
	mInvulnerable = true;
	mInvulnTimer  = duration_ms;
}

/** Resets all upgrade levels and the fire cooldown — called at the start of each new game. */
void Spaceship::ResetUpgrades()
{
	mGunLevel     = 0;
	mSpeedLevel   = 0;
	mBrakeLevel   = 0;
	mFireCooldown = 0;
}