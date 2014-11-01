/* LoadPanel.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LOAD_PANEL_H_
#define LOAD_PANEL_H_

#include "Panel.h"

#include "SavedGame.h"

#include <map>
#include <string>
#include <vector>

class PlayerInfo;



// UI panel for loading and saving games. The game is automatically saved when
// the player takes off from any planet, so if they want to be able to go back
// to a previous game state they must save a "snapshot" of that state.
class LoadPanel : public Panel {
public:
	LoadPanel(PlayerInfo &player, UI &gamePanels);
	
	virtual void Draw() const;
	
	// New player "conversation" callback.
	void OnCallback(int value);
	
	
protected:
	// Only override the ones you need; the default action is to return false.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod);
	virtual bool Click(int x, int y);
	virtual bool Drag(int dx, int dy);
	virtual bool Scroll(int dx, int dy);
	
	
private:
	void UpdateLists();
	void DeletePilot();
	void DeleteSave();
	
	
private:
	PlayerInfo &player;
	SavedGame loadedInfo;
	UI &gamePanels;
	
	std::map<std::string, std::vector<std::string>> files;
	std::string selectedPilot;
	std::string selectedFile;
	
	bool sideHasFocus;
	int sideScroll = 0;
	int centerScroll = 0;
};



#endif
