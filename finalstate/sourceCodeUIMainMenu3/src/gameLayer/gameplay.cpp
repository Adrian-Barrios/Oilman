#include "gameplay.h"
#include <entities/droppeedItem.h>
#include <ui.h>
#include <audio.h>
#include <settings.h>
#include <worldGenerator.h>
#include <helpers.h>
#include <imgui.h>
#include <saveMap.h>

Rectangle Gameplay::getInventoryRectangle(float w, float h)
{
	Rectangle inventoryRectangle;

	inventoryRectangle.height = h * 0.30;
	inventoryRectangle.width = inventoryRectangle.height * 3;

	inventoryRectangle = placeRectangleTopLeftCorner(inventoryRectangle, w);

	inventoryRectangle.x += w * 0.01;
	inventoryRectangle.y += h * 0.01;
	return inventoryRectangle;
}

void Gameplay::spawnSlime(Vector2 position)
{
	Slime slime;

	slime.physics.teleport(position);

	auto id = entities.idHolder.getEntityIdAndIncrement();

	entities.entities[id] = std::make_unique<Slime>(slime);
}

void Gameplay::spawnDroppedItem(Vector2 position, int type)
{
	DroppedItem droppedItem;

	droppedItem.teleport(position);
	droppedItem.itemType = type;

	auto id = entities.idHolder.getEntityIdAndIncrement();

	entities.entities[id] = (std::make_unique<DroppedItem>(droppedItem));
}

bool Gameplay::init()
{

	generateWorld(gameMap);

	camera.target = {20, 120};  // world-space center of view
	camera.rotation = 0.0f;
	camera.zoom = 100.0f;

	player.physics.teleport({20, 60});
	player.physics.transform.w = 0.9f;
	player.physics.transform.h = 1.8f;

	spawnSlime({18, 60});

	return true;
}

bool Gameplay::update(AssetManager &assetManager)
{


	float deltaTime = GetFrameTime();
	if (deltaTime > 1.f / 5) { deltaTime = 1 / 5.f; }

	camera.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};

	ClearBackground({75, 75, 150, 255});

	if (IsKeyPressed(KEY_F10)) { showImgui = !showImgui; }


#pragma region camera movement

	static float CAMERA_SPEED = 10;
	static bool creative = false;

	{

		bool moving = 0;
		bool falling = 0;

		if (IsKeyDown(KEY_LEFT))
		{
			player.physics.transform.pos.x -= CAMERA_SPEED * GetFrameTime();
			moving = true;
			player.animations.movingLeft = true;
		}

		if (IsKeyDown(KEY_RIGHT))
		{
			player.physics.transform.pos.x += CAMERA_SPEED * GetFrameTime();
			moving = true;
			player.animations.movingLeft = false;
		}

		if (creative)
		{
			if (IsKeyDown(KEY_UP)) { player.physics.transform.pos.y -= CAMERA_SPEED * GetFrameTime(); moving = true; }
			if (IsKeyDown(KEY_DOWN)) { player.physics.transform.pos.y += CAMERA_SPEED * GetFrameTime(); moving = true; }
		};

		if (IsKeyDown(KEY_SPACE))
		{
			player.physics.jump(12.0);
		}

		if (player.physics.downTouch)
		{
			falling = 0;
		}
		else
		{
			falling = 1;
		}

		if (falling)
		{
			player.animations.setAnimation(2);
		}
		else if (moving)
		{
			player.animations.setAnimation(1);
		}
		else
		{
			player.animations.setAnimation(0);
		}

		player.animations.update(deltaTime, 0.08, 7);

	}

#pragma endregion

