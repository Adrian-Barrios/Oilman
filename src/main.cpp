#include <iostream>
#include "raylib.h"
#include "imgui.h"
#include <rlImGui.h>
int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(30);
	InitWindow(800, 450, "Oilman");
	rlImGuiSetup(true); // Required setup context. Won't work without this.

	int posX = 30;
	int posY = 200;
	int size = 100;

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		rlImGuiBegin(); // ImGui window

		ImGui::Begin("Test");
		ImGui::Text("ImGui visible test");
		ImGui::Button("Click me");
		ImGui::End();

		Color c;
		c.r = 255;
		c.g = 0;
		c.b = 200;
		c.a = 255;

		DrawText("Oilman", 350, 50, 50, RED);
		DrawRectangle(posX, posY, size, size, BLUE);
		posX += 1;

		rlImGuiEnd();

		EndDrawing();
	}

	rlImGuiShutdown();
	CloseWindow();
	return 0;
}