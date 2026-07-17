#include <raylib.h>
#include "gameMain.h"
#include <iostream>

struct GameData
{
	float positionX = 100;
	float positionY = 100;
} gameData;

bool initGame()
{
	return true;
}
bool updateGame()
{
	#pragma region movementAndDeltaTime
	float deltaTime = GetFrameTime();
	if (deltaTime > 1.f / 5) {deltaTime = 1 / 5.f;} // Prevents sprite from jumping around.
	Color c;
	c.r = 255;
	c.g = 0;
	c.b = 200;
	c.a = 255;

	if (IsKeyDown(KEY_DOWN)) {gameData.positionY += 200*deltaTime;}
	if (IsKeyDown(KEY_UP))   {gameData.positionY -= 200*deltaTime;}
	if (IsKeyDown(KEY_LEFT)) {gameData.positionX -= 200*deltaTime;}
	if (IsKeyDown(KEY_RIGHT)){gameData.positionX += 200*deltaTime;}
	#pragma endregion

	DrawText("Oilman", 350, 50, 50, RED);
	DrawRectangle(gameData.positionX, gameData.positionY, 40, 40, BLUE);

	return true;
}
void closeGame()
{ 
 
	std::cout << "Close game called. Game closed." << std::endl;
}
