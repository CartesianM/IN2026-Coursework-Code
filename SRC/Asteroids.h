#ifndef __ASTEROIDS_H__
#define __ASTEROIDS_H__

#include "GameUtil.h"
#include "GameSession.h"
#include "IKeyboardListener.h"
#include "IGameWorldListener.h"
#include "IScoreListener.h"
#include "ScoreKeeper.h"
#include "Player.h"
#include "IPlayerListener.h"

class GameObject;
class Spaceship;
class GUILabel;

class Asteroids : public GameSession, public IKeyboardListener, public IGameWorldListener, public IScoreListener, public IPlayerListener
{
public:
	Asteroids(int argc, char *argv[]);
	virtual ~Asteroids(void);

	virtual void Start(void);
	virtual void Stop(void);

	// Declaration of IKeyboardListener interface ////////////////////////////////

	void OnKeyPressed(uchar key, int x, int y);
	void OnKeyReleased(uchar key, int x, int y);
	void OnSpecialKeyPressed(int key, int x, int y);
	void OnSpecialKeyReleased(int key, int x, int y);

	// Declaration of IScoreListener interface //////////////////////////////////

	void OnScoreChanged(int score);

	// Declaration of the IPlayerLister interface //////////////////////////////

	void OnPlayerKilled(int lives_left);

	// Declaration of IGameWorldListener interface //////////////////////////////

	void OnWorldUpdated(GameWorld* world) {}
	void OnObjectAdded(GameWorld* world, shared_ptr<GameObject> object) {}
	void OnObjectRemoved(GameWorld* world, shared_ptr<GameObject> object);

	// Override the default implementation of ITimerListener ////////////////////
	void OnTimer(int value);

private:
	// Game state — because we need to know if the player is still chickening out on the menu
	enum GameState { STATE_MENU, STATE_PLAYING, STATE_INSTRUCTIONS };
	GameState mGameState;
	int mSelectedMenuItem;

	shared_ptr<Spaceship> mSpaceship;
	shared_ptr<GUILabel> mScoreLabel;
	shared_ptr<GUILabel> mLivesLabel;
	shared_ptr<GUILabel> mGameOverLabel;

	// Menu GUI labels
	shared_ptr<GUILabel> mMenuTitleLabel;
	shared_ptr<GUILabel> mMenuHeaderLabel;
	shared_ptr<GUILabel> mMenuOption1Label;
	shared_ptr<GUILabel> mMenuOption2Label;
	shared_ptr<GUILabel> mMenuOption3Label;
	shared_ptr<GUILabel> mMenuOption4Label;

	// Instructions screen labels — for people who actually read the manual
	shared_ptr<GUILabel> mInstrTitleLabel;
	shared_ptr<GUILabel> mInstrAimLabel;
	shared_ptr<GUILabel> mInstrControlsTitleLabel;
	shared_ptr<GUILabel> mInstrControl1Label;
	shared_ptr<GUILabel> mInstrControl2Label;
	shared_ptr<GUILabel> mInstrControl3Label;
	shared_ptr<GUILabel> mInstrControl4Label;
	shared_ptr<GUILabel> mInstrBackLabel;

	uint mLevel;
	uint mAsteroidCount;

	void ResetSpaceship();
	shared_ptr<GameObject> CreateSpaceship();
	void CreateGUI();
	void CreateAsteroids(const uint num_asteroids);
	shared_ptr<GameObject> CreateExplosion();
	void StartGame();
	void UpdateMenuHighlight();
	void ShowInstructions();              // swap to the instructions screen
	void ShowMenu();                      // escape hatch back to the menu
	void SetMenuLabelsVisible(bool visible);         // show or bury the menu all at once
	void SetInstructionLabelsVisible(bool visible);  // same trick for the instructions screen

	const static uint SHOW_GAME_OVER = 0;
	const static uint START_NEXT_LEVEL = 1;
	const static uint CREATE_NEW_PLAYER = 2;

	ScoreKeeper mScoreKeeper;
	Player mPlayer;
};

#endif