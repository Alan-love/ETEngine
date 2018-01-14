#include "stdafx.hpp"
#include "AbstractFramework.hpp"

#include <FreeImage.h>

#include "../SceneGraph/SceneManager.hpp"
#include "../GraphicsHelper/LightVolume.hpp"
#include "../GraphicsHelper/ShadowRenderer.hpp"
#include "../GraphicsHelper/TextRenderer.hpp"
#include "../Helper/PerformanceInfo.hpp"
#include "../GraphicsHelper/PrimitiveRenderer.hpp"
#include "../GraphicsHelper/RenderPipeline.hpp"


#ifdef EDITOR
#include "../Editor/Editor.hpp"
#endif
#include "FileSystem/Entry.h"
#include "FileSystem/JSONparser.h"
#include "FileSystem/FileUtil.h"
#include "FileSystem/JSONdom.h"


void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {
	printf("\n*** ");
	if (fif != FIF_UNKNOWN) {
		printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
	}
	printf(message);
	printf(" ***\n");
}
AbstractFramework::AbstractFramework()
{
}
AbstractFramework::~AbstractFramework()
{
	Logger::Release();

	ContentManager::Release();

	FreeImage_DeInitialise();

#ifdef EDITOR
	Editor::GetInstance()->DestroyInstance();
#endif

	SDL_GL_DeleteContext(m_GlContext);
	SDL_Quit();

	SceneManager::GetInstance()->DestroyInstance();
	InputManager::GetInstance()->DestroyInstance();
	Context::GetInstance()->DestroyInstance();
	
	RenderPipeline::GetInstance()->DestroyInstance();
	Settings::GetInstance()->DestroyInstance();
}

void AbstractFramework::Run()
{
	InitializeSDL();
	LoadConfig();
	InitializeWindow();
	InitializeUtilities();
	BindOpenGL();
	InitializeDebug();
	InitializeGame();

	GameLoop();
}



void AbstractFramework::InitializeSDL()
{
	// Request an OpenGL 4.5 context (should be core)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	// Also request a depth buffer
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	// Request a debug context.
#if defined(GRAPHICS_API_DEBUG)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	// Initialize SDL 
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		quit_SDL_error("Couldn't initialize SDL");
	atexit(SDL_Quit);
	SDL_GL_LoadLibrary(NULL);
}

void AbstractFramework::LoadConfig()
{
	Settings* pSet = Settings::GetInstance();//Initialize Game Settings

	File* jsonFile = new File("./config.json", nullptr);
	if (!jsonFile->Open(FILE_ACCESS_MODE::Read))
		return;

	JSON::Parser parser = JSON::Parser(FileUtil::AsText(jsonFile->Read()));
	delete jsonFile;
	jsonFile = nullptr;

	JSON::Object* root = parser.GetRoot();
	if (!root)
	{
		std::cout << "unable to read config.json" << std::endl;
		return;
	}

	JSON::Object* graphics = (*root)["graphics"]->obj();
	if (graphics)
	{
		JSON::ApplyNumValue(graphics, pSet->Graphics.NumCascades, "CSM Cascade Count");
		JSON::ApplyNumValue(graphics, pSet->Graphics.NumPCFSamples, "PCF Sample Count");
		JSON::ApplyNumValue(graphics, pSet->Graphics.CSMDrawDistance, "CSM Draw Distance");
		JSON::ApplyNumValue(graphics, pSet->Graphics.PbrBrdfLutSize, "PBR BRDF LUT Resolution");
		JSON::ApplyNumValue(graphics, pSet->Graphics.TextureScaleFactor, "Texture Scale Factor");
		JSON::ApplyNumValue(graphics, pSet->Graphics.NumBlurPasses, "Bloom Blur Passes");
	}
	JSON::Object* window = (*root)["window"]->obj();
	if (window)
	{
		JSON::ApplyBoolValue(window, pSet->Window.Fullscreen, "Fullscreen");
		JSON::ApplyStrValue(window, pSet->Window.Title, "Title");
		std::vector<ivec2> resolutions;
		JSON::Value* jval = (*window)["Resolutions"];
		if (jval)
		{
			for (JSON::Value* res : jval->arr()->value)
			{
				ivec2 vec;
				if (JSON::ArrayVector(res, vec))resolutions.push_back(vec);
			}
		}
		uint32 resIdx;
		if (JSON::ApplyNumValue(window, resIdx, pSet->Window.Fullscreen ? "Fullscreen Resolution" : "Windowed Resolution"))
		{
			if (resIdx < (uint32)resolutions.size()) 
				pSet->Window.Resize(resolutions[resIdx].x, resolutions[resIdx].y, false);
		}
	}
	SceneManager::GetInstance();//Initialize SceneManager
	AddScenes();
	JSON::String* initialScene = (*root)["start scene"]->str();
	if (initialScene)
	{
		SceneManager::GetInstance()->SetActiveGameScene(initialScene->value);
	}
}

