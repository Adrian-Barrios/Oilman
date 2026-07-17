#pragma once
#include <blocks.h>
#include <vector>

struct GameMap
{
	int w = 0;
	int h = 0;

	std::vector<Block> mapData; // A map is simply a collection of blocks.

	void create(int w, int h);
	Block& getBlocUnsafe(int x, int y);
	Block* getBlockSafe(int x, int y); // Will check we didn't go outside the map boundaries.
};