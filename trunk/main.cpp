/* main.cpp
Copyright (c) 2014 by Michael Zahniser

Main function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Files.h"
#include "FrameTimer.h"
#include "GameData.h"
#include "MainPanel.h"
#include "MenuPanel.h"
#include "Panel.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "UI.h"

#ifdef __APPLE__
#include <OpenGL/GL3.h>
#else
#include <GL/glew.h>
#endif

#include <SDL2/SDL.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;



int main(int argc, char *argv[])
{
	for(const char *const *it = argv + 1; *it; ++it)
	{
		string arg = *it;
		if(arg == "-h" || arg == "--help")
		{
			cerr << endl;
			cerr << "Command line options:" << endl;
			cerr << "    -h, --help: print this help message." << endl;
			cerr << "    -v, --version: print version information." << endl;
			cerr << "    -l, --load: display CPU and GPU load." << endl;
			cerr << "    -t, --table: print table of ship statistics." << endl;
			cerr << "    -w, --weapons: print table of weapon statistics." << endl;
			cerr << "    -r, --resources <path>: load resources from given directory." << endl;
			cerr << "    -c, --config <path>: save user's files to given directory." << endl;
			cerr << endl;
			cerr << "Report bugs to: mzahniser@gmail.com" << endl;
			cerr << "Home page: <http://endless-sky.googlecode.com>" << endl;
			cerr << endl;
			return 0;
		}
		else if(arg == "-v" || arg == "--version")
		{
			cerr << endl;
			cerr << "Endless Sky 0.5.0" << endl;
			cerr << "License GPLv3+: GNU GPL version 3 or later: <http://gnu.org/licenses/gpl.html>" << endl;
			cerr << "This is free software: you are free to change and redistribute it." << endl;
			cerr << "There is NO WARRANTY, to the extent permitted by law." << endl;
			cerr << endl;
			return 0;
		}
	}
	PlayerInfo player;
	
	try {
		SDL_Init(SDL_INIT_VIDEO);
		
		// Begin loading the game data.
		GameData::BeginLoad(argv);
		Audio::Init();
		
		player.LoadRecent();
		player.ApplyChanges();
		
		// Check how big the window can be.
		SDL_DisplayMode mode;
		if(SDL_GetCurrentDisplayMode(0, &mode))
		{
			cerr << "Unable to query monitor resolution!" << endl;
			return 1;
		}
		
		DataFile prefs(Files::Config() + "preferences.txt");
		Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
		for(const DataNode &node : prefs)
		{
			if(node.Token(0) == "fullscreen")
				flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
			else if(node.Token(0) == "window size" && node.Size() >= 3)
				Screen::Set(node.Value(1), node.Value(2));
			else if(node.Token(0) == "volume" && node.Size() >= 2)
				Audio::SetVolume(node.Value(1));
		}
		
		// Make the window just slightly smaller than the monitor resolution.
		int maxWidth = mode.w;
		int maxHeight = mode.h;
		// Restore this after toggling fullscreen.
		int restoreWidth = 0;
		int restoreHeight = 0;
		if(maxWidth < 640 || maxHeight < 480)
		{
			cerr << "Monitor resolution is too small!" << endl;
			return 1;
		}
		if(Screen::Width() && Screen::Height())
		{
			if(flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
			{
				restoreWidth = Screen::Width();
				restoreHeight = Screen::Height();
				Screen::Set(maxWidth, maxHeight);
			}
		}
		else
			Screen::Set(maxWidth - 100, maxHeight - 100);
		
		// Create the window.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		
		SDL_Window *window = SDL_CreateWindow("Endless Sky",
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
			Screen::Width(), Screen::Height(), flags);
		if(!window)
		{
			cerr << "Unable to create window!" << endl;
			return 1;
		}
		
		SDL_GLContext context = SDL_GL_CreateContext(window);
		if(!context)
		{
			cerr << "Unable to create OpenGL context!" << endl;
			return 1;
		}
		
		if(SDL_GL_MakeCurrent(window, context))
		{
			cerr << "Unable to set the current OpenGL context!" << endl;
			return 1;
		}
		SDL_GL_SetSwapInterval(1);
		
		// Initialize GLEW.
#ifndef __APPLE__
		glewExperimental = GL_TRUE;
		if(glewInit() != GLEW_OK)
		{
			cerr << "Unable to initialize GLEW!" << endl;
			return 1;
		}
#endif
		
		glClearColor(0.f, 0.f, 0.0f, 1.f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		
		GameData::LoadShaders();
		
		
		UI gamePanels;
		UI menuPanels;
		menuPanels.Push(new MenuPanel(player, gamePanels));
		
		string swizzleName = "GL_ARB_texture_swizzle";
#ifndef __APPLE__
		const char *extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
		if(!strstr(extensions, swizzleName.c_str()))
#else
		bool hasSwizzle = false;
		GLint extensionCount;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
		for(GLint i = 0; i < extensionCount && !hasSwizzle; ++i)
		{
			const char *extension = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
			hasSwizzle = (extension && extension == swizzleName);
		}
		if(!hasSwizzle)
#endif
			menuPanels.Push(new Dialog(
				"Note: your computer does not support the \"texture swizzling\" OpenGL feature, "
				"which Endless Sky uses to draw ships in different colors depending on which "
				"government they belong to. So, all human ships will be the same color, which "
				"may be confusing. Consider upgrading your graphics driver (or your OS)."));
		
		FrameTimer timer(60);
		while(!menuPanels.IsDone())
		{
			// Handle any events that occurred in this frame.
			SDL_Event event;
			while(SDL_PollEvent(&event))
			{
				UI &activeUI = (menuPanels.IsEmpty() ? gamePanels : menuPanels);
				
				// The caps lock key slows the game down (to make it easier to
				// see and debug things that are happening quickly).
				if((event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
						&& event.key.keysym.sym == SDLK_CAPSLOCK)
				{
					timer.SetFrameRate((event.key.keysym.mod & KMOD_CAPS) ? 10 : 60);
				}
				else if(event.type == SDL_KEYDOWN && menuPanels.IsEmpty()
						&& event.key.keysym.sym == GameData::Keys().Get(Key::MENU))
				{
					menuPanels.Push(shared_ptr<Panel>(
						new MenuPanel(player, gamePanels)));
				}
				else if(event.type == SDL_QUIT)
				{
					menuPanels.Quit();
				}
				else if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
				{
					Screen::Set(event.window.data1 & ~1, event.window.data2 & ~1);
					SDL_SetWindowSize(window, Screen::Width(), Screen::Height());
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
				else if(activeUI.Handle(event))
				{
					// No need to do anything more!
				}
				else if(event.type == SDL_KEYDOWN
						&& event.key.keysym.sym == GameData::Keys().Get(Key::FULLSCREEN))
				{
					if(restoreWidth)
					{
						SDL_SetWindowFullscreen(window, 0);
						Screen::Set(restoreWidth, restoreHeight);
						SDL_SetWindowSize(window, Screen::Width(), Screen::Height());
						restoreWidth = 0;
						restoreHeight = 0;
					}
					else
					{
						restoreWidth = Screen::Width();
						restoreHeight = Screen::Height();
						Screen::Set(maxWidth, maxHeight);
						SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
					}
					glViewport(0, 0, Screen::Width(), Screen::Height());
				}
			}
			
			// Tell all the panels to step forward, then draw them.
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).StepAll();
			Audio::Step();
			// That may have cleared out the menu, in which case we should draw
			// the game panels instead:
			(menuPanels.IsEmpty() ? gamePanels : menuPanels).DrawAll();
			
			SDL_GL_SwapWindow(window);
			timer.Wait();
		}
		
		// If you quit while landed on a planet, save the game.
		if(player.GetPlanet())
			player.Save();
		
		DataWriter out(Files::Config() + "preferences.txt");
		out.Write("volume", Audio::Volume());
		bool isFullscreen = (restoreWidth != 0);
		if(isFullscreen)
		{
			out.Write("window size", restoreWidth, restoreHeight);
			out.Write("fullscreen");
		}
		else
			out.Write("window size", Screen::Width(), Screen::Height());
		
		Audio::Quit();
		SDL_GL_DeleteContext(context);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
	catch(const runtime_error &error)
	{
		cerr << error.what() << endl;
	}
	
	return 0;
}
