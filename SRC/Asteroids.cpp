#include "Asteroid.h"
#include "Asteroids.h"
#include "HighScoreTable.h"
#include <iomanip>
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
	mCurrentScore = 0;
	mIsNewHighScore = false;
	// Default to Easy so a new player isn't punished before choosing a mode
	mDifficulty = DIFF_EASY;
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
		if (key == '\r' || key == '\n')
		{
			if      (mSelectedMenuItem == 0) StartGame();
			else if (mSelectedMenuItem == 1) CycleDifficulty();   // cycles Easy -> Medium -> Hard -> Easy
			else if (mSelectedMenuItem == 2) ShowInstructions();
			else if (mSelectedMenuItem == 3) ShowHighScores();
		}
		return;
	}

	if (mGameState == STATE_INSTRUCTIONS)
	{
		// Any of these keys gets you out — we're not holding anyone hostage
		if (key == '\r' || key == '\n' || key == 27 /* ESC */ || key == '\b')
			ShowMenu();
		return;
	}

	if (mGameState == STATE_HIGH_SCORES)
	{
		// Seen enough? Get out of here.
		if (key == '\r' || key == '\n' || key == 27 /* ESC */)
			ShowMenu();
		return;
	}

	if (mGameState == STATE_GAME_OVER)
	{
		if (mIsNewHighScore)
		{
			if (key == '\r' || key == '\n')
			{
				// They've had their moment of glory — save it and head home
				SubmitScore();
				ShowMenu();
			}
			else if (key == '\b')
			{
				// Classic backspace — delete the last character, no questions asked
				if (!mCurrentTagInput.empty())
				{
					mCurrentTagInput.pop_back();
					UpdateTagInputLabel();
				}
			}
			else if (key >= 32 && key <= 126 && (int)mCurrentTagInput.size() < 12)
			{
				// Printable character and still under the 12-char limit — add it
				mCurrentTagInput += (char)key;
				UpdateTagInputLabel();
			}
		}
		else
		{
			// No high score — nothing to type, just let them escape
			if (key == '\r' || key == '\n' || key == 27 /* ESC */)
				ShowMenu();
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

	// Arrow keys only make sense in the menu and during gameplay — block everything else
	if (mGameState == STATE_INSTRUCTIONS ||
		mGameState == STATE_HIGH_SCORES  ||
		mGameState == STATE_GAME_OVER) return;

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
	// Only the spaceship needs to hear about key releases — ignore everything else
	if (mGameState != STATE_PLAYING) return;

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
		// Hand off to ShowGameOver — it decides what to show based on the score
		ShowGameOver();
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

	// Difficulty option — shows current selection; press Enter to cycle through modes
	mMenuOption2Label = make_shared<GUILabel>("Difficulty: Easy");
	mMenuOption2Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption2Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption2Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption2Label), GLVector2f(0.5f, 0.45f));

	// Instructions option — actually works, unlike its neighbours
	mMenuOption3Label = make_shared<GUILabel>("Instructions");
	mMenuOption3Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption3Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption3Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption3Label), GLVector2f(0.5f, 0.38f));

	// High Scores — a monument to future ambition, currently just decoration
	mMenuOption4Label = make_shared<GUILabel>("High Scores");
	mMenuOption4Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mMenuOption4Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mMenuOption4Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mMenuOption4Label), GLVector2f(0.5f, 0.31f));

	// --- Instructions screen labels — all hidden until the player is curious enough to ask ---

	// Same gold as the main title — consistency is a virtue
	mInstrTitleLabel = make_shared<GUILabel>("INSTRUCTIONS");
	mInstrTitleLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrTitleLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrTitleLabel->SetColor(GLVector3f(1.0f, 0.8f, 0.0f));
	mInstrTitleLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrTitleLabel), GLVector2f(0.5f, 0.85f));

	// The entire point of the game, in one sentence
	mInstrAimLabel = make_shared<GUILabel>("AIM: Survive and destroy the asteroids");
	mInstrAimLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrAimLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrAimLabel->SetColor(GLVector3f(0.9f, 0.9f, 0.9f));
	mInstrAimLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrAimLabel), GLVector2f(0.5f, 0.72f));

	// Slightly dimmed so it reads as a header rather than a control entry
	mInstrControlsTitleLabel = make_shared<GUILabel>("CONTROLS:");
	mInstrControlsTitleLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrControlsTitleLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrControlsTitleLabel->SetColor(GLVector3f(0.8f, 0.8f, 0.8f));
	mInstrControlsTitleLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrControlsTitleLabel), GLVector2f(0.5f, 0.62f));

	mInstrControl1Label = make_shared<GUILabel>("UP ARROW    - Thrust forward");
	mInstrControl1Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrControl1Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrControl1Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mInstrControl1Label->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrControl1Label), GLVector2f(0.5f, 0.54f));

	mInstrControl2Label = make_shared<GUILabel>("LEFT ARROW  - Rotate left");
	mInstrControl2Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrControl2Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrControl2Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mInstrControl2Label->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrControl2Label), GLVector2f(0.5f, 0.47f));

	mInstrControl3Label = make_shared<GUILabel>("RIGHT ARROW - Rotate right");
	mInstrControl3Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrControl3Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrControl3Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mInstrControl3Label->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrControl3Label), GLVector2f(0.5f, 0.40f));

	mInstrControl4Label = make_shared<GUILabel>("SPACE       - Shoot");
	mInstrControl4Label->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrControl4Label->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrControl4Label->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
	mInstrControl4Label->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrControl4Label), GLVector2f(0.5f, 0.33f));

	// Dimmed grey — it's a whisper, not a shout. You've already read the manual.
	mInstrBackLabel = make_shared<GUILabel>("Press ENTER or ESC to return");
	mInstrBackLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mInstrBackLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mInstrBackLabel->SetColor(GLVector3f(0.6f, 0.6f, 0.6f));
	mInstrBackLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mInstrBackLabel), GLVector2f(0.5f, 0.18f));

	// --- Game over screen labels — all hidden until the bitter end ---

	// Shown only when the player actually earned a spot on the board
	mNewHighScoreLabel = make_shared<GUILabel>("NEW HIGH SCORE!");
	mNewHighScoreLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mNewHighScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mNewHighScoreLabel->SetColor(GLVector3f(1.0f, 1.0f, 0.0f));  // bright yellow — they earned it
	mNewHighScoreLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mNewHighScoreLabel), GLVector2f(0.5f, 0.65f));

	mEnterTagLabel = make_shared<GUILabel>("Enter your gamer tag:");
	mEnterTagLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mEnterTagLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mEnterTagLabel->SetColor(GLVector3f(0.9f, 0.9f, 0.9f));
	mEnterTagLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mEnterTagLabel), GLVector2f(0.5f, 0.42f));

	// This one updates live as the player types — starts as an empty cursor
	mTagInputLabel = make_shared<GUILabel>("> _");
	mTagInputLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mTagInputLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mTagInputLabel->SetColor(GLVector3f(0.4f, 1.0f, 0.4f));  // green, like a terminal
	mTagInputLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mTagInputLabel), GLVector2f(0.5f, 0.35f));

	mTagConfirmLabel = make_shared<GUILabel>("ENTER to confirm  |  BACKSPACE to delete");
	mTagConfirmLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mTagConfirmLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mTagConfirmLabel->SetColor(GLVector3f(0.6f, 0.6f, 0.6f));
	mTagConfirmLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mTagConfirmLabel), GLVector2f(0.5f, 0.26f));

	// Shown instead of the tag prompt when the score doesn't make the cut
	mNotHighScoreLabel = make_shared<GUILabel>("Score not high enough for the leaderboard");
	mNotHighScoreLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mNotHighScoreLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mNotHighScoreLabel->SetColor(GLVector3f(0.8f, 0.4f, 0.4f));  // muted red — sorry mate
	mNotHighScoreLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mNotHighScoreLabel), GLVector2f(0.5f, 0.38f));

	// Shared "get out" prompt for the non-high-score path
	mGameOverContinueLabel = make_shared<GUILabel>("Press ENTER to return to menu");
	mGameOverContinueLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mGameOverContinueLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mGameOverContinueLabel->SetColor(GLVector3f(0.6f, 0.6f, 0.6f));
	mGameOverContinueLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mGameOverContinueLabel), GLVector2f(0.5f, 0.28f));

	// --- High scores screen labels ---

	mHSTitleLabel = make_shared<GUILabel>("HIGH SCORES");
	mHSTitleLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mHSTitleLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mHSTitleLabel->SetColor(GLVector3f(1.0f, 0.8f, 0.0f));  // same gold as the main title
	mHSTitleLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mHSTitleLabel), GLVector2f(0.5f, 0.92f));

	// Column headers so the table doesn't look like a random list of numbers
	mHSColumnHeader = make_shared<GUILabel>("RANK  TAG              SCORE");
	mHSColumnHeader->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mHSColumnHeader->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mHSColumnHeader->SetColor(GLVector3f(0.7f, 0.7f, 0.7f));
	mHSColumnHeader->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mHSColumnHeader), GLVector2f(0.5f, 0.84f));

	// Ten entry slots — created now, filled by UpdateHighScoreLabels() at display time
	// Evenly spaced from y=0.77 down to y=0.14 (0.07 gap between each)
	for (int i = 0; i < 10; i++)
	{
		mHSEntryLabels[i] = make_shared<GUILabel>("---");
		mHSEntryLabels[i]->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
		mHSEntryLabels[i]->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
		mHSEntryLabels[i]->SetColor(GLVector3f(1.0f, 1.0f, 1.0f));
		mHSEntryLabels[i]->SetVisible(false);
		mGameDisplay->GetContainer()->AddComponent(
			static_pointer_cast<GUIComponent>(mHSEntryLabels[i]),
			GLVector2f(0.5f, 0.77f - i * 0.07f));
	}

	mHSBackLabel = make_shared<GUILabel>("Press ENTER or ESC to return");
	mHSBackLabel->SetHorizontalAlignment(GUIComponent::GUI_HALIGN_CENTER);
	mHSBackLabel->SetVerticalAlignment(GUIComponent::GUI_VALIGN_MIDDLE);
	mHSBackLabel->SetColor(GLVector3f(0.6f, 0.6f, 0.6f));
	mHSBackLabel->SetVisible(false);
	mGameDisplay->GetContainer()->AddComponent(
		static_pointer_cast<GUIComponent>(mHSBackLabel), GLVector2f(0.5f, 0.05f));
}