void AbstractFramework::InitializeWindow()
{
	Settings* pSet = Settings::GetInstance();
	PerformanceInfo::GetInstance();//Initialize performance measurment #todo: disable for shipped project?
	InputManager::GetInstance()->Init();//init input manager

	//Create Window
	uint32 WindowFlags = SDL_WINDOW_OPENGL;
	if(pSet->Window.Fullscreen)
	{
		WindowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	else
	{
		WindowFlags |= SDL_WINDOW_RESIZABLE;
	}

#ifdef EDITOR
	ivec2 dim = pSet->Window.EditorDimensions;
#else
	ivec2 dim = pSet->Window.Dimensions;
#endif
	pSet->Window.pWindow = SDL_CreateWindow(pSet->Window.Title.c_str(), 
											 SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
											 dim.x, dim.y, 
											 WindowFlags);
	
	if(pSet->Window.pWindow == NULL)
	{
		quit_SDL_error("Couldn't set video mode");
	}

	//OpenGl context creation
	m_GlContext = SDL_GL_CreateContext(pSet->Window.pWindow);
	if (m_GlContext == NULL)quit_SDL_error("Failed to create OpenGL context");

	// Use v-sync
	SDL_GL_SetSwapInterval(1);
}

void AbstractFramework::InitializeUtilities()
{
	FreeImage_Initialise();
	#ifdef _DEBUG
		FreeImage_SetOutputMessage(FreeImageErrorHandler);
	#endif
}

void AbstractFramework::BindOpenGL()
{
	// Check OpenGL properties and create open gl function pointers
	std::cout << "OpenGL loaded" << std::endl << std::endl;
	gladLoadGLLoader(SDL_GL_GetProcAddress);
	std::cout << "Vendor: \t" << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer:\t" << glGetString(GL_RENDERER) << std::endl;
	std::cout << "Version:\t" << glGetString(GL_VERSION) << std::endl << std::endl;
}

void AbstractFramework::InitializeDebug()
{
	Logger::Initialize();
	// Enable the debug callback -- is now done by logger
}

void AbstractFramework::InitializeGame()
{
	//Initialize Managers
	ContentManager::Initialize();

	RenderPipeline::GetInstance()->Initialize();

#ifdef EDITOR
	Editor::GetInstance()->Initialize();
#endif

}

void AbstractFramework::GameLoop()
{
	while (true)
	{
		InputManager::GetInstance()->UpdateEvents();
		if (InputManager::GetInstance()->IsExitRequested())return;

		std::vector<AbstractScene*> activeScenes;
		if (SceneManager::GetInstance()->GetActiveScene())
		{
			activeScenes.push_back(SceneManager::GetInstance()->GetActiveScene());
		}

		Update();
		//******
		//UPDATE

	#ifdef EDITOR
		Editor::GetInstance()->Update();
	#endif

		SceneManager::GetInstance()->Update();
		//****
		//DRAW

	#ifdef EDITOR
		RenderPipeline::GetInstance()->Draw(activeScenes, Editor::GetInstance()->GetSceneTarget());
		Editor::GetInstance()->Draw();
	#else

		// #note: currently only one scene but could be expanded for nested scenes
		RenderPipeline::GetInstance()->Draw(activeScenes, 0);

	#endif
		RenderPipeline::GetInstance()->SwapBuffers();
	}
}

void AbstractFramework::ClearTarget()
{
	// Clear the screen to white
	STATE->SetClearColor(vec4(1));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}