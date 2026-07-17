#include <iostream>
#include "raylib.h"
#include "imgui.h"
#include <rlImGui.h>
int main()
{
#pragma region initialConfig
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(30);
	InitWindow(800, 450, "Oilman");
#pragma endregion

#pragma region imguiMainConfig
	// ImGui Config
	rlImGuiSetup(true); // Required setup context. Won't work without this.
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 2;
	io.ConfigFlags = ImGuiConfigFlags_DockingEnable; // Allows us to snap ImGui windows together
	ImGui::StyleColorsLight();
#pragma endregion

#pragma region mainWhileLoop
	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);
		#pragma region imGuiWindow
		rlImGuiBegin(); // ImGui window

		// Allow for docking ImGui windows without covering with a gray screen
		// Allow for docking ImGui windows without covering with a gray screen
		ImGui::PushStyleColor(ImGuiCol_WindowBg, {});
		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, {}); 
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()); // Allows docking on main window.
		ImGui::PopStyleColor(2);

		ImGui::Begin("Test");
		ImGui::Text("ImGui visible test");
		if (ImGui::Button("Button"))
		{
			std::cout << "Button pressed" << std::endl;
		}
		if (ImGui::Button("Button 2")) // Can't have 2 widgets with the same Id.
		{
			std::cout << "Second button pressed" << std::endl;
		}
		ImGui::Separator();
		ImGui::NewLine();
		static float a = 0;
		ImGui::SliderFloat("slider", &a, 0, 1);
		ImGui::End();
		#pragma endregion imGuiWindow

		Color c;
		c.r = 255;
		c.g = 0;
		c.b = 200;
		c.a = 255;

		DrawText("Oilman", 350, 50, 50, RED);
		rlImGuiEnd();

		EndDrawing();
	}
#pragma endregion

	rlImGuiShutdown();
	CloseWindow();
	return 0;
}