/* HailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "HailPanel.h"

#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Sprite.h"
#include "SpriteShader.h"
#include "UI.h"
#include "WrappedText.h"

#include <algorithm>

using namespace std;



HailPanel::HailPanel(PlayerInfo &player, const std::shared_ptr<Ship> &ship)
	: player(player), ship(ship),
	sprite(ship->GetSprite().GetSprite()), unit(2. * ship->Unit())
{
	const Government *gov = ship->GetGovernment();
	header = gov->GetName() + " ship \"" + ship->Name() + "\":";
	
	if(gov->IsEnemy())
	{
		SetBribe(gov->GetBribeFraction());
		if(bribe)
			message = "If you want us to leave you alone, it'll cost you "
				+ Format::Number(bribe) + " credits.";
	}
	else if(!ship->GetPersonality().IsSurveillance() && !ship->IsFighter())
	{
		// Is the player in any need of assistance?
		const Ship *playerShip = player.GetShip();
		// Check if the player is out of fuel.
		if(!playerShip->JumpsRemaining())
		{
			playerNeedsHelp = true;
			canGiveFuel = ship->CanRefuel(*playerShip);
		}
		// Check if the player is disabled.
		if(playerShip->IsDisabled())
		{
			playerNeedsHelp = true;
			canRepair = true;
		}
		
		if(canGiveFuel || canRepair)
			message = "Looks like you've gotten yourself into a bit of trouble. "
				"Would you like us to ";
		if(canGiveFuel && canRepair)
			message += "patch you up and give you some fuel?";
		else if(canGiveFuel)
			message += "give you some fuel?";
		else if(canRepair)
			message += "patch you up?";
	}
	
	if(message.empty())
		message = ship->GetHail();
}



HailPanel::HailPanel(PlayerInfo &player, const StellarObject *object)
	: player(player), planet(object->GetPlanet()),
	sprite(object->GetSprite().GetSprite()), unit(object->Position().Unit())
{
	const Government *gov = player.GetSystem()->GetGovernment();
	if(planet)
		header = gov->GetName() + " planet \"" + planet->Name() + "\":";
	
	if(player.GetShip())
	{
		for(const Mission &mission : player.Missions())
			if(mission.HasClearance(planet) && mission.ClearanceMessage() != "auto"
					&& mission.HasFullClearance())
			{
				planet->Bribe();
				message = mission.ClearanceMessage();
				return;
			}
		if(planet->CanLand())
			message = "You are cleared to land, " + player.GetShip()->Name() + ".";
		else
		{
			SetBribe(planet->GetBribeFraction());
			if(bribe)
				message = "If you want to land here, it'll cost you "
					+ Format::Number(bribe) + " credits.";
			else
				message = "I'm afraid we can't permit you to land here.";
		}
	}
}



void HailPanel::Draw() const
{
	DrawBackdrop();
	
	Information interfaceInfo;
	interfaceInfo.SetString("header", header);
	if(ship)
	{
		bool isEnemy = ship->GetGovernment()->IsEnemy();
		if(isEnemy)
		{
			interfaceInfo.SetCondition("can bribe");
			interfaceInfo.SetCondition("cannot assist");
		}
		else
			interfaceInfo.SetCondition("can assist");
	}
	else
	{
		if(!planet->CanLand())
			interfaceInfo.SetCondition("can bribe");
		else
			interfaceInfo.SetCondition("cannot bribe");
		
		interfaceInfo.SetCondition("can dominate");
	}
	
	const Interface *interface = GameData::Interfaces().Get("hail panel");
	interface->Draw(interfaceInfo);
	
	// Draw the sprite, rotated, scaled, and swizzled as necessary.
	int swizzle = ship ? ship->GetGovernment()->GetSwizzle() : 0;
	uint32_t tex = sprite->Texture();
	
	float pos[2] = {-170.f, -10.f};
	
	double zoom = min(1., 200. / max(sprite->Width(), sprite->Height()));
	Point uw = unit * (sprite->Width() * zoom);
	Point uh = unit * (sprite->Height() * zoom);
	float tr[4] = {
		static_cast<float>(-uw.Y()), static_cast<float>(uw.X()),
		static_cast<float>(-uh.X()), static_cast<float>(-uh.Y())};
	
	SpriteShader::Bind();
	SpriteShader::Add(tex, tex, pos, tr, swizzle);
	SpriteShader::Unbind();
	
	// Draw the current message.
	WrappedText wrap;
	wrap.SetAlignment(WrappedText::JUSTIFIED);
	wrap.SetWrapWidth(330);
	wrap.SetFont(FontSet::Get(14));
	wrap.Wrap(message);
	wrap.Draw(Point(-50., -50.), *GameData::Colors().Get("medium"));
}



bool HailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	bool shipIsEnemy = ship && ship->GetGovernment()->IsEnemy();
	if(key == 'd' || key == SDLK_RETURN)
		GetUI()->Pop(this);
	else if(key == 'a' || key == 't' || key == 'h')
	{
		// TODO: handle dominating planets.
		if(planet)
		{
			message = "Please don't joke about that sort of thing.";
			return true;
		}
		else if(shipIsEnemy)
			return false;
		if(playerNeedsHelp)
		{
			if(canGiveFuel || canRepair)
			{
				ship->SetShipToAssist(player.Ships().front());
				message = "Hang on, we'll be there in a minute.";
			}
			else
				message = "Sorry, but if we give you fuel we won't have enough"
					" to make it to the next system.";
		}
		else if(ship)
			message = "You don't seem to be in need of repairs or fuel assistance.";
	}
	else if(key == 'b' || key == 'o')
	{
		// Make sure it actually makes sense to bribe this ship.
		if(ship && !shipIsEnemy)
			return true;
		
		if(bribe)
		{
			if(ship)
			{
				ship->GetGovernment()->Bribe();
				Messages::Add("You bribed a " + ship->GetGovernment()->GetName() + " ship "
					+ Format::Number(bribe) + " credits to refrain from attacking you today.");
			}
			else
			{
				planet->Bribe();
				Messages::Add("You bribed the authorities on " + planet->Name() + " "
					+ Format::Number(bribe) + " credits to permit you to land.");
			}
			
			// TODO: handle landing bribes to planets.
			player.Accounts().AddCredits(-bribe);
			message = "It's a pleasure doing business with you.";
			bribe = 0;
		}
		else
			message = "I do not want your money.";
	}
	
	return true;
}



bool HailPanel::Click(int x, int y)
{
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("hail panel");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key)
			return DoKey(key);
	}
	
	return true;
}



void HailPanel::SetBribe(double scale)
{
	// Find the total value of your fleet.
	int value = 0;
	for(const shared_ptr<Ship> &it : player.Ships())
		value += it->Cost();
	
	bribe = 1000 * static_cast<int>(sqrt(value) * scale);
	if(scale && !bribe)
		bribe = 1000;
}
