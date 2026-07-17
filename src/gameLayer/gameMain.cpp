#include <raylib.h>
#include "gameMain.h"
#include <iostream>
#include "assetManager.h"
struct GameData
{
} gameData;

AssetManager assetManager;

bool initGame()
{
	assetManager.loadAll();
	return true;
}
bool updateGame()
{

	DrawTexturePro(assetManager.dirt, 
		{0,0,(float)assetManager.dirt.width,(float)assetManager.dirt.height},
		{50,50,100,100},{},0,WHITE); // Color affects the texture.

	return true;
}
void closeGame()
{ 
 
	std::cout << "Close game called. Game closed." << std::endl;
}
