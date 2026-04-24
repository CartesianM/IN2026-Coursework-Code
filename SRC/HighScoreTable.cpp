#include "HighScoreTable.h"

bool HighScoreTable::IsHighScore(int score) const
{
	// Table not full yet — any score gets in, even a sad little 10
	if ((int)mEntries.size() < MAX_ENTRIES) return true;

	// Table is full — only beat the last place loser to get in
	return score > mEntries.back().score;
}

void HighScoreTable::AddScore(const std::string& tag, int score)
{
	ScoreEntry entry;
	// If they couldn't be bothered typing a name, give them a mystery alias
	entry.tag   = tag.empty() ? "???" : tag;
	entry.score = score;

	// Walk down the list until we find someone we can beat — then cut in line
	auto it = mEntries.begin();
	while (it != mEntries.end() && it->score >= score)
		++it;
	mEntries.insert(it, entry);

	// Ruthlessly evict whoever dropped to 11th place
	if ((int)mEntries.size() > MAX_ENTRIES)
		mEntries.resize(MAX_ENTRIES);
}
