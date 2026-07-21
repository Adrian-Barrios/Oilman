#include <raylib.h>
#include "gameMain.h"
#include "imgui.h"
#include <rlImGui.h>
#include <iostream>
#include <assetManager.h>
#include <gameMap.h>
#include <helpers.h>
#include <raymath.h>
#include <worldGenerator.h>
#include <randomStuff.h>
#include <saveMap.h>
#include <string>
#include <structure.h>

// player.png is 32x64, and blocks are drawn as 1x1 world units from a 32x32
// atlas, so the player covers one block across and two blocks up
constexpr float PLAYER_WIDTH = 1.f;
constexpr float PLAYER_HEIGHT = 2.f;

struct GameData
{
	GameMap gameMap;
	Camera2D camera;

	Vector2 playerPosition = {};

	int creativeSelectedBlock = Block::dirt;

	// the two corners of the middle click drag, kept unordered; the min/max are
	// worked out where they're used
	Vector2 selectionStart = {};
	Vector2 selectionEnd = {};

	Structure copyStructure;

	char saveName[100] = {};

} gameData;

AssetManager assetManager;

// the game runs in one of two modes, swapped with tab: playing, and level editing
// where the ImGui windows show and the mouse copies and pastes structures
bool levelEditingMode = false;

bool initGame()
{
	assetManager.loadAll();

	#pragma region mapCreation
	generateWorld(gameData.gameMap,1234);
	#pragma endregion

	#pragma region camera
	gameData.camera.target = { 20, 120 };
	gameData.camera.rotation = 0.0f;
	gameData.camera.zoom = 100.0f;
	#pragma endregion

	return true;
}
bool updateGame()
{

	float deltaTime = GetFrameTime();
	if (deltaTime > 1.f / 5) { deltaTime = 1 / 5.f; }

	if (IsKeyPressed(KEY_TAB)) { levelEditingMode = !levelEditingMode; }

	gameData.camera.offset = { GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f };

#pragma region cameraBounds
	// stoping the view before the world edge so the empty space past the map never shows
	float halfViewWidth = (GetScreenWidth() / 2.0f) / gameData.camera.zoom;

	if (gameData.gameMap.w <= halfViewWidth * 2)
	{
		gameData.camera.target.x = gameData.gameMap.w / 2.0f;
	}
	else
	{
		gameData.camera.target.x = Clamp(gameData.camera.target.x,
			halfViewWidth, gameData.gameMap.w - halfViewWidth);
	}
#pragma endregion

	ClearBackground({ 75,75,150,255 });

	BeginMode2D(gameData.camera);

#pragma region drawInitialBlocks

	// Get mouse position for frame
	Vector2 topLeftView = GetScreenToWorld2D({ 0, 0 }, gameData.camera);
	Vector2 bottomRightView = GetScreenToWorld2D({ (float)GetScreenWidth(), (float)GetScreenHeight() }, gameData.camera);


	int startXView = (int)floorf(topLeftView.x - 1);
	int endXView = (int)ceilf(bottomRightView.x + 1);
	int startYView = (int)floorf(topLeftView.y - 1);
	int endYView = (int)ceilf(bottomRightView.y + 1);

	startXView = Clamp(startXView, 0, gameData.gameMap.w - 1);
	endXView = Clamp(endXView, 0, gameData.gameMap.w - 1);

	startYView = Clamp(startYView, 0, gameData.gameMap.h - 1);
	endYView = Clamp(endYView, 0, gameData.gameMap.h - 1);


	for (int y = startYView; y <= endYView; y++)
		for (int x = startXView; x <= endXView; x++)
		{

			auto& b = gameData.gameMap.getBlocUnsafe(x, y);

			if (b.type != Block::air)
			{

				DrawTexturePro(
					assetManager.textures,
					getTextureAtlas(b.type, 0, 32, 32), //source
					{ (float)x, (float)y, 1, 1 }, //dest
					{ 0, 0 },// origin (top-left corner)
					0.0f, // rotation
					WHITE // tint
				);

			}


		}

#pragma endregion

#pragma region drawPlayer

	// no physics yet, the player just rides the camera. camera.offset is the middle
	// of the screen, so camera.target is whatever world point ends up centered,
	// including after the world edge clamp above
	gameData.playerPosition = gameData.camera.target;

	DrawTexturePro(
		assetManager.player,
		{ 0, 0, (float)assetManager.player.width, (float)assetManager.player.height }, //source
		{ gameData.playerPosition.x, gameData.playerPosition.y,
			PLAYER_WIDTH, PLAYER_HEIGHT }, //dest
		{ PLAYER_WIDTH / 2.f, PLAYER_HEIGHT / 2.f }, // origin (sprite center)
		0.0f, // rotation
		WHITE // tint
	);

#pragma endregion

#pragma region cameraMovement
	static float CAMERA_SPEED = 7;
	if (IsKeyDown(KEY_LEFT)) gameData.camera.target.x -= CAMERA_SPEED * deltaTime;
	if (IsKeyDown(KEY_RIGHT)) gameData.camera.target.x += CAMERA_SPEED * deltaTime;
	if (IsKeyDown(KEY_DOWN)) gameData.camera.target.y += CAMERA_SPEED * deltaTime;
	if (IsKeyDown(KEY_UP)) gameData.camera.target.y -= CAMERA_SPEED * deltaTime;

#pragma endregion 
	
#pragma region addAndDeleteBlocks
	
	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(worldPos.x);
	int blockY = (int)floor(worldPos.y);

	if (gameData.creativeSelectedBlock < 0) { gameData.creativeSelectedBlock = 0; }
	if (gameData.creativeSelectedBlock >= Block::BLOCKS_COUNT) { gameData.creativeSelectedBlock = Block::BLOCKS_COUNT - 1; } //Prevents selecting inexistent block

	// don't edit the world while the cursor is over an ImGui window
	bool mouseOverUI = ImGui::GetIO().WantCaptureMouse;

	if (levelEditingMode)
	{
		if (!mouseOverUI)
		{
			// holding middle click drags out the region to copy. the press anchors one
			// corner and the drag keeps moving the other one
			if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE))
			{
				gameData.selectionStart = Vector2{ (float)blockX, (float)blockY };
				gameData.selectionEnd = gameData.selectionStart;
			}
			else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
			{
				gameData.selectionEnd = Vector2{ (float)blockX, (float)blockY };
			}

			if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
			{
				// copyFromMap orders the corners itself
				gameData.copyStructure.copyFromMap(gameData.gameMap,
					gameData.selectionStart, gameData.selectionEnd);
			}

			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				gameData.copyStructure.pasteIntoMap(gameData.gameMap,
					Vector2{ (float)blockX, (float)blockY });
			}
		}
	}
	else
	{
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
				b->type = gameData.creativeSelectedBlock;
			}
		}
	}

	// Draw frame
	DrawTexturePro(
		assetManager.frame,
		{ 0,0, (float)assetManager.frame.width, (float)assetManager.frame.height },
		{ (float)blockX, (float)blockY,1,1 },
		{ 0,0 },
		0.0f,
		WHITE
	);

	if (levelEditingMode)
	{
		// the drag corners are unordered, so take the min and max to get the rectangle
		float minX = fminf(gameData.selectionStart.x, gameData.selectionEnd.x);
		float minY = fminf(gameData.selectionStart.y, gameData.selectionEnd.y);
		float maxX = fmaxf(gameData.selectionStart.x, gameData.selectionEnd.x);
		float maxY = fmaxf(gameData.selectionStart.y, gameData.selectionEnd.y);

		Rectangle rect;
		rect.x = minX;
		rect.y = minY;
		rect.width = maxX - minX + 1;
		rect.height = maxY - minY + 1;

		DrawRectangleLinesEx(rect, 0.1,
			{ 20, 101, 250, 145 });
	}

