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
#include "Government.h"
#include "Interface.h"
#include "Key.h"
#include "Outfit.h"
#include "Planet.h"
#include "Sale.h"
#include "Set.h"
#include "Ship.h"
#include "ShipName.h"
#include "SpriteQueue.h"
#include "StarField.h"
#include "System.h"
#include "Trade.h"

#include <map>
#include <string>



// Class storing all the data used in the game: sprites, data files, etc.
class GameData {
public:
	void BeginLoad(const char * const *argv);
	void LoadShaders();
	double Progress() const;
	
	const Set<Conversation> &Conversations() const;
	const Set<Effect> &Effects() const;
	const Set<Fleet> &Fleets() const;
	const Set<Government> &Governments() const;
	const Set<Interface> &Interfaces() const;
	const Set<Outfit> &Outfits() const;
	const Set<Planet> &Planets() const;
	const Set<Ship> &Ships() const;
	const Set<ShipName> &ShipNames() const;
	const Set<System> &Systems() const;
	
	const std::vector<Trade::Commodity> &Commodities() const;
	
	const StarField &Background() const;
	
	// Get the mapping of keys to commands.
	const Key &Keys() const;
	Key &Keys();
	const Key &DefaultKeys() const;
	
	const std::string &ResourcePath() const;
	
	bool ShouldShowLoad() const;
	
	
private:
	void FindFiles(const std::string &path);
	static void FindImages(const std::string &path, int start, std::map<std::string, std::string> &images);
	static std::string Name(const std::string &path);
	
	
private:
	Set<Color> colors;
	Set<Conversation> conversations;
	Set<Effect> effects;
	Set<Fleet> fleets;
	Set<Government> governments;
	Set<Interface> interfaces;
	Set<Outfit> outfits;
	Set<Planet> planets;
	Set<Ship> ships;
	Set<ShipName> shipNames;
	Set<System> systems;
	
	Set<Sale<Ship>> shipSales;
	Set<Sale<Outfit>> outfitSales;
	
	Trade trade;
	
	Key keys;
	Key defaultKeys;
	
	StarField background;
	
	SpriteQueue queue;
	std::string basePath;
	
	bool showLoad;
};



#endif
