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

// all in blocks per second, or blocks per second squared for gravity. y grows
// downwards, so gravity is positive and a jump starts off negative
constexpr float PLAYER_MOVE_SPEED = 7.f;
constexpr float PLAYER_JUMP_SPEED = 12.f;
constexpr float GRAVITY = 40.f;
constexpr float MAX_FALL_SPEED = 30.f;

// what the mouse does inside level editing mode
enum EditingSubMode
{
	copyPasteSubMode = 0,
	placementSubMode,
};

struct GameData
{
	GameMap gameMap;
	Camera2D camera;

	Vector2 playerPosition = {}; // top left corner of the player's collision box
	Vector2 playerVelocity = {};
	bool playerGrounded = false;
	bool playerFacingRight = true; // kept from the last move, so idling doesn't reset it

	int creativeSelectedBlock = Block::dirt;
	int editingSubMode = copyPasteSubMode;

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

// does the player's box at this position overlap anything solid. the sides and the
// bottom of the map count as solid so you can't walk off the world or fall out of
// it, but the open sky above the map doesn't
static bool boxOverlapsSolid(GameMap& map, Vector2 pos)
{
	int minX = (int)floorf(pos.x);
	int maxX = (int)ceilf(pos.x + PLAYER_WIDTH) - 1;
	int minY = (int)floorf(pos.y);
	int maxY = (int)ceilf(pos.y + PLAYER_HEIGHT) - 1;

	for (int y = minY; y <= maxY; y++)
		for (int x = minX; x <= maxX; x++)
		{
			if (x < 0 || x >= map.w) { return true; }
			if (y >= map.h) { return true; }
			if (y < 0) { continue; }

			if (Block::isSolid(map.getBlocUnsafe(x, y).type)) { return true; }
		}

	return false;
}

// moves the player along a single axis and stops flush against the first solid
// block in the way, returning whether it hit something. the move is split into sub
// block steps so a fast fall can't tunnel straight through a thin floor
static bool movePlayerAxis(GameMap& map, Vector2& pos, float amount, bool horizontal)
{
	constexpr float MAX_STEP = 0.25f;

	int steps = (int)ceilf(fabsf(amount) / MAX_STEP);
	if (steps < 1) { steps = 1; }

	float stepAmount = amount / steps;

	for (int i = 0; i < steps; i++)
	{
		Vector2 next = pos;
		if (horizontal) { next.x += stepAmount; }
		else { next.y += stepAmount; }

		if (boxOverlapsSolid(map, next))
		{
			// back out to the face of the block we ran into
			if (horizontal)
			{
				if (stepAmount > 0) { pos.x = floorf(next.x + PLAYER_WIDTH) - PLAYER_WIDTH; }
				else { pos.x = floorf(next.x) + 1; }
			}
			else
			{
				if (stepAmount > 0) { pos.y = floorf(next.y + PLAYER_HEIGHT) - PLAYER_HEIGHT; }
				else { pos.y = floorf(next.y) + 1; }
			}

			return true;
		}

		pos = next;
	}

	return false;
}

// drops the player onto the first solid block of their spawn column
static void spawnPlayer(GameMap& map, Vector2& pos)
{
	constexpr int SPAWN_X = 20;

	pos = { (float)SPAWN_X, 0 };

	for (int y = 0; y < map.h; y++)
	{
		if (Block::isSolid(map.getBlocUnsafe(SPAWN_X, y).type))
		{
			pos.y = y - PLAYER_HEIGHT;
			break;
		}
	}
}

bool initGame()
{
	assetManager.loadAll();

	#pragma region mapCreation
	generateWorld(gameData.gameMap,1234);
	spawnPlayer(gameData.gameMap, gameData.playerPosition);
	#pragma endregion

	#pragma region camera
	gameData.camera.target = gameData.playerPosition;
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

#pragma region playerPhysics
	// the player is frozen while editing, so you can build around them and fly the
	// camera off without dragging them along
	if (!levelEditingMode)
	{
		float moveInput = 0;
		if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) { moveInput -= 1; }
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { moveInput += 1; }

		gameData.playerVelocity.x = moveInput * PLAYER_MOVE_SPEED;

		if (moveInput != 0) { gameData.playerFacingRight = moveInput > 0; }

		bool jumpPressed = IsKeyPressed(KEY_SPACE)
			|| IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);

		if (jumpPressed && gameData.playerGrounded)
		{
			gameData.playerVelocity.y = -PLAYER_JUMP_SPEED;
			gameData.playerGrounded = false;
		}

		gameData.playerVelocity.y += GRAVITY * deltaTime;
		if (gameData.playerVelocity.y > MAX_FALL_SPEED)
		{
			gameData.playerVelocity.y = MAX_FALL_SPEED;
		}

		// one axis at a time, so running into a wall still lets gravity work and
		// vice versa
		movePlayerAxis(gameData.gameMap, gameData.playerPosition,
			gameData.playerVelocity.x * deltaTime, true);

		bool hitVertically = movePlayerAxis(gameData.gameMap, gameData.playerPosition,
			gameData.playerVelocity.y * deltaTime, false);

		if (hitVertically)
		{
			// landed if we were on the way down, hit a ceiling if we were going up
			gameData.playerGrounded = gameData.playerVelocity.y > 0;
			gameData.playerVelocity.y = 0;
		}
		else
		{
			gameData.playerGrounded = false;
		}