void Asteroids::OnScoreChanged(int score)
{
	// Keep a local copy so we can check it against the table when the game ends
	mCurrentScore = score;

	std::ostringstream msg_stream;
	msg_stream << "Score: " << score;
	mScoreLabel->SetText(msg_stream.str());
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

	// Wipe the slate — score and tag carry over from nothing
	mCurrentScore = 0;
	mCurrentTagInput = "";

	// Nuke every overlay in one go — no ghost labels haunting the gameplay
	SetMenuLabelsVisible(false);
	SetInstructionLabelsVisible(false);
	SetGameOverLabelsVisible(false);
	SetHighScoreScreenLabelsVisible(false);
	mGameOverLabel->SetVisible(false);

	// Reset the player's life pool to however many the chosen difficulty grants.
	// Without this a second playthrough would start with 0 lives.
	int startingLives = GetLivesForDifficulty();
	mPlayer.Reset(startingLives);

	// Sync the HUD lives label before it becomes visible
	std::ostringstream livesStream;
	livesStream << "Lives: " << startingLives;
	mLivesLabel->SetText(livesStream.str());

	// Time to actually play — show the HUD
	mScoreLabel->SetVisible(true);
	mLivesLabel->SetVisible(true);

	// The ship has been waiting patiently in the wings, now it's showtime
	mSpaceship->Reset();
	mGameWorld->AddObject(mSpaceship);
}

