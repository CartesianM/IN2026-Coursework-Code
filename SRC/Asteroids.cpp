#include "Asteroid.h"
#include "Asteroids.h"
#include "Animation.h"
#include "AnimationManager.h"
#include "GameUtil.h"
#include "GameWindow.h"
#include "GameWorld.h"
#include "GameDisplay.h"
#include "Spaceship.h"
#include "BoundingShape.h"
#include "BoundingSphere.h"
#include "GUILabel.h"
#include "Explosion.h"

// PUBLIC INSTANCE CONSTRUCTORS ///////////////////////////////////////////////

/** Constructor. Takes arguments from command line, just in case. */
Asteroids::Asteroids(int argc, char *argv[])
	: GameSession(argc, argv)
{
	mLevel = 0;
	mAsteroidCount = 0;
	mGameState = STATE_MENU;
	mSelectedMenuItem = 0;
}

/** Destructor. */
Asteroids::~Asteroids(void)
{
}

// PUBLIC INSTANCE METHODS ////////////////////////////////////////////////////

/** Start an asteroids game. */
void Asteroids::Start()
{
	// Create a shared pointer for the Asteroids game object - DO NOT REMOVE
	shared_ptr<Asteroids> thisPtr = shared_ptr<Asteroids>(this);

	// Add this class as a listener of the game world
	mGameWorld->AddListener(thisPtr.get());

	// Add this as a listener to the world and the keyboard
	mGameWindow->AddKeyboardListener(thisPtr);

	// Add a score keeper to the game world
	mGameWorld->AddListener(&mScoreKeeper);

	// Add this class as a listener of the score keeper
	mScoreKeeper.AddListener(thisPtr);

	// Create an ambient light to show sprite textures
	GLfloat ambient_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat diffuse_light[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse_light);
	glEnable(GL_LIGHT0);

	Animation *explosion_anim = AnimationManager::GetInstance().CreateAnimationFromFile("explosion", 64, 1024, 64, 64, "explosion_fs.png");
	Animation *asteroid1_anim = AnimationManager::GetInstance().CreateAnimationFromFile("asteroid1", 128, 8192, 128, 128, "asteroid1_fs.png");
	Animation *spaceship_anim = AnimationManager::GetInstance().CreateAnimationFromFile("spaceship", 128, 128, 128, 128, "spaceship_fs.png");

	// Pre-create the spaceship (but don't add to world yet — added when game starts)
	CreateSpaceship();
	// Create background asteroids for the menu screen
	CreateAsteroids(10);

	// Create the GUI (menu + HUD, HUD hidden initially)
	CreateGUI();

	// Add a player (watcher) to the game world
	mGameWorld->AddListener(&mPlayer);

	// Add this class as a listener of the player
	mPlayer.AddListener(thisPtr);

	// Start the game
	GameSession::Start();
}

/** Stop the current game. */
void Asteroids::Stop()
{
	// Stop the game
	GameSession::Stop();
}

// PUBLIC INSTANCE METHODS IMPLEMENTING IKeyboardListener /////////////////////

void Asteroids::OnKeyPressed(uchar key, int x, int y)
{
	if (mGameState == STATE_MENU)
	{
		// Enter or '1' selects the highlighted option
		if (key == '\r' || key == '\n')
		{
			if (mSelectedMenuItem == 0) StartGame();
			// Options 1-3 are empty placeholders for now
		}
		return;
	}

	// STATE_PLAYING controls
	switch (key)
	{
	case ' ':
		mSpaceship->Shoot();
		break;
	default:
		break;
	}
}

void Asteroids::OnKeyReleased(uchar key, int x, int y) {}

void Asteroids::OnSpecialKeyPressed(int key, int x, int y)
{
	if (mGameState == STATE_MENU)
	{
		const int NUM_OPTIONS = 4;
		switch (key)
		{
		case GLUT_KEY_UP:
			mSelectedMenuItem = (mSelectedMenuItem - 1 + NUM_OPTIONS) % NUM_OPTIONS;
			UpdateMenuHighlight();
			break;
		case GLUT_KEY_DOWN:
			mSelectedMenuItem = (mSelectedMenuItem + 1) % NUM_OPTIONS;
			UpdateMenuHighlight();
			break;
		default: break;
		}
		return;
	}

	switch (key)
	{
	// If up arrow key is pressed start applying forward thrust
	case GLUT_KEY_UP: mSpaceship->Thrust(10); break;
	// If left arrow key is pressed start rotating anti-clockwise
	case GLUT_KEY_LEFT: mSpaceship->Rotate(90); break;
	// If right arrow key is pressed start rotating clockwise
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(-90); break;
	// Default case - do nothing
	default: break;
	}
}

