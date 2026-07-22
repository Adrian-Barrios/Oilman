#include "assetManager.h"

void AssetManager::loadAll()
{
	dirt = LoadTexture(RESOURCES_PATH "dirt.png");

	textures = LoadTexture(RESOURCES_PATH "textures.png");

	frame = LoadTexture(RESOURCES_PATH "frame.png");

	player = LoadTexture(RESOURCES_PATH "player.png");

	pumpjack = LoadTexture(RESOURCES_PATH "structures/pumpjack.png");

	titlecard = LoadTexture(RESOURCES_PATH "screens/titlecard.png");

	forestBG = LoadTexture(RESOURCES_PATH "forestBG.png");

	oildrop = LoadTexture(RESOURCES_PATH "oil/Oildrop.png");
}