void Asteroids::ShowInstructions()
{
	// Player asked for instructions — respect that, hide the menu and show the goods
	mGameState = STATE_INSTRUCTIONS;
	SetMenuLabelsVisible(false);
	SetInstructionLabelsVisible(true);
}

void Asteroids::ShowMenu()
{
	// Back to safety — hide whatever screen we came from and resurrect the menu
	mGameState = STATE_MENU;
	SetInstructionLabelsVisible(false);
	SetHighScoreScreenLabelsVisible(false);
	SetGameOverLabelsVisible(false);
	mGameOverLabel->SetVisible(false);

	// HUD has no business being on the menu
	mScoreLabel->SetVisible(false);
	mLivesLabel->SetVisible(false);

	SetMenuLabelsVisible(true);
}

void Asteroids::SetMenuLabelsVisible(bool visible)
{
	// Six labels to wrangle — toggling them all one by one would be tedious
	mMenuTitleLabel->SetVisible(visible);
	mMenuHeaderLabel->SetVisible(visible);
	mMenuOption1Label->SetVisible(visible);
	mMenuOption2Label->SetVisible(visible);
	mMenuOption3Label->SetVisible(visible);
	mMenuOption4Label->SetVisible(visible);
}

void Asteroids::SetInstructionLabelsVisible(bool visible)
{
	// Eight labels, one switch — much cleaner than copy-pasting SetVisible everywhere
	mInstrTitleLabel->SetVisible(visible);
	mInstrAimLabel->SetVisible(visible);
	mInstrControlsTitleLabel->SetVisible(visible);
	mInstrControl1Label->SetVisible(visible);
	mInstrControl2Label->SetVisible(visible);
	mInstrControl3Label->SetVisible(visible);
	mInstrControl4Label->SetVisible(visible);
	mInstrBackLabel->SetVisible(visible);
}

