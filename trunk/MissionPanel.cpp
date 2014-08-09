/* MissionPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/



#include "MissionPanel.h"

#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "GameData.h"
#include "Information.h"
#include "Interface.h"
#include "Mission.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "UI.h"

#include <map>

using namespace std;

namespace {
	static const int SIDE_WIDTH = 280;
}



MissionPanel::MissionPanel(PlayerInfo &player)
	: MapPanel(player, -4),
	available(player.AvailableJobs()),
	accepted(player.Missions()),
	special(player.SpecialMissions()),
	availableIt(player.AvailableJobs().begin()),
	acceptedIt(player.AvailableJobs().empty() ? accepted.begin() : accepted.end()),
	specialIt(special.end()),
	availableScroll(0), acceptedScroll(0), dragSide(0)
{
	// Center the system slightly above the center of the screen because the
	// lower panel is taking up more space than the upper one.
	center = Point(0., -80.) - selectedSystem->Position();
	
	wrap.SetWrapWidth(380);
	wrap.SetFont(FontSet::Get(14));
	wrap.SetAlignment(WrappedText::JUSTIFIED);
}



void MissionPanel::Draw() const
{
	MapPanel::Draw();
	
	DrawSelectedSystem();
	Point pos = DrawPanel(
		Screen::TopLeft() + Point(0., -availableScroll),
		"Missions available here:",
		available.size());
	DrawList(available, pos);
	
	pos = DrawPanel(
		Screen::TopRight() + Point(-SIDE_WIDTH, -acceptedScroll),
		"Your current missions:",
		player.SpecialMissions().size() + accepted.size());
	pos = DrawList(special, pos);
	DrawList(accepted, pos);
	
	DrawMissionInfo();
}



// Only override the ones you need; the default action is to return false.
bool MissionPanel::KeyDown(SDL_Keycode key, Uint16 mod)
{
	if(key == 'd')
		GetUI()->Pop(this);
	else if(key == 'a')
	{
		if(CanAccept())
		{
			const Mission &toAccept = *availableIt;
			++availableIt;
			player.AcceptJob(toAccept);
			if(availableIt == available.end() && !available.empty())
				--availableIt;
		}
		else if(acceptedIt != accepted.end())
		{
			const Mission &toAbort = *acceptedIt;
			++acceptedIt;
			player.AbortMission(toAbort);
			if(acceptedIt == accepted.end() && !accepted.empty())
				--acceptedIt;
		}
		else if(specialIt != special.end())
		{
			const Mission &toAbort = **specialIt;
			++specialIt;
			player.AbortMission(toAbort);
			if(specialIt == special.end() && !special.empty())
				--specialIt;
		}
		return true;
	}
	else if(key == SDLK_LEFT && availableIt == available.end())
	{
		acceptedIt = accepted.end();
		specialIt = special.end();
		availableIt = available.begin();
	}
	else if(key == SDLK_RIGHT && acceptedIt == accepted.end() && specialIt == special.end())
	{
		availableIt = available.end();
		specialIt = special.begin();
		if(specialIt == special.end())
			acceptedIt = accepted.begin();
		else
			acceptedIt = accepted.end();
	}
	else if(key == SDLK_UP)
	{
		if(availableIt != available.end())
		{
			if(availableIt == available.begin())
				availableIt = available.end();
			--availableIt;
		}
		else if(acceptedIt != accepted.end())
		{
			if(acceptedIt == accepted.begin())
			{
				specialIt = special.end();
				acceptedIt = accepted.end();
				if(special.empty())
					--acceptedIt;
				else
					--specialIt;
			}
			else
				--acceptedIt;
		}
		else if(specialIt != special.end())
		{
			if(specialIt == special.begin())
			{
				specialIt = special.end();
				acceptedIt = accepted.end();
				if(accepted.empty())
					--specialIt;
				else
					--acceptedIt;
			}
			else
				--specialIt;
		}
	}
	else if(key == SDLK_DOWN)
	{
		if(availableIt != available.end())
		{
			++availableIt;
			if(availableIt == available.end())
				availableIt = available.begin();
		}
		else if(acceptedIt != accepted.end())
		{
			++acceptedIt;
			if(acceptedIt == accepted.end())
			{
				if(special.empty())
					acceptedIt = accepted.begin();
				else
					specialIt = special.begin();
			}
		}
		else if(specialIt != special.end())
		{
			++specialIt;
			if(specialIt == special.end())
			{
				if(accepted.empty())
					specialIt = special.begin();
				else
					acceptedIt = accepted.begin();
			}
		}
	}
	else
		return false;
	
	if(availableIt != available.end())
		Select(availableIt->Destination()->GetSystem());
	else if(acceptedIt != accepted.end())
		Select(acceptedIt->Destination()->GetSystem());
	else if(specialIt != special.end())
		Select((*specialIt)->Destination()->GetSystem());
	if(selectedSystem)
		center = Point(0., -80.) - selectedSystem->Position();
	
	return true;
}



bool MissionPanel::Click(int x, int y)
{
	dragSide = 0;
	
	// Handle clicks on the interface buttons.
	const Interface *interface = GameData::Interfaces().Get("mission");
	if(interface)
	{
		char key = interface->OnClick(Point(x, y));
		if(key != '\0')
			return KeyDown(static_cast<SDL_Keycode>(key), KMOD_NONE);
	}
	
	if(x < Screen::Left() + SIDE_WIDTH)
	{
		unsigned index = max(0, (y + availableScroll - 36 - Screen::Top()) / 20);
		if(index < available.size())
		{
			availableIt = available.begin();
			while(index--)
				++availableIt;
			acceptedIt = accepted.end();
			specialIt = special.end();
			dragSide = -1;
			Select(availableIt->Destination()->GetSystem());
			center = Point(0., -80.) - selectedSystem->Position();
			return true;
		}
	}
	else if(x >= Screen::Right() - SIDE_WIDTH)
	{
		unsigned index = max(0, (y + acceptedScroll - 36 - Screen::Top()) / 20);
		if(index < special.size() + accepted.size())
		{
			specialIt = special.begin();
			while(specialIt != special.end() && index--)
				++specialIt;
			if(specialIt == special.end())
			{
				acceptedIt = accepted.begin();
				while(acceptedIt != accepted.end() && index--)
					++acceptedIt;
				Select(acceptedIt->Destination()->GetSystem());
			}
			else
			{
				acceptedIt = accepted.end();
				Select((*specialIt)->Destination()->GetSystem());
			}
			availableIt = available.end();
			dragSide = 1;
			
			center = Point(0., -80.) - selectedSystem->Position();
			return true;
		}
	}
	
	// Figure out if a system was clicked on.
	Point click = Point(x, y) - center;
	const System *system = nullptr;
	for(const auto &it : GameData::Systems())
		if(click.Distance(it.second.Position()) < 10.)
		{
			system = &it.second;
			break;
		}
	if(system)
	{
		Select(system);
		int options = available.size() + special.size() + accepted.size();
		while(options--)
		{
			if(availableIt != available.end())
			{
				++availableIt;
				if(availableIt == available.end())
				{
					if(!special.empty())
						specialIt = special.begin();
					else if(!accepted.empty())
						acceptedIt = accepted.begin();
					else
						availableIt = available.begin();
				}
			}
			else if(specialIt != special.end())
			{
				++specialIt;
				if(specialIt == special.end())
				{
					if(!accepted.empty())
						acceptedIt = accepted.begin();
					else if(!available.empty())
						availableIt = available.begin();
					else
						specialIt = special.begin();
				}
			}
			else if(acceptedIt != accepted.end())
			{
				++acceptedIt;
				if(acceptedIt == accepted.end())
				{
					if(!available.empty())
						availableIt = available.begin();
					else if(!special.empty())
						specialIt = special.begin();
					else
						acceptedIt = accepted.begin();
				}
			}
			if(availableIt != available.end() && availableIt->Destination()->GetSystem() == system)
				break;
			if(specialIt != special.end() && (*specialIt)->Destination()->GetSystem() == system)
				break;
			if(acceptedIt != accepted.end() && acceptedIt->Destination()->GetSystem() == system)
				break;
		}
	}
	
	return true;
}



bool MissionPanel::Drag(int dx, int dy)
{
	if(dragSide < 0)
	{
		availableScroll = max(0,
			min(static_cast<int>(available.size() * 20 + 70 - Screen::Height()),
				availableScroll - dy));
	}
	else if(dragSide > 0)
	{
		acceptedScroll = max(0,
			min(static_cast<int>(accepted.size() * 20 + 70 - Screen::Height()),
				acceptedScroll - dy));
	}
	else
		MapPanel::Drag(dx, dy);
	
	return true;
}



void MissionPanel::DrawSelectedSystem() const
{
	const Sprite *sprite = SpriteSet::Get("ui/selected system");
	SpriteShader::Draw(sprite, Point(0., Screen::Top() + .5 * sprite->Height()));
	
	string text;
	if(!selectedSystem)
		text = "Selected system: none";
	else if(!player.HasVisited(selectedSystem))
		text = "Selected system: unexplored system";
	else	
		text = "Selected system: " + selectedSystem->Name();
	
	int jumps = distance.Distance(selectedSystem);
	if(jumps == 1)
		text += " (1 jump away)";
	else if(jumps > 0)
		text += " (" + to_string(jumps) + " jumps away)";
	
	const Font &font = FontSet::Get(14);
	Point pos(-.5 * font.Width(text), Screen::Top() + .5 * (30. - font.Height()));
	font.Draw(text, pos, *GameData::Colors().Get("bright"));
}



Point MissionPanel::DrawPanel(Point pos, const std::string &label, int entries) const
{
	const Font &font = FontSet::Get(14);
	Color back(.125, 1.);
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	
	// Draw the panel.
	Point size(SIDE_WIDTH, 20 * entries + 40);
	FillShader::Fill(pos + .5 * size, size, back);
	
	// Edges:
	const Sprite *bottom = SpriteSet::Get("ui/bottom edge");
	Point edgePos = pos + Point(.5 * size.X(), size.Y());
	Point bottomOff(0., .5 * bottom->Height());
	SpriteShader::Draw(bottom, edgePos + bottomOff);
	
	const Sprite *left = SpriteSet::Get("ui/left edge");
	const Sprite *right = SpriteSet::Get("ui/right edge");
	double dy = .5 * left->Height();
	Point leftOff(-.5 * (size.X() + left->Width()), 0.);
	Point rightOff(.5 * (size.X() + right->Width()), 0.);
	while(dy && edgePos.Y() > Screen::Top())
	{
		edgePos.Y() -= dy;
		SpriteShader::Draw(left, edgePos + leftOff);
		SpriteShader::Draw(right, edgePos + rightOff);
		edgePos.Y() -= dy;
	}
	
	pos += Point(10., 10. + (20. - font.Height()) * .5);
	font.Draw(label, pos, selected);
	FillShader::Fill(
		pos + Point(.5 * size.X() - 5., 15.),
		Point(size.X() - 10., 1.),
		unselected);
	pos.Y() += 5.;
	
	return pos;
}



Point MissionPanel::DrawList(const list<Mission> &list, Point pos) const
{
	const Font &font = FontSet::Get(14);
	Color highlight = *GameData::Colors().Get("faint");
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	Color dim = *GameData::Colors().Get("dim");
	
	for(auto it = list.begin(); it != list.end(); ++it)
	{
		pos.Y() += 20.;
		
		bool isSelected = (it == availableIt || it == acceptedIt);
		if(isSelected)
			FillShader::Fill(
				pos + Point(.5 * SIDE_WIDTH - 5., 8.),
				Point(SIDE_WIDTH - 10., 20.),
				highlight);
		
		bool canAccept = (&list != &available || player.CanAccept(*it));
		font.Draw(it->Name(), pos,
			(!canAccept ? dim : isSelected ? selected : unselected));
	}
	
	return pos;
}




Point MissionPanel::DrawList(const list<const Mission *> &list, Point pos) const
{
	const Font &font = FontSet::Get(14);
	Color highlight = *GameData::Colors().Get("faint");
	Color unselected = *GameData::Colors().Get("medium");
	Color selected = *GameData::Colors().Get("bright");
	
	for(auto it = list.begin(); it != list.end(); ++it)
	{
		pos.Y() += 20.;
		
		bool isSelected = (it == specialIt);
		if(isSelected)
			FillShader::Fill(
				pos + Point(.5 * SIDE_WIDTH - 5., 8.),
				Point(SIDE_WIDTH - 10., 20.),
				highlight);
		
		font.Draw((*it)->Name(), pos, isSelected ? selected : unselected);
	}
	
	return pos;
}



void MissionPanel::DrawMissionInfo() const
{
	Information info;
	
	// The "accept / abort" button text and activation depends on what mission,
	// if any, is selected, and whether missions are available.
	if(CanAccept())
		info.SetCondition("can accept");
	else if(acceptedIt != accepted.end() || specialIt != special.end())
		info.SetCondition("can abort");
	else if(available.size())
		info.SetCondition("cannot accept");
	else
		info.SetCondition("cannot abort");
	
	int cargoFree = player.Cargo().Free();
	info.SetString("cargo free", to_string(cargoFree) + " tons");
	
	int bunksFree = player.Cargo().Bunks();
	info.SetString("bunks free", to_string(bunksFree) + " bunks");
	
	info.SetString("today", player.GetDate().ToString());
	
	const Interface *interface = GameData::Interfaces().Get("mission");
	interface->Draw(info);
	
	// If a mission is selected, draw its descriptive text.
	if(availableIt != available.end())
		wrap.Wrap(availableIt->Description());
	else if(acceptedIt != accepted.end())
		wrap.Wrap(acceptedIt->Description());
	else if(specialIt != special.end())
		wrap.Wrap((*specialIt)->Description());
	else
		return;
	wrap.Draw(Point(-190., Screen::Bottom() - 183.), *GameData::Colors().Get("bright"));
}



bool MissionPanel::CanAccept() const
{
	if(availableIt == available.end())
		return false;
	
	return player.CanAccept(*availableIt);
}
