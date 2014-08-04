/* GameData.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameData.h"

#include "DataFile.h"
#include "DotShader.h"
#include "Files.h"
#include "FillShader.h"
#include "FontSet.h"
#include "LineShader.h"
#include "PointerShader.h"
#include "OutlineShader.h"
#include "SpriteSet.h"
#include "SpriteShader.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

using namespace std;

namespace {
	Set<Color> colors;
	Set<Conversation> conversations;
	Set<Effect> effects;
	Set<Fleet> fleets;
	Set<Government> governments;
	Set<Interface> interfaces;
	Set<Mission> missions;
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
	
	SpriteQueue spriteQueue;
	
	map<const Sprite *, pair<string, string>> deferred;
	
	bool showLoad;
}



void GameData::BeginLoad(const char * const *argv)
{
	showLoad = false;
	bool printTable = false;
	for(const char * const *it = argv + 1; *it; ++it)
	{
		if((*it)[0] == '-')
		{
			string arg = *it;
			if(arg == "-l" || arg == "--load")
				showLoad = true;
			if(arg == "-t" || arg == "--table")
				printTable = true;
			continue;
		}
	}
	Files::Init(argv);
	
	// Now, read all the images in all the path directories. For each unique
	// name, only remember one instance, letting things on the higher priority
	// paths override the default images.
	vector<string> imageFiles = Files::RecursiveList(Files::Images());
	map<string, string> images;
	for(const string &path : imageFiles)
		LoadImage(path, images);
	
	// From the name, strip out any frame number, plus the extension.
	for(const auto &it : images)
	{
		string name = Name(it.first);
		if(name.substr(0, 5) == "land/")
			deferred[SpriteSet::Get(name)] = pair<string, string>(name, it.second);
		else
			spriteQueue.Add(name, it.second);
	}
	
	// Iterate through the paths starting with the last directory given. That
	// is, things in folders near the start of the path have the ability to
	// override things in folders later in the path.
	vector<string> dataFiles = Files::RecursiveList(Files::Data());
	for(const string &path : dataFiles)
		LoadFile(path);
	
	// Now that all the stars are loaded, update the neighbor lists.
	for(auto &it : systems)
		it.second.UpdateNeighbors(systems);
	// And, update the ships with the outfits we've now finished loading.
	for(auto &it : ships)
		it.second.FinishLoading();
	
	if(printTable)
	{
		cout << "model" << '\t' << "cost" << '\t' << "shields" << '\t' << "hull" << '\t'
			<< "mass" << '\t' << "crew" << '\t' << "cargo" << '\t' << "bunks" << '\t'
			<< "fuel" << '\t' << "outfit" << '\t' << "weapon" << '\t' << "engine" << '\t'
			<< "speed" << '\t' << "accel" << '\t' << "turn" << '\t'
			<< "e_gen" << '\t' << "e_use" << '\t' << "h_gen" << '\t' << "h_max" << '\n';
		for(auto &it : ships)
		{
			const Ship &ship = it.second;
			cout << it.first << '\t';
			cout << ship.Cost() << '\t';
			
			const Outfit &attributes = ship.Attributes();
			cout << attributes.Get("shields") << '\t';
			cout << attributes.Get("hull") << '\t';
			cout << attributes.Get("mass") << '\t';
			cout << attributes.Get("required crew") << '\t';
			cout << attributes.Get("cargo space") << '\t';
			cout << attributes.Get("bunks") << '\t';
			cout << attributes.Get("fuel capacity") << '\t';
			
			cout << attributes.Get("outfit space") << '\t';
			cout << attributes.Get("weapon capacity") << '\t';
			cout << attributes.Get("engine capacity") << '\t';
			cout << 60. * attributes.Get("thrust") / attributes.Get("drag") << '\t';
			cout << 3600. * attributes.Get("thrust") / attributes.Get("mass") << '\t';
			cout << 60. * attributes.Get("turn") / attributes.Get("mass") << '\t';
			
			double energy = attributes.Get("thrusting energy")
				+ attributes.Get("turning energy");
			double heat = attributes.Get("heat generation") - attributes.Get("cooling")
				+ attributes.Get("thrusting heat") + attributes.Get("turning heat");
			for(const auto &oit : ship.Outfits())
				if(oit.first->IsWeapon())
				{
					double reload = oit.first->WeaponGet("reload");
					energy += oit.second * oit.first->WeaponGet("firing energy") / reload;
					heat += oit.second * oit.first->WeaponGet("firing heat") / reload;
				}
			cout << 60. * attributes.Get("energy generation") << '\t';
			cout << 60. * energy << '\t';
			cout << 60. * heat << '\t';
			// Maximum heat is 100 degrees per ton. Bleed off rate is 1/1000
			// per 60th of a second, so:
			cout << 60. * ship.Mass() * .1 * attributes.Get("heat dissipation") << '\n';
		}
		cout.flush();
	}
}



void GameData::LoadShaders()
{
	FontSet::Add(Files::Images() + "font/ubuntu14r.png", 14);
	FontSet::Add(Files::Images() + "font/ubuntu18r.png", 18);
	
	// Load the key settings.
	defaultKeys.Load(Files::Resources() + "keys.txt");
	string keysPath = Files::Config() + "keys.txt";
	keys = defaultKeys;
	keys.Load(keysPath);
	
	DotShader::Init();
	FillShader::Init();
	LineShader::Init();
	OutlineShader::Init();
	PointerShader::Init();
	SpriteShader::Init();
	
	background.Init(16384, 4096);
}



double GameData::Progress()
{
	return spriteQueue.Progress();
}



// Begin loading a sprite that was previously deferred. Currently this is
// done with all landscapes to speed up the program's startup.
void GameData::Preload(const Sprite *sprite)
{
	auto it = deferred.find(sprite);
	if(it != deferred.end())
	{
		spriteQueue.Add(it->second.first, it->second.second);
		deferred.erase(it);
	}
}



void GameData::FinishLoading()
{
	spriteQueue.Finish();
}



// Revert any changes that have been made to the universe.
void GameData::Revert()
{
	// Currently it is not possible to change anything, so nothing needs to be
	// reverted. Eventually, we will need to save a copy of the systems,
	// planets, and sales, and maybe other things as well.
}



void GameData::SetDate(const Date &date)
{
	for(auto &it : systems)
		it.second.SetDate(date);
}



const Set<Color> &GameData::Colors()
{
	return colors;
}



const Set<Conversation> &GameData::Conversations()
{
	return conversations;
}



const Set<Effect> &GameData::Effects()
{
	return effects;
}



const Set<Fleet> &GameData::Fleets()
{
	return fleets;
}



const Set<Government> &GameData::Governments()
{
	return governments;
}



const Set<Interface> &GameData::Interfaces()
{
	return interfaces;
}



const Set<Mission> &GameData::Missions()
{
	return missions;
}



const Set<Outfit> &GameData::Outfits()
{
	return outfits;
}



const Set<Planet> &GameData::Planets()
{
	return planets;
}



const Set<Ship> &GameData::Ships()
{
	return ships;
}



const Set<ShipName> &GameData::ShipNames()
{
	return shipNames;
}



const Set<System> &GameData::Systems()
{
	return systems;
}



const vector<Trade::Commodity> &GameData::Commodities()
{
	return trade.Commodities();
}



const StarField &GameData::Background()
{
	return background;
}



// Get the mapping of keys to commands.
const Key &GameData::Keys()
{
	return keys;
}



void GameData::SetKey(Key::Command command, int key)
{
	keys.Set(command, key);
}



const Key &GameData::DefaultKeys()
{
	return defaultKeys;
}



bool GameData::ShouldShowLoad()
{
	return showLoad;
}



void GameData::LoadFile(const string &path)
{
	// This is an ordinary file. Check to see if it is an image.
	if(path.length() < 4 || path.compare(path.length() - 4, 4, ".txt"))
		return;
	
	DataFile data(path);
	
	for(const DataNode &node : data)
	{
		const string &key = node.Token(0);
		if(key == "color" && node.Size() >= 6)
			colors.Get(node.Token(1))->Load(
				node.Value(2), node.Value(3), node.Value(4), node.Value(5));
		else if(key == "conversation" && node.Size() >= 2)
			conversations.Get(node.Token(1))->Load(node);
		else if(key == "effect" && node.Size() >= 2)
			effects.Get(node.Token(1))->Load(node);
		else if(key == "fleet" && node.Size() >= 2)
			fleets.Get(node.Token(1))->Load(node);
		else if(key == "government" && node.Size() >= 2)
			governments.Get(node.Token(1))->Load(node);
		else if(key == "interface")
			interfaces.Get(node.Token(1))->Load(node);
		else if(key == "mission")
			missions.Get(node.Token(1))->Load(node);
		else if(key == "outfit" && node.Size() >= 2)
			outfits.Get(node.Token(1))->Load(node);
		else if(key == "outfitter" && node.Size() >= 2)
			outfitSales.Get(node.Token(1))->Load(node, outfits);
		else if(key == "planet" && node.Size() >= 2)
			planets.Get(node.Token(1))->Load(node, shipSales, outfitSales);
		else if(key == "ship" && node.Size() >= 2)
		{
			// Allow multiple named variants of the same ship model.
			const string &name = node.Token((node.Size() > 2) ? 2 : 1);
			ships.Get(name)->Load(node);
		}
		else if(key == "shipyard" && node.Size() >= 2)
			shipSales.Get(node.Token(1))->Load(node, ships);
		else if(key == "name" && node.Size() >= 2)
			shipNames.Get(node.Token(1))->Load(node);
		else if(key == "system" && node.Size() >= 2)
			systems.Get(node.Token(1))->Load(node, planets);
		else if(key == "trade")
			trade.Load(node);
	}
}



void GameData::LoadImage(const string &path, map<string, string> &images)
{
	bool isJpg = !path.compare(path.length() - 4, 4, ".jpg");
	bool isPng = !path.compare(path.length() - 4, 4, ".png");
	
	// This is an ordinary file. Check to see if it is an image.
	if(isJpg || isPng)
		images[path.substr(Files::Images().length())] = path;
}



string GameData::Name(const string &path)
{
	// The path always ends in a three-letter extension, ".png" or ".jpg".
	int end = path.length() - 4;
	while(end--)
		if(path[end] < '0' || path[end] > '9')
			break;
	
	// This should never happen, but just in case someone creates a file named
	// "images/123.jpg" or something:
	if(end < 0)
		end = path.length() - 4;
	else if(path[end] != '-' && path[end] != '~' && path[end] != '+')
		end = path.length() - 4;
	
	return path.substr(0, end);
}