#pragma region entities

	auto updateEntityPhysics = [&](auto &entity, bool applyGravity = true)
	{
		if (applyGravity) { entity.physics.applyGravity(); }

		entity.physics.updateForces(deltaTime);

		entity.physics.resolveConstrains(gameMap);

		entity.physics.updateFinal();
	};

	updateEntityPhysics(player, !creative);

	camera.target = player.physics.transform.pos;

	//clamp camera
	{
		float zoom = camera.zoom;

		float screenWidth = GetScreenWidth();
		float screenHeight = GetScreenHeight();

		// half of the visible area (adjusted for zoom)
		float halfViewWidth = (screenWidth * 0.5f) / zoom;
		float halfViewHeight = (screenHeight * 0.5f) / zoom;

		float minX = halfViewWidth;
		float maxX = gameMap.w - halfViewWidth;
		float minY = halfViewHeight;
		float maxY = gameMap.h - halfViewHeight;

		// if the map is smaller than the view (zoomed out a lot), we just set the camera to the center of the world
		if (maxX < minX)
		{
			camera.target.x = gameMap.w * 0.5f;
		}
		else
		{
			camera.target.x = Clamp(camera.target.x, minX, maxX);
		}

		if (maxY < minY)
		{
			camera.target.y = gameMap.h * 0.5f;
		}
		else
		{
			camera.target.y = Clamp(camera.target.y, minY, maxY);
		}

	}



	std::ranlux24_base rng(std::random_device{}());

	//update all entities
	for (auto it = entities.entities.begin(); it != entities.entities.end();)
	{
		EntityUpdateData updateData
		{
			player.getPosition(),
			rng,
			entities,
			it->first
		};

		bool shouldKill = false;

		if (!it->second->update(deltaTime, updateData)
			|| it->second->life <= 0
			)
		{
			shouldKill = true;
		}

		if (shouldKill)
		{
			// erase returns the next valid iterator
			it = entities.entities.erase(it);
		}
		else
		{
			//physics
			updateEntityPhysics(*it->second);

			++it;
		}
	}



#pragma endregion


	bool insideInventoryMenu = 0;
	Rectangle inventoryRectangle = getInventoryRectangle(GetScreenWidth(), GetScreenHeight());

	if (insideInventory &&
		CheckCollisionPointRec(GetMousePosition(), inventoryRectangle))
	{
		insideInventoryMenu = true;
	}


	Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), camera);
	int blockX = (int)floor(worldPos.x);
	int blockY = (int)floor(worldPos.y);

	if (creativeSelectedBlock < 0) { creativeSelectedBlock = 0; }
	if (creativeSelectedBlock >= Block::BLOCKS_COUNT) { creativeSelectedBlock = Block::BLOCKS_COUNT - 1; }

	//selection
	if (showImgui)
	{

		if (IsKeyPressed(KEY_ONE)) { selectionStart = Vector2{(float)blockX, (float)blockY}; }
		if (IsKeyPressed(KEY_TWO)) { selectionEnd = Vector2{(float)blockX, (float)blockY}; }
		if (IsKeyPressed(KEY_THREE))
		{
			copyStructure.pasteIntoMap(gameMap, Vector2{(float)blockX, (float)blockY});
		}

		if (selectionStart.x > selectionEnd.x)
		{
			std::swap(selectionStart.x, selectionEnd.x);
		}

		if (selectionStart.y > selectionEnd.y)
		{
			std::swap(selectionStart.y, selectionEnd.y);
		}


	}

	if (!showImgui)
	{
		if (!insideInventoryMenu)
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				auto b = gameMap.getBlockSafe(blockX, blockY);
				if (b)
				{
					if (b->type)
					{
						spawnDroppedItem({(float)blockX + 0.5f, (float)blockY + 0.5f}, b->type);
					}

					*b = {};
				}
			}

		if (!insideInventoryMenu)
			if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
			{
				auto b = gameMap.getBlockSafe(blockX, blockY);
				if (b)
				{
					b->type = creativeSelectedBlock;
				}
			}

		if (IsKeyPressed(KEY_TAB))
		{
			insideInventory = !insideInventory;
		}
	}
