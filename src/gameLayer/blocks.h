#pragma once
#include <cstdint>

struct Block
{
	enum
	{
		air = 0,
		dirt,
		grassBlock,
		stone,
		grass,
		sand,
		sandRuby,
		sandStone,
		woodPlank,
		stoneBricks,
		clay,
		woodLog,
		leaves,
		copper,
		iron,
		gold,
		copperBlock,
		ironBlock,
		goldBlock,
		bricks,
		snow,
		ice,
		rubyBlock,
		platform,
		workBench,
		glass,
		furnace,
		painting,
		sappling,
		snowBlueRuby,
		blueRubyBlock,
		door,
		jar,
		table,
		wordrobe,
		bookShelf,
		snowBricks,
		iceTable,
		iceWordrobe,
		iceBookShelf,
		icePlatform,
		sandTable,
		sandWordrobe,
		sandBookShelf,
		sandPlatform,
		woodenChest,
		iceChest,
		sandChest,
		boneChest,
		boneBricks,
		boneBench,
		boneWordrobe,
		boneBookShelf,
		bonePlatform,

		BLOCKS_COUNT,
	};
	std::uint16_t type = 0;

	// blocks the player can't walk through. the decorative overlays are passable,
	// otherwise a tuft of grass would stop you dead
	static bool isSolid(std::uint16_t type)
	{
		switch (type)
		{
			case air:
			case grass:
			case sappling:
				return false;
			default:
				return true;
		}
	}

	void sanitize()
	{
		if (type >= BLOCKS_COUNT)
		{
			type = 0;
		}
	}
};