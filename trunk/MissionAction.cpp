/* MissionAction.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MissionAction.h"

#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "UI.h"

using namespace std;



void MissionAction::Load(const DataNode &node)
{
	if(node.Size() >= 2)
		trigger = node.Token(1);
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "dialog")
		{
			for(int i = 1; i < child.Size(); ++i)
			{
				if(!dialogText.empty())
					dialogText += "\n\t";
				dialogText += child.Token(i);
			}
			for(const DataNode &grand : child)
				for(int i = 0; i < grand.Size(); ++i)
				{
					if(!dialogText.empty())
						dialogText += "\n\t";
					dialogText += grand.Token(i);
				}
		}
		else if(child.Token(0) == "conversation" && child.HasChildren())
			conversation.Load(child);
		else if(child.Token(0) == "conversation" && child.Size() > 1)
			stockConversation = GameData::Conversations().Get(child.Token(1));
		else if(child.Token(0) == "outfit" && child.Size() >= 2)
		{
			int count = (child.Size() < 3 ? 1 : static_cast<int>(child.Value(2)));
			gifts[GameData::Outfits().Get(child.Token(1))] = count;
		}
		else if(child.Token(0) == "payment" && child.Size() >= 2)
			payment += child.Value(1);
		else if(child.Token(0) == "payment")
			giveDefaultPayment = true;
		else
			conditions.Add(child);
	}
}



// Note: the Save() function can assume this is an instantiated mission, not
// a template, so it only has to save a subset of the data.
void MissionAction::Save(DataWriter &out) const
{
	out.Write("on", trigger);
	out.BeginChild();
	
	if(!dialogText.empty())
	{
		out.Write("dialog");
		out.BeginChild();
		
		// Break the text up into paragraphs.
		size_t begin = 0;
		while(true)
		{
			size_t pos = dialogText.find("\n\t", begin);
			if(pos == string::npos)
				pos = dialogText.length();
			out.Write(dialogText.substr(begin, pos - begin));
			if(pos == dialogText.length())
				break;
			begin = pos + 2;
		}
		out.EndChild();
	}
	if(!conversation.IsEmpty())
		conversation.Save(out);
	
	for(const auto &it : gifts)
		out.Write("outfit", it.first->Name(), it.second);
	if(payment)
		out.Write("payment", payment);
	
	conditions.Save(out);
	
	out.EndChild();
}



int MissionAction::Payment() const
{
	return payment;
}



// Check if this action can be completed right now. It cannot be completed
// if it takes away money or outfits that the player does not have.
bool MissionAction::CanBeDone(const PlayerInfo &player) const
{
	if(player.Accounts().Credits() < -payment)
		return false;
	
	const Ship *flagship = player.GetShip();
	for(const auto &it : gifts)
	{
		if(it.second >= 0)
			continue;
		
		// The outfit can be taken from the player's cargo or from the flagship.
		// TODO: decide if taking it from an escort is acceptable too.
		int available = player.Cargo().Get(it.first);
		if(flagship)
			available += flagship->OutfitCount(it.first);
		
		if(available < -it.second)
			return false;
	}
	return true;
}



void MissionAction::Do(PlayerInfo &player, UI *ui, const System *destination) const
{
	bool isOffer = (trigger == "offer");
	if(!conversation.IsEmpty())
	{
		ConversationPanel *panel = new ConversationPanel(player, conversation, destination);
		if(isOffer)
			panel->SetCallback(&player, &PlayerInfo::MissionCallback);
		ui->Push(panel);
	}
	else if(!dialogText.empty())
	{
		if(isOffer)
			ui->Push(new Dialog(dialogText, player));
		else
			ui->Push(new Dialog(dialogText));
	}
	else if(isOffer && ui)
		player.MissionCallback(Conversation::ACCEPT);
	
	Ship *flagship = player.GetShip();
	for(const auto &it : gifts)
	{
		int count = it.second;
		string message = to_string(abs(count)) + " " + it.first->Name();
		if(count == 1)
			message + " was added to your ";
		else if(count == -1)
			message += " was removed from your ";
		else if(count > 0)
			message += " were added to your ";
		else
			message += " were removed from your ";
		
		bool didCargo = false;
		bool didShip = false;
		int cargoCount = player.Cargo().Get(it.first);
		if(count < 0 && cargoCount)
		{
			int moved = min(cargoCount, -count);
			count += moved;
			player.Cargo().Transfer(it.first, moved);
			didCargo = true;
		}
		while(flagship && count)
		{
			int moved = (count > 0) ? 1 : -1;
			if(flagship->Attributes().CanAdd(*it.first, moved))
			{
				flagship->AddOutfit(it.first, moved);
				didShip = true;
			}
			else
				break;
			count -= moved;
		}
		if(count > 0)
		{
			player.Cargo().Transfer(it.first, -count);
			didCargo = true;
		}
		if(didCargo && didShip)
			message += "cargo hold and your flagship.";
		else if(didCargo)
			message += "cargo hold.";
		else
			message += "flagship.";
		Messages::Add(message);
	}
	
	if(payment)
		player.Accounts().AddCredits(payment);
	
	conditions.Apply(player.Conditions());
}



MissionAction MissionAction::Instantiate(map<string, string> &subs, int defaultPayment) const
{
	MissionAction result;
	result.trigger = trigger;
	
	result.gifts = gifts;
	result.payment = payment + (giveDefaultPayment ? defaultPayment : 0);
	// Fill in the payment amount if this is the "complete" action (which comes
	// before all the others in the list).
	if(trigger == "complete")
		subs["<payment>"] = Format::Number(result.payment)
			+ (result.payment == 1 ? " credit" : " credits");
	
	if(!dialogText.empty())
		result.dialogText = Format::Replace(dialogText, subs);
	
	if(stockConversation)
		result.conversation = stockConversation->Substitute(subs);
	else if(!conversation.IsEmpty())
		result.conversation = conversation.Substitute(subs);
	
	result.conditions = conditions;
	
	return result;
}
