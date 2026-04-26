#ifndef __SCOREKEEPER_H__
#define __SCOREKEEPER_H__

#include "GameUtil.h"

#include "GameObject.h"
#include "GameObjectType.h"
#include "IScoreListener.h"
#include "IGameWorldListener.h"

class ScoreKeeper : public IGameWorldListener
{
public:
	ScoreKeeper() { mScore = 0; }
	virtual ~ScoreKeeper() {}

	// Resets the score to zero and notifies listeners — called at the start of each new game
	void Reset()
	{
		mScore = 0;
		FireScoreChanged();
	}

	// Subtracts points for a player action; floors at zero to prevent negative scores
	void Deduct(int amount)
	{
		mScore -= amount;
		if (mScore < 0) mScore = 0;
		FireScoreChanged();
	}

	// Adds points directly — called by game logic when it knows a valid kill occurred
	void AddScore(int amount)
	{
		mScore += amount;
		FireScoreChanged();
	}

	void OnWorldUpdated(GameWorld* world) {}
	void OnObjectAdded(GameWorld* world, shared_ptr<GameObject> object) {}
	void OnObjectRemoved(GameWorld* world, shared_ptr<GameObject> object) {}

	void AddListener(shared_ptr<IScoreListener> listener)
	{
		mListeners.push_back(listener);
	}

	void FireScoreChanged()
	{
		// Send message to all listeners
		for (ScoreListenerList::iterator lit = mListeners.begin(); lit != mListeners.end(); ++lit) {
			(*lit)->OnScoreChanged(mScore);
		}
	}

private:
	int mScore;

	typedef std::list< shared_ptr<IScoreListener> > ScoreListenerList;

	ScoreListenerList mListeners;
};

#endif