		gameData.camera.target = {
			gameData.playerPosition.x + PLAYER_WIDTH / 2.f,
			gameData.playerPosition.y + PLAYER_HEIGHT / 2.f };
	}
#pragma endregion

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

	// playerPosition is the top left of the collision box, which is exactly what
	// the destination rectangle wants

	// a negative source width mirrors the sprite, the sheet has him facing right
	float playerSourceWidth = (float)assetManager.player.width;
	if (!gameData.playerFacingRight) { playerSourceWidth = -playerSourceWidth; }

	DrawTexturePro(
		assetManager.player,
		{ 0, 0, playerSourceWidth, (float)assetManager.player.height }, //source
		{ gameData.playerPosition.x, gameData.playerPosition.y,
			PLAYER_WIDTH, PLAYER_HEIGHT }, //dest
		{ 0, 0 }, // origin (top-left corner)
		0.0f, // rotation
		WHITE // tint
	);

#pragma endregion

#pragma region cameraMovement
	// only a free camera while editing. while playing it follows the player
	static float CAMERA_SPEED = 7;
	if (levelEditingMode)
	{
		if (IsKeyDown(KEY_LEFT)) gameData.camera.target.x -= CAMERA_SPEED * deltaTime;
		if (IsKeyDown(KEY_RIGHT)) gameData.camera.target.x += CAMERA_SPEED * deltaTime;
		if (IsKeyDown(KEY_DOWN)) gameData.camera.target.y += CAMERA_SPEED * deltaTime;
		if (IsKeyDown(KEY_UP)) gameData.camera.target.y -= CAMERA_SPEED * deltaTime;
	}

#pragma endregion
	
#pragma region addAndDeleteBlocks
	
	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), gameData.camera);
	int blockX = (int)floor(worldPos.x);
	int blockY = (int)floor(worldPos.y);

	if (gameData.creativeSelectedBlock < 0) { gameData.creativeSelectedBlock = 0; }
	if (gameData.creativeSelectedBlock >= Block::BLOCKS_COUNT) { gameData.creativeSelectedBlock = Block::BLOCKS_COUNT - 1; } //Prevents selecting inexistent block

	// don't edit the world while the cursor is over an ImGui window
	bool mouseOverUI = ImGui::GetIO().WantCaptureMouse;

	if (levelEditingMode && gameData.editingSubMode == placementSubMode)
	{
		if (!mouseOverUI)
		{
			// held rather than pressed so dragging paints a run of blocks
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				auto b = gameData.gameMap.getBlockSafe(blockX, blockY);
				if (b)
				{
					b->type = gameData.creativeSelectedBlock;
				}
			}

			if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				auto b = gameData.gameMap.getBlockSafe(blockX, blockY);
				if (b)
				{
					*b = {};
				}
			}
		}
	}
	else if (levelEditingMode)
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

	// show the block that would be placed, ghosted over the cell under the cursor
	if (levelEditingMode && gameData.editingSubMode == placementSubMode
		&& gameData.creativeSelectedBlock != Block::air)
	{
		DrawTexturePro(
			assetManager.textures,
			getTextureAtlas(gameData.creativeSelectedBlock, 0, 32, 32), //source
			{ (float)blockX, (float)blockY, 1, 1 }, //dest
			{ 0, 0 },// origin (top-left corner)
			0.0f, // rotation
			{ 255, 255, 255, 140 } // tint, faded so it reads as a preview
		);
	}

	if (levelEditingMode && gameData.editingSubMode == copyPasteSubMode)
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

		ImGui::RadioButton("Copy paste mode", &gameData.editingSubMode, copyPasteSubMode);
		ImGui::RadioButton("Placement mode", &gameData.editingSubMode, placementSubMode);

		if (gameData.editingSubMode == copyPasteSubMode)
		{
			ImGui::TextUnformatted("Middle drag selects, right copies, left pastes");
		}
		else
		{
			ImGui::TextUnformatted("Left places the selected block, right deletes");
		}

		ImGui::Separator();

		ImTextureID tex = (ImTextureID)(intptr_t)assetManager.textures.id;

		// normalized atlas coordinates, which is what ImGui wants for uvs
		auto atlasUV = [](int blockType)
		{
			auto atlas = getTextureAtlas(blockType, 0, 32, 32);
			atlas.x /= assetManager.textures.width;
			atlas.width /= assetManager.textures.width;
			atlas.y /= assetManager.textures.height;
			atlas.height /= assetManager.textures.height;
			return atlas;
		};

		auto selectedAtlas = atlasUV(gameData.creativeSelectedBlock);

		ImGui::TextUnformatted("Selected block:");
		ImGui::SameLine();
		ImGui::Image(tex, { 35,35 },
			{ selectedAtlas.x, selectedAtlas.y },
			{ selectedAtlas.x + selectedAtlas.width, selectedAtlas.y + selectedAtlas.height });

		for (int i = 0; i < Block::BLOCKS_COUNT; i++)
		{

			auto atlas = atlasUV(i);

			ImGui::PushID(i);

			// tint the button of the block that's currently selected so it stands
			// out from the rest of the palette
			bool isSelected = (i == gameData.creativeSelectedBlock);
			if (isSelected)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.55f, 0.95f, 1.f));
			}

			if (ImGui::ImageButton(tex,
				{ 35,35 }, { atlas.x, atlas.y },
				{ atlas.x + atlas.width, atlas.y + atlas.height }))
			{
				gameData.creativeSelectedBlock = i;
			}

			if (isSelected)
			{
				ImGui::PopStyleColor();
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