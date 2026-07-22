#include <raylib.h>
#include "gameMain.h"
#include <asserts.h>
#include <assetManager.h>
#include <gameMap.h>
#include <helpers.h>
#include <raymath.h>
#include <worldGenerator.h>
#include <imgui.h>
#include <structure.h>
#include <string>
#include <saveMap.h>
#include <physics.h>
#include <entities/slime.h>
#include <entities/droppeedItem.h>
#include <entityIdHolder.h>
#include "player.h"
#include "items.h"
#include <audio.h>
#include <settings.h>
#include <drawBackground.h>
#include <ui.h>
#include <gameplay.h>

AssetManager assetManager;
Gameplay gameplay;
UIEngine mainMenuButtons;
DrawBackground backgroundForMainMenu;
bool gameplayRunning = false;


bool initGame()
{

	Audio::init();
	assetManager.loadAll();
	loadSettings();
	
	gameplay.init();

	return true;
}

bool updateGame()
{
	Audio::update();
	updateSettings();

	ClearBackground({0, 0, 0, 255});

	if (!gameplayRunning)
	{

		//you can animate the background if you want by moving the camera and changing it from time to time
		Camera2D c = {};
		c.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
		c.target = Vector2{200, 500};
		c.zoom = 20;
		backgroundForMainMenu.draw(GetFrameTime(), assetManager, c, {1000, 1000});


		mainMenuButtons.addTitle("Canvas Adventures");

		if (mainMenuButtons.addButton("Start game"))
		{
			gameplayRunning = true;
			gameplay = {}; //make sure we reset the gameplay
			gameplay.init();

		}

		mainMenuButtons.addButton("Settings");

		if (mainMenuButtons.addButton("Exit"))
		{
			return false;
		}

		mainMenuButtons.updateAndRender();

		return true;
	}
	else
	{
		return gameplay.update(assetManager);
	}

	return true;
}

void closeGame()
{
}