#pragma region draw world

	//background
	{

		int backgroundType = DrawBackground::forest;

		if (player.getPosition().x > gameMap.desertStart &&
			player.getPosition().x < gameMap.desertEnd
			)
		{
			backgroundType = DrawBackground::desert;
		}

		if (player.getPosition().y > 130)
		{
			backgroundType = DrawBackground::cave;
		}

		background.setBackground(backgroundType);

		background.draw(deltaTime, assetManager, camera,
			{(float)gameMap.w, (float)gameMap.h});

	}


	BeginMode2D(camera);


	Vector2 topLeftView = GetScreenToWorld2D({0, 0}, camera);
	Vector2 bottomRightView = GetScreenToWorld2D({(float)GetScreenWidth(), (float)GetScreenHeight()}, camera);

	int startXView = (int)floorf(topLeftView.x - 1);
	int endXView = (int)ceilf(bottomRightView.x + 1);
	int startYView = (int)floorf(topLeftView.y - 1);
	int endYView = (int)ceilf(bottomRightView.y + 1);

	startXView = Clamp(startXView, 0, gameMap.w - 1);
	endXView = Clamp(endXView, 0, gameMap.w - 1);

	startYView = Clamp(startYView, 0, gameMap.h - 1);
	endYView = Clamp(endYView, 0, gameMap.h - 1);


	for (int y = startYView; y <= endYView; y++)
		for (int x = startXView; x <= endXView; x++)
		{

			auto &b = gameMap.getBlocUnsafe(x, y);

			if (b.type != Block::air)
			{

				DrawTexturePro(
					assetManager.textures,
					getTextureAtlas(b.type, 0, 32, 32), //source
					{(float)x, (float)y, 1, 1}, //dest
					{0, 0},// origin (top-left corner)
					0.0f, // rotation
					WHITE // tint
				);

			}


		}

	//draw selected block
	if (!insideInventoryMenu)
	{
		DrawTexturePro(
			assetManager.frame,
			{0,0, (float)assetManager.frame.width, (float)assetManager.frame.height}, //source
			{(float)blockX, (float)blockY, 1, 1}, //dest
			{0, 0},// origin (top-left corner)
			0.0f, // rotation
			WHITE // tint
		);
	}

	//show the structure selection
	if (showImgui)
	{
		Rectangle rect;
		rect.x = selectionStart.x;
		rect.y = selectionStart.y;
		rect.width = selectionEnd.x - selectionStart.x;
		rect.height = selectionEnd.y - selectionStart.y;

		rect.width++;
		rect.height++;

		DrawRectangleLinesEx(rect, 0.1,
			{20, 101, 250, 145});
	}

	for (auto &e : entities.entities)
	{
		e.second->render(assetManager);
	}

	player.render(assetManager);

	DrawRectangleLinesEx(player.physics.transform.getAABB(), 0.1,
		{20, 101, 250, 120});


	EndMode2D();

#pragma endregion


#pragma region ui
	{

		float w = GetScreenWidth();
		float h = GetScreenHeight();

		Rectangle hearthRectangle;

		hearthRectangle.height = h * 0.05;
		hearthRectangle.width = hearthRectangle.height * 5;

		hearthRectangle = placeRectangleTopRightCorner(hearthRectangle, w);

		//test the drawing area
		//DrawRectangle(hearthRectangle.x, hearthRectangle.y, hearthRectangle.width, hearthRectangle.height,
		//	RED);


		for (int i = 0; i < 5; i++)
		{

			Rectangle oneHearthRectangle = hearthRectangle;
			oneHearthRectangle.width = oneHearthRectangle.height;
			oneHearthRectangle.x += oneHearthRectangle.width * i;

			//shrink hearths
			//oneHearthRectangle = shrinkRectanglePercentage(oneHearthRectangle, 0.1, 0.1);

			DrawTexturePro(
				assetManager.hearts,
				getTextureAtlas(0, 0, assetManager.hearts.width / 3, assetManager.hearts.height), //source
				oneHearthRectangle, //dest
				{0, 0},// origin (top-left corner)
				0.0f, // rotation
				WHITE // tint
			);

		}


		if (insideInventory)
		{

			Rectangle inventoryRectangle = getInventoryRectangle(w, h);

			DrawRectangle(inventoryRectangle.x, inventoryRectangle.y, inventoryRectangle.width, inventoryRectangle.height,
				{100, 100, 100, 100});

			inventoryRectangle = shrinkRectanglePercentage(inventoryRectangle, 0.01, 0.01);

			Rectangle oneCellRectangle;
			oneCellRectangle.height = inventoryRectangle.height / 3;
			oneCellRectangle.width = oneCellRectangle.height;
			oneCellRectangle.x = inventoryRectangle.x;
			oneCellRectangle.y = inventoryRectangle.y;

			for (int i = 0; i < 9; i++)
				for (int j = 0; j < 3; j++)
				{

					Rectangle r = oneCellRectangle;
					r.x += i * oneCellRectangle.width;
					r.y += j * oneCellRectangle.height;

					r = shrinkRectanglePercentage(r, 0.1, 0.1);

					if (CheckCollisionPointRec(GetMousePosition(), r))
					{
						DrawTexturePro(
							assetManager.frame,
							{0,0,(float)assetManager.frame.width, (float)assetManager.frame.height}, //source
							r, //dest
							{0, 0},// origin (top-left corner)
							0.0f, // rotation
							{220,250,220,250} // tint
						);
					}
					else
					{
						DrawTexturePro(
							assetManager.frame,
							{0,0,(float)assetManager.frame.width, (float)assetManager.frame.height}, //source
							r, //dest
							{0, 0},// origin (top-left corner)
							0.0f, // rotation
							{180,180,200,240} // tint
						);
					}

				}


		}


	}
