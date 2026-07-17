#include <raylib.h>
#include "gameMain.h"
#include <iostream>
#include <assetManager.h>
#include <gameMap.h>
#include <helpers.h>

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
	gameData.gameMap.getBlocUnsafe(1, 1).type = Block::grass;
	gameData.gameMap.getBlocUnsafe(2, 2).type = Block::goldBlock;
	gameData.gameMap.getBlocUnsafe(3, 3).type = Block::glass;
	gameData.gameMap.getBlocUnsafe(4, 4).type = Block::platform;
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

	// Get mouse position for frame
	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(worldPos.x);
	int blockY = (int)floor(worldPos.y);

	#pragma region cameraMovement
	if (IsKeyDown(KEY_LEFT)) gameData.camera.target.x -= 7.f * deltaTime;
	if (IsKeyDown(KEY_RIGHT)) gameData.camera.target.x +=7.f * deltaTime;
	if (IsKeyDown(KEY_DOWN)) gameData.camera.target.y += 7.f * deltaTime;
	if (IsKeyDown(KEY_UP)) gameData.camera.target.y -= 7.f * deltaTime;
	#pragma endregion 

	#pragma region drawInitialBlocks
	for (int y = 0; y < gameData.gameMap.h; y++)
		for (int x = 0; x < gameData.gameMap.w; x++)
		{
			auto& b = gameData.gameMap.getBlocUnsafe(x, y);
			if (b.type != Block::air)
			{

				float size = 1;
				float posX = x * size;
				float posY = y * size;
				DrawTexturePro(assetManager.textures,
					getTextureAtlas(b.type,0,32,32),
					{(float)x, (float)y,1,1},
					{0,0}, // Origin, top left corner 
					0.0f, // Rotation
					WHITE); // tint
			}
		}

	#pragma endregion
	
#pragma region addAndDeleteBlocks
	
	// Draw frame
	DrawTexturePro(
		assetManager.frame,
		{ 0,0, (float)assetManager.frame.width, (float)assetManager.frame.height },
		{ (float)blockX, (float)blockY,1,1 },
		{ 0,0 },
		0.0f,
		WHITE
	);
	
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		auto b = gameData.gameMap.getBlockSafe(blockX, blockY);
		if (b)
		{
			*b = {};
		}
	}
	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
	{
		auto b = gameData.gameMap.getBlockSafe(blockX, blockY);
		if (b)
		{
			b->type = Block::gold;
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