void Asteroids::OnSpecialKeyReleased(int key, int x, int y)
{
	if (mGameState == STATE_MENU) return;

	switch (key)
	{
	// If up arrow key is released stop applying forward thrust
	case GLUT_KEY_UP: mSpaceship->Thrust(0); break;
	// If left arrow key is released stop rotating
	case GLUT_KEY_LEFT: mSpaceship->Rotate(0); break;
	// If right arrow key is released stop rotating
	case GLUT_KEY_RIGHT: mSpaceship->Rotate(0); break;
	// Default case - do nothing
	default: break;
	}
}


// PUBLIC INSTANCE METHODS IMPLEMENTING IGameWorldListener ////////////////////

void Asteroids::OnObjectRemoved(GameWorld* world, shared_ptr<GameObject> object)
{
	if (object->GetType() == GameObjectType("Asteroid"))
	{
		shared_ptr<GameObject> explosion = CreateExplosion();
		explosion->SetPosition(object->GetPosition());
		explosion->SetRotation(object->GetRotation());
		mGameWorld->AddObject(explosion);
		mAsteroidCount--;
		if (mAsteroidCount <= 0) 
		{ 
			SetTimer(500, START_NEXT_LEVEL); 
		}
	}
}

// PUBLIC INSTANCE METHODS IMPLEMENTING ITimerListener ////////////////////////

void Asteroids::OnTimer(int value)
{
	if (value == CREATE_NEW_PLAYER)
	{
		mSpaceship->Reset();
		mGameWorld->AddObject(mSpaceship);
	}

	if (value == START_NEXT_LEVEL)
	{
		mLevel++;
		int num_asteroids = 10 + 2 * mLevel;
		CreateAsteroids(num_asteroids);
	}

	if (value == SHOW_GAME_OVER)
	{
		mGameOverLabel->SetVisible(true);
	}

}

// PROTECTED INSTANCE METHODS /////////////////////////////////////////////////
shared_ptr<GameObject> Asteroids::CreateSpaceship()
{
	// Create a raw pointer to a spaceship that can be converted to
	// shared_ptrs of different types because GameWorld implements IRefCount
	mSpaceship = make_shared<Spaceship>();
	mSpaceship->SetBoundingShape(make_shared<BoundingSphere>(mSpaceship->GetThisPtr(), 4.0f));
	shared_ptr<Shape> bullet_shape = make_shared<Shape>("bullet.shape");
	mSpaceship->SetBulletShape(bullet_shape);
	Animation *anim_ptr = AnimationManager::GetInstance().GetAnimationByName("spaceship");
	shared_ptr<Sprite> spaceship_sprite =
		make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	mSpaceship->SetSprite(spaceship_sprite);
	mSpaceship->SetScale(0.1f);
	// Reset spaceship back to centre of the world
	mSpaceship->Reset();
	// Return the spaceship so it can be added to the world
	return mSpaceship;

}

void Asteroids::CreateAsteroids(const uint num_asteroids)
{
	mAsteroidCount = num_asteroids;
	for (uint i = 0; i < num_asteroids; i++)
	{
		Animation *anim_ptr = AnimationManager::GetInstance().GetAnimationByName("asteroid1");
		shared_ptr<Sprite> asteroid_sprite
			= make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
		asteroid_sprite->SetLoopAnimation(true);
		shared_ptr<GameObject> asteroid = make_shared<Asteroid>();
		asteroid->SetBoundingShape(make_shared<BoundingSphere>(asteroid->GetThisPtr(), 10.0f));
		asteroid->SetSprite(asteroid_sprite);
		asteroid->SetScale(0.2f);
		mGameWorld->AddObject(asteroid);
	}
}