#pragma endregion

	EndMode2D();

	if (levelEditingMode)
	{
		ImGui::Begin("Game control");
		ImGui::SliderFloat("Camera speed: ", &CAMERA_SPEED, 5, 30);
		ImGui::SliderFloat("Camera zoom: ", &gameData.camera.zoom, 30, 100);
		ImGui::Separator();

		for (int i = 0; i < Block::BLOCKS_COUNT; i++)
		{

			auto atlas = getTextureAtlas(i, 0, 32, 32);
			atlas.x /= assetManager.textures.width;
			atlas.width /= assetManager.textures.width;
			atlas.y /= assetManager.textures.height;
			atlas.height /= assetManager.textures.height;

			ImGui::PushID(i);

			ImTextureID tex = (ImTextureID)(intptr_t)assetManager.textures.id;
			if (ImGui::ImageButton(tex,
				{ 35,35 }, { atlas.x, atlas.y },
				{ atlas.x + atlas.width, atlas.y + atlas.height }))
			{
				gameData.creativeSelectedBlock = i;
			}

			ImGui::PopID();

			if (i % 10 != 0)
			{
				ImGui::SameLine();
			}

		}

		ImGui::End();
	}

	DrawText(levelEditingMode ? "EDITING (tab)" : "PLAYING (tab)",
		10, 34, 20, RAYWHITE);

	DrawFPS(10, 10);
	return true;
}
void closeGame()
{
	std::cout << "Closed game" << std::endl;
}