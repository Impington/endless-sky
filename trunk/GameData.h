/* GameData.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GAME_DATA_H_
#define GAME_DATA_H_

#include "Color.h"
#include "Conversation.h"
#include "Effect.h"
#include "Fleet.h"
#include "Galaxy.h"
#include "GameEvent.h"
#include "Government.h"
#include "Interface.h"
#include "Mission.h"
#include "Outfit.h"
#include "Phrase.h"
#include "Planet.h"
#include "Politics.h"
#include "Sale.h"
#include "Set.h"
#include "Ship.h"
#include "SpriteQueue.h"
#include "StarField.h"
#include "System.h"
#include "Trade.h"

#include <map>
#include <string>

class Date;
class DataNode;



// Class storing all the data used in the game: sprites, data files, etc. This
// data is globally accessible, but can only be modified in certain ways.
// Events that occur over the course of the game may change the state of the
// game data, so we must revert to the initial state when loading a new player
// and then apply whatever changes have happened in that particular player's
// universe.
class GameData {
public:
	static void BeginLoad(const char * const *argv);
	static void LoadShaders();
	static double Progress();
	// Begin loading a sprite that was previously deferred. Currently this is
	// done with all landscapes to speed up the program's startup.
	static void Preload(const Sprite *sprite);
	static void FinishLoading();
	
	// Revert any changes that have been made to the universe.
	static void Revert();
	static void SetDate(const Date &date);
	// Apply the given change to the universe.
	static void Change(const DataNode &node);
	
	static const Set<Color> &Colors();
	static const Set<Conversation> &Conversations();
	static const Set<Effect> &Effects();
	static const Set<GameEvent> &Events();
	static const Set<Fleet> &Fleets();
	static const Set<Galaxy> &Galaxies();
	static const Set<Government> &Governments();
	static const Set<Interface> &Interfaces();
	static const Set<Mission> &Missions();
	static const Set<Outfit> &Outfits();
	static const Set<Phrase> &Phrases();
	static const Set<Planet> &Planets();
	static const Set<Ship> &Ships();
	static const Set<System> &Systems();
	
	static const Government *PlayerGovernment();
	static Politics &GetPolitics();
	
	static const std::vector<Trade::Commodity> &Commodities();
	
	static const StarField &Background();
	
	
private:
	static void LoadFile(const std::string &path);
	static void LoadImage(const std::string &path, std::map<std::string, std::string> &images);
	static std::string Name(const std::string &path);
	
	static void PrintShipTable();
	static void PrintWeaponTable();
};



#endif
