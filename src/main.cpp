#include <iostream>
#include "raylib.h"

int main()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(30);
	InitWindow(800, 450, "Oilman");

	int posX = 30;
	int posY = 200;
	int size = 100;

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		DrawText("Oilman", 350, 50, 50, RED);
		DrawRectangle(posX, posY, size, size, BLUE);
		posX += 1;

		EndDrawing();
	}
	CloseWindow();
	return 0;
}