void Asteroids::ShowHighScores()
{
	// Refresh label text from the actual table data before revealing the screen
	UpdateHighScoreLabels();

	mGameState = STATE_HIGH_SCORES;
	SetMenuLabelsVisible(false);
	SetHighScoreScreenLabelsVisible(true);
}

void Asteroids::ShowGameOver()
{
	mGameState = STATE_GAME_OVER;

	// Hide HUD — score and lives have served their purpose
	mScoreLabel->SetVisible(false);
	mLivesLabel->SetVisible(false);

	// GAME OVER label stays on screen regardless of high score status
	mGameOverLabel->SetVisible(true);

	mIsNewHighScore = mHighScoreTable.IsHighScore(mCurrentScore);
	mCurrentTagInput = "";  // clear any leftover input from a previous run

	if (mIsNewHighScore)
	{
		// Roll out the red carpet — show the tag entry prompt
		UpdateTagInputLabel();
		mNewHighScoreLabel->SetVisible(true);
		mEnterTagLabel->SetVisible(true);
		mTagInputLabel->SetVisible(true);
		mTagConfirmLabel->SetVisible(true);
	}
	else
	{
		// Tough luck — show the consolation labels
		mNotHighScoreLabel->SetVisible(true);
		mGameOverContinueLabel->SetVisible(true);
	}
}

void Asteroids::SubmitScore()
{
	// Hand the tag and score over to the table — it'll sort itself out
	mHighScoreTable.AddScore(mCurrentTagInput, mCurrentScore);
}

void Asteroids::UpdateTagInputLabel()
{
	// Show "> " + whatever they've typed so far + a blinking-cursor underscore
	mTagInputLabel->SetText("> " + mCurrentTagInput + "_");
}

void Asteroids::UpdateHighScoreLabels()
{
	const std::vector<ScoreEntry>& entries = mHighScoreTable.GetEntries();

	for (int i = 0; i < 10; i++)
	{
		if (i < (int)entries.size())
		{
			// Format: "1.  PlayerName        1500"
			// setw pads the tag to 16 chars so the score column stays aligned
			std::ostringstream ss;
			ss << std::left << (i + 1) << ".   "
			   << std::setw(16) << entries[i].tag
			   << entries[i].score;
			mHSEntryLabels[i]->SetText(ss.str());
		}
		else
		{
			// Empty slot — nothing to brag about here yet
			mHSEntryLabels[i]->SetText("---");
		}
	}
}

void Asteroids::SetGameOverLabelsVisible(bool visible)
{
	// All game over prompt labels in one toggle — both the HS and non-HS variants
	mNewHighScoreLabel->SetVisible(visible);
	mEnterTagLabel->SetVisible(visible);
	mTagInputLabel->SetVisible(visible);
	mTagConfirmLabel->SetVisible(visible);
	mNotHighScoreLabel->SetVisible(visible);
	mGameOverContinueLabel->SetVisible(visible);
}

void Asteroids::SetHighScoreScreenLabelsVisible(bool visible)
{
	// Title, column header, all 10 entries, and the back hint
	mHSTitleLabel->SetVisible(visible);
	mHSColumnHeader->SetVisible(visible);
	for (int i = 0; i < 10; i++)
		mHSEntryLabels[i]->SetVisible(visible);
	mHSBackLabel->SetVisible(visible);
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

// Returns the starting life count for the currently selected difficulty.
// Easy = 6 lives (forgiving), Medium = 4 (balanced), Hard = 2 (unforgiving).
int Asteroids::GetLivesForDifficulty() const
{
	switch (mDifficulty)
	{
	case DIFF_EASY:   return 6;
	case DIFF_MEDIUM: return 4;
	case DIFF_HARD:   return 2;
	default:          return 4;
	}
}

// Cycles the difficulty one step forward (Easy -> Medium -> Hard -> Easy)
// and refreshes the menu label so the player always sees their current choice.
void Asteroids::CycleDifficulty()
{
	switch (mDifficulty)
	{
	case DIFF_EASY:   mDifficulty = DIFF_MEDIUM; break;
	case DIFF_MEDIUM: mDifficulty = DIFF_HARD;   break;
	case DIFF_HARD:   mDifficulty = DIFF_EASY;   break;
	}

	// Map the enum back to a display string for the menu label
	const char* names[] = { "Easy", "Medium", "Hard" };
	mMenuOption2Label->SetText(std::string("Difficulty: ") + names[(int)mDifficulty]);

	// Re-apply highlight colours so the label colour stays correct after the text change
	UpdateMenuHighlight();
}




