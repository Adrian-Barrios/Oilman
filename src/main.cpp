#include <iostream>
#include "raylib.h"

int main()
{
	SetTargetFPS(30);
	InitWindow(800, 450, "my game");

	int posX = 30;
	int posY = 30;
	int size = 100;

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);

		DrawRectangle(posX, posY, size, size, BLUE);
		posX += 1;

		EndDrawing();
	}

	return 0;
}