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
#include "HighScoreTable.h"

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
	// Game state — covers every screen the player might find themselves on
	enum GameState { STATE_MENU, STATE_PLAYING, STATE_INSTRUCTIONS, STATE_HIGH_SCORES, STATE_GAME_OVER };
	GameState mGameState;
	int mSelectedMenuItem;

	// Difficulty controls starting lives: Easy = 6, Medium = 4, Hard = 2
	enum Difficulty { DIFF_EASY, DIFF_MEDIUM, DIFF_HARD };
	Difficulty mDifficulty;

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

	// Tracks the player's score mid-game so we know if it's a high score at the end
	int mCurrentScore;
	// Whether the last game ended with a leaderboard-worthy score
	bool mIsNewHighScore;
	// What the player has typed so far for their gamer tag
	std::string mCurrentTagInput;

	// The leaderboard itself — survives for the whole session
	HighScoreTable mHighScoreTable;

	// Game over screen labels — shown after the last life is lost
	shared_ptr<GUILabel> mNewHighScoreLabel;   // "NEW HIGH SCORE!" flash
	shared_ptr<GUILabel> mEnterTagLabel;        // prompt to type a name
	shared_ptr<GUILabel> mTagInputLabel;        // live display of what they're typing
	shared_ptr<GUILabel> mTagConfirmLabel;      // "ENTER to confirm" hint
	shared_ptr<GUILabel> mNotHighScoreLabel;    // consolation message for the rest
	shared_ptr<GUILabel> mGameOverContinueLabel; // "ENTER to return to menu"

	// High scores screen labels
	shared_ptr<GUILabel> mHSTitleLabel;
	shared_ptr<GUILabel> mHSColumnHeader;
	shared_ptr<GUILabel> mHSEntryLabels[10];  // one label per leaderboard slot
	shared_ptr<GUILabel> mHSBackLabel;

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
	void ShowHighScores();                // pull up the leaderboard
	void ShowGameOver();                  // end of the road — check score, show prompts
	void SubmitScore();                   // lock in the tag and save to the table
	void UpdateTagInputLabel();           // refresh the "> typing..." label as keys are pressed
	void UpdateHighScoreLabels();         // rewrite the 10 entry labels from current table data
	void SetMenuLabelsVisible(bool visible);             // show or bury the menu all at once
	void SetInstructionLabelsVisible(bool visible);      // same trick for the instructions screen
	void SetGameOverLabelsVisible(bool visible);          // toggle all game over prompt labels
	void SetHighScoreScreenLabelsVisible(bool visible);   // toggle all high score screen labels

	// Cycles the selected difficulty and refreshes the menu label
	void CycleDifficulty();
	// Returns the starting life count for the currently selected difficulty
	int GetLivesForDifficulty() const;

	const static uint SHOW_GAME_OVER = 0;
	const static uint START_NEXT_LEVEL = 1;
	const static uint CREATE_NEW_PLAYER = 2;

	ScoreKeeper mScoreKeeper;
	Player mPlayer;
};

#endif