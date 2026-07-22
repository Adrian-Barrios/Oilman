#pragma once
#include <raylib.h>

struct AssetManager
{
	Texture2D dirt = {};
	Texture2D textures = {};
	Texture2D frame = {};
	Texture2D player = {};
	Texture2D pumpjack = {};
	Texture2D titlecard = {};
	Texture2D forestBG = {};
	Texture2D oildrop = {};
	void loadAll();
};