#pragma endregion

	if (showImgui)
	{
		ImGui::Begin("Game controll");

		ImGui::SliderFloat("Camera zoom:", &camera.zoom, 2, 150);
		ImGui::SliderFloat("Camera speed:", &CAMERA_SPEED, 5, 100);
		ImGui::Checkbox("Creative:", &creative);

		if (ImGui::Button("Hurt a slime"))
		{
			for (auto &e : entities.entities)
			{

				if (e.second->getEntityType() == EntityType_Slime)
				{
					e.second->life -= 3;
					break;
				}

			}
		}

		if (ImGui::Button("Spawn Slime"))
		{
			spawnSlime({18, 60});
		}


		if (ImGui::Button("Copy"))
		{
			copyStructure.copyFromMap(gameMap,
				selectionStart, selectionEnd);
		}

		ImGui::InputText("File name", saveName, sizeof(saveName));

		if (ImGui::Button("Safe to file"))
		{
			std::string path = RESOURCES_PATH "structures/";
			path += saveName;
			path += ".bin";

			saveBlockDataToFile(copyStructure.mapData,
				copyStructure.w, copyStructure.h,
				path.c_str());
		}

		if (ImGui::Button("Load from file"))
		{
			std::string path = RESOURCES_PATH "structures/";
			path += saveName;
			path += ".bin";

			loadBlockDataFromFile(copyStructure.mapData,
				copyStructure.w, copyStructure.h,
				path.c_str());
		}

		ImGui::Separator();


		ImGui::SliderFloat("master volume", &getSettings().masterVolume, 0, 1);
		ImGui::SliderFloat("sound volume", &getSettings().soundsVolume, 0, 1);
		ImGui::SliderFloat("music volume", &getSettings().musicVolume, 0, 1);

		if (ImGui::Button("Play sound"))
		{
			Audio::playSound(Audio::placeBlock);
		}

		if (ImGui::Button("Play music forest"))
		{
			Audio::playMusic(Audio::musicForest);
		}

		if (ImGui::Button("Play music desert"))
		{
			Audio::playMusic(Audio::musicDesert);
		}

		ImGui::Separator();

		if (ImGui::Button("Save World"))
		{
			saveWorld(gameMap,
				entities, player);
		}

		if (ImGui::Button("Load World"))
		{
			if (!loadWorld(gameMap,
				entities, player))
			{
				return false;
			}
		}

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
				{35,35}, {atlas.x, atlas.y},
				{atlas.x + atlas.width, atlas.y + atlas.height}))
			{
				creativeSelectedBlock = i;
			}

			ImGui::PopID();

			if (i % 10 != 0)
			{
				ImGui::SameLine();
			}

		}


		ImGui::End();
	}

#pragma endregion

	DrawFPS(10, 10);


	return true;
}
