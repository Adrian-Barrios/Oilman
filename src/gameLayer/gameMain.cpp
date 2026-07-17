#include <raylib.h>
#include "gameMain.h"
#include <iostream>

bool initGame()
{
	return true;
}
bool updateGame()
{
	if (IsKeyPressed(KEY_DOWN)) // Key/input actions
	{
		std::cout << "Pressed key down" << std::endl;
	}
	Color c;
	c.r = 255;
	c.g = 0;
	c.b = 200;
	c.a = 255;

	DrawText("Oilman", 350, 50, 50, RED);
	return true;
}
void closeGame()
{ 
	std::cout << "Close game called. Game closed." << std::endl;
}
