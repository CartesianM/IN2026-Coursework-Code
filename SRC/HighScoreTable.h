#ifndef __HIGHSCORETABLE_H__
#define __HIGHSCORETABLE_H__

#include "GameUtil.h"
#include <vector>
#include <iomanip>

// One row in the leaderboard — a name and the carnage they caused
struct ScoreEntry
{
	std::string tag;
	int score;
};

class HighScoreTable
{
public:
	// Top 10 only — if you're not good enough, the table doesn't want to know you
	static const int MAX_ENTRIES = 10;

	// Returns true if this score earns a spot on the board
	bool IsHighScore(int score) const;

	// Inserts the score in the right place and boots the 11th loser out if needed
	void AddScore(const std::string& tag, int score);

	// Read-only view of the current leaderboard, sorted best-first
	const std::vector<ScoreEntry>& GetEntries() const { return mEntries; }

	int GetSize() const { return (int)mEntries.size(); }

private:
	std::vector<ScoreEntry> mEntries;
};

#endif
