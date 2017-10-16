#pragma once
#include "../Helper/Singleton.hpp"
#include <string>
#include <SDL.h>
#include <vector>

struct Settings : public Singleton<Settings>
{
public:
	Settings():Window(WindowSettings())
	{

	}
	~Settings() {}

	struct WindowSettings
	{
		WindowSettings() :
			Fullscreen(false),
			Title("ETEngine"),
			pWindow(nullptr)
		{
			std::vector<glm::ivec2> resolutions;
			resolutions.push_back(glm::ivec2(1280, 720));
			resolutions.push_back(glm::ivec2(1920, 1080));
			resolutions.push_back(glm::ivec2(2560, 1440));

			uint32 baseRes = 2;

			Width = Fullscreen ? resolutions[baseRes].x : resolutions[baseRes-1].x;
			Height = Fullscreen ? resolutions[baseRes].y : resolutions[baseRes-1].y;
		}
		void VSync(const bool enabled){SDL_GL_SetSwapInterval(enabled);}
		float GetAspectRatio() { return (float)Width / (float)Height; }

		bool Fullscreen;
		int32 Width;
		int32 Height;
		std::string Title;
		SDL_Window* pWindow;
	}Window;

	struct GraphicsSettings
	{
		GraphicsSettings() :
			NumCascades(3),
			NumPCFSamples(3),
			CSMDrawDistance(200),
			NumBlurPasses(5)
		{

		}
		void VSync(const bool enabled) { SDL_GL_SetSwapInterval(enabled); }

		//Shadow Quality
		int32 NumCascades;
		float CSMDrawDistance;
		int32 NumPCFSamples;

		//Bloom Quality
		int32 NumBlurPasses;
	}Graphics;
};