void Asteroids::CreateGUI()
{
	// Add a (transparent) border around the edge of the game display
	mGameDisplay->GetContainer()->SetBorder(GLVector2i(10, 10));

	// --- HUD labels (hidden until game starts) ---

	mScoreLabel = make_shared<GUILabel>("Score: 0");
	mScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_TOP);
	mScoreLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mScoreLabel), GLVector2f(0.0f, 1.0f));

	mLivesLabel = make_shared<GUILabel>("Lives: 3");
	mLivesLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_BOTTOM);
	mLivesLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mLivesLabel), GLVector2f(0.0f, 0.0f));

	mGameOverLabel = make_shared<GUILabel>("GAME OVER");
	mGameOverLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mGameOverLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameOverLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mGameOverLabel), GLVector2f(0.5f, 0.5f));

	// --- Menu labels ---

	mMenuTitleLabel = make_shared<GUILabel>("ASTEROIDS");
	mMenuTitleLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuTitleLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuTitleLabel->SetColor(GLVector3f(1.0f, 0.8f, 0.0f));  // gold title
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuTitleLabel), GLVector2f(0.5f, 0.72f));

	mMenuHeaderLabel = make_shared<GUILabel>("--- MENU ---");
	mMenuHeaderLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuHeaderLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuHeaderLabel->SetColor(GLVector3f(0.8f, 0.8f, 0.8f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuHeaderLabel), GLVector2f(0.5f, 0.60f));

	mMenuOption1Label = make_shared<GUILabel>("Start Game");
	mMenuOption1Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption1Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption1Label->SetColor(GLVector3f(1.0f, 1.0f, 0.0f));  // selected = yellow
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption1Label), GLVector2f(0.5f, 0.52f));

	mMenuOption2Label = make_shared<GUILabel>("Options");
	mMenuOption2Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption2Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption2Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption2Label), GLVector2f(0.5f, 0.45f));

	mMenuOption3Label = make_shared<GUILabel>("High Scores");
	mMenuOption3Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption3Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption3Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption3Label), GLVector2f(0.5f, 0.38f));

	mMenuOption4Label = make_shared<GUILabel>("Quit");
	mMenuOption4Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption4Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption4Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption4Label), GLVector2f(0.5f, 0.31f));
}

void Asteroids::OnScoreChanged(int score)
{
	// Format the score message using an string-based stream
	std::ostringstream msg_stream;
	msg_stream << "Score: " << score;
	// Get the score message as a string
	std::string score_msg = msg_stream.str();
	mScoreLabel->SetText(score_msg);
}

void Asteroids::OnPlayerKilled(int lives_left)
{
	shared_ptr<GameObject> explosion = CreateExplosion();
	explosion->SetPosition(mSpaceship->GetPosition());
	explosion->SetRotation(mSpaceship->GetRotation());
	mGameWorld->AddObject(explosion);

	// Format the lives left message using an string-based stream
	std::ostringstream msg_stream;
	msg_stream << "Lives: " << lives_left;
	// Get the lives left message as a string
	std::string lives_msg = msg_stream.str();
	mLivesLabel->SetText(lives_msg);

	if (lives_left > 0) 
	{ 
		SetTimer(1000, CREATE_NEW_PLAYER); 
	}
	else
	{
		SetTimer(500, SHOW_GAME_OVER);
	}
}

shared_ptr<GameObject> Asteroids::CreateExplosion()
{
	Animation *anim_ptr = AnimationManager::GetInstance().GetAnimationByName("explosion");
	shared_ptr<Sprite> explosion_sprite =
		make_shared<Sprite>(anim_ptr->GetWidth(), anim_ptr->GetHeight(), anim_ptr);
	explosion_sprite->SetLoopAnimation(false);
	shared_ptr<GameObject> explosion = make_shared<Explosion>();
	explosion->SetSprite(explosion_sprite);
	explosion->Reset();
	return explosion;
}

void Asteroids::StartGame()
{
	mGameState = STATE_PLAYING;

	// Hide all menu labels
	mMenuTitleLabel->SetVisible(false);
	mMenuHeaderLabel->SetVisible(false);
	mMenuOption1Label->SetVisible(false);
	mMenuOption2Label->SetVisible(false);
	mMenuOption3Label->SetVisible(false);
	mMenuOption4Label->SetVisible(false);

	// Show HUD
	mScoreLabel->SetVisible(true);
	mLivesLabel->SetVisible(true);

	// Spawn the spaceship into the world
	mSpaceship->Reset();
	mGameWorld->AddObject(mSpaceship);
}

void Asteroids::UpdateMenuHighlight()
{
	// All options default to white; selected option turns yellow
	GLVector3f white(1.0f, 1.0f, 1.0f);
	GLVector3f selected(1.0f, 1.0f, 0.0f);

	mMenuOption1Label->SetColor(mSelectedMenuItem == 0 ? selected : white);
	mMenuOption2Label->SetColor(mSelectedMenuItem == 1 ? selected : white);
	mMenuOption3Label->SetColor(mSelectedMenuItem == 2 ? selected : white);
	mMenuOption4Label->SetColor(mSelectedMenuItem == 3 ? selected : white);
}




