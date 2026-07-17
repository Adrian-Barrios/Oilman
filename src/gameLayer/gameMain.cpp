#include <raylib.h>
#include "gameMain.h"
#include <iostream>
#include <assetManager.h>
#include <gameMap.h>

struct GameData
{
	GameMap gameMap;
	Camera2D camera;

} gameData;

AssetManager assetManager;

bool initGame()
{
	assetManager.loadAll();

	#pragma region mapCreation
	gameData.gameMap.create(30, 10);
	gameData.gameMap.getBlocUnsafe(0, 0).type = Block::dirt;
	gameData.gameMap.getBlocUnsafe(1, 1).type = Block::dirt;
	gameData.gameMap.getBlocUnsafe(2, 2).type = Block::dirt;
	gameData.gameMap.getBlocUnsafe(3, 3).type = Block::dirt;
	gameData.gameMap.getBlocUnsafe(4, 4).type = Block::dirt;
	#pragma endregion

	#pragma region camera
	gameData.camera.target = { 0,0 };
	gameData.camera.rotation = 0.0f;
	gameData.camera.zoom = 100.0f;
	#pragma endregion

	return true;
}
bool updateGame()
{

	float deltaTime = GetFrameTime();
	if (deltaTime > 1.f / 5) { deltaTime = 1 / 5.f; }

	gameData.camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };

	ClearBackground({ 75,75,150,255 });

	BeginMode2D(gameData.camera);

	#pragma region cameraMovement
	if (IsKeyDown(KEY_LEFT)) gameData.camera.target.x -= 7.f * deltaTime;
	if (IsKeyDown(KEY_RIGHT)) gameData.camera.target.x +=7.f * deltaTime;
	if (IsKeyDown(KEY_DOWN)) gameData.camera.target.y += 7.f * deltaTime;
	if (IsKeyDown(KEY_UP)) gameData.camera.target.y -= 7.f * deltaTime;
	#pragma endregion 

	#pragma region drawBlocks
	for (int y = 0; y < gameData.gameMap.h; y++)
		for (int x = 0; x < gameData.gameMap.w; x++)
		{
			auto& b = gameData.gameMap.getBlocUnsafe(x, y);
			if (b.type != Block::air)
			{
				float size = 1;
				float posX = x * size;
				float posY = y * size;
				DrawTexturePro(assetManager.dirt,
					Rectangle{ 0,0,(float)assetManager.dirt.width,(float)assetManager.dirt.height },
					{ posX, posY, size, size }, 
					{0,0}, // Origin, top left corner 
					0, // Rotation
					WHITE); // tint
			}
		}
	
	#pragma endregion

	EndMode2D();

	return true;
}
void closeGame()
{ 
 
	std::cout << "Close game called. Game closed." << std::endl;
}
