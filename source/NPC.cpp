/* NPC.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "NPC.h"

#include "ConversationPanel.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Dialog.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "Messages.h"
#include "PlayerInfo.h"
#include "Random.h"
#include "Ship.h"
#include "ShipEvent.h"
#include "System.h"
#include "UI.h"

using namespace std;



void NPC::Load(const DataNode &node)
{
	// Any tokens after the "npc" tag list the things that must happen for this
	// mission to succeed.
	for(int i = 1; i < node.Size(); ++i)
	{
		if(node.Token(i) == "save")
			failIf |= ShipEvent::DESTROY;
		else if(node.Token(i) == "kill")
			succeedIf |= ShipEvent::DESTROY;
		else if(node.Token(i) == "board")
			succeedIf |= ShipEvent::BOARD;
		else if(node.Token(i) == "assist")
			succeedIf |= ShipEvent::ASSIST;
		else if(node.Token(i) == "disable")
			succeedIf |= ShipEvent::DISABLE;
		else if(node.Token(i) == "scan cargo")
			succeedIf |= ShipEvent::SCAN_CARGO;
		else if(node.Token(i) == "scan outfits")
			succeedIf |= ShipEvent::SCAN_OUTFITS;
		else if(node.Token(i) == "land")
			succeedIf |= ShipEvent::LAND;
		else if(node.Token(i) == "evade")
			mustEvade = true;
		else if(node.Token(i) == "accompany")
			mustAccompany = true;
	}
	
	for(const DataNode &child : node)
	{
		if(child.Token(0) == "system")
		{
			if(child.Size() >= 2)
			{
				if(child.Token(1) == "destination")
					isAtDestination = true;
				else
					system = GameData::Systems().Get(child.Token(1));
			}
			else
				location.Load(child);
		}
		else if(child.Token(0) == "waypoint" || child.Token(0) == "patrol")
		{
			doPatrol |= child.Token(0) == "patrol";
			if(!child.HasChildren())
			{
				// Given "waypoint/patrol" or "waypoint/patrol <system 1> .... <system N>"
				if(child.Size() == 1)
					needsWaypoint = true;
				else if(doPatrol && child.Size() == 2)
					child.PrintTrace("Skipping invalid use of 'patrol': list 0 or 2+ systems to patrol between:");
				else
					for(int i = 1; i < child.Size(); ++i)
						waypoints.push_back(GameData::Systems().Get(child.Token(i)));
			}
			else
			{
				// Given "waypoint/patrol" and child nodes. These get processed during NPC instantiation.
				for(const DataNode &grand : child)
				{
					if(!grand.HasChildren())
						grand.PrintTrace("Skipping invalid patrol waypoint specification:");
					else
					{
						waypointFilters.emplace_back();
						waypointFilters.back().Load(grand);
					}
				}
				if(doPatrol && waypointFilters.size() == 1)
				{
					child.PrintTrace("Skipping invalid use of 'patrol': list 0 or 2+ systems to patrol between:");
					waypointFilters.clear();
				}
			}
		}
		else if(child.Token(0) == "land" || child.Token(0) == "visit")
		{
			doVisit |= child.Token(0) == "visit";
			if(!child.HasChildren())
			{
				// Given "land/visit" or "land/visit <planet 1> ... <planet N>".
				if(child.Size() == 1)
					needsStopover = true;
				else
					for(int i = 1; i < child.Size(); ++i)
						stopovers.push_back(GameData::Planets().Get(child.Token(i)));
			}
			else
			{
				// Given "land/visit" and child nodes. These get processed during NPC instantiation.
				for(const DataNode &grand : child)
				{
					if(!grand.HasChildren())
						grand.PrintTrace("Skipping invalid stopover specification:");
					else
					{
						stopoverFilters.emplace_back();
						stopoverFilters.back().Load(grand);
					}
				}
			}
		}
		else if(child.Token(0) == "succeed" && child.Size() >= 2)
			succeedIf = child.Value(1);
		else if(child.Token(0) == "fail" && child.Size() >= 2)
			failIf = child.Value(1);
		else if(child.Token(0) == "evade")
			mustEvade = true;
		else if(child.Token(0) == "accompany")
			mustAccompany = true;
		else if(child.Token(0) == "government" && child.Size() >= 2)
			government = GameData::Governments().Get(child.Token(1));
		else if(child.Token(0) == "personality")
			personality.Load(child);
		else if(child.Token(0) == "dialog")
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
		else if(child.Token(0) == "ship")
		{
			if(child.HasChildren())
			{
				ships.push_back(make_shared<Ship>());
				ships.back()->Load(child);
				for(const DataNode &grand : child)
					if(grand.Token(0) == "actions" && grand.Size() >= 2)
						actions[ships.back().get()] = grand.Value(1);
			}
			else if(child.Size() >= 2)
			{
				stockShips.push_back(GameData::Ships().Get(child.Token(1)));
				shipNames.push_back(child.Token(1 + (child.Size() > 2)));
			}
		}
		else if(child.Token(0) == "fleet")
		{
			if(child.HasChildren())
			{
				fleets.push_back(Fleet());
				fleets.back().Load(child);
			}
			else if(child.Size() >= 2)
				stockFleets.push_back(GameData::Fleets().Get(child.Token(1)));
		}
	}
	
	// Since a ship's government is not serialized, set it now.
	for(const shared_ptr<Ship> &ship : ships)
	{
		ship->SetGovernment(government);
		ship->SetPersonality(personality);
		ship->SetIsSpecial();
		ship->FinishLoading(false);
		if(!waypoints.empty())
			ship->SetWaypoints(waypoints, doPatrol);
		if(!stopovers.empty())
			ship->SetStopovers(stopovers, doVisit);
	}
}



// Note: the Save() function can assume this is an instantiated mission, not
// a template, so fleets will be replaced by individual ships already.
void NPC::Save(DataWriter &out) const
{
	out.Write("npc");
	out.BeginChild();
	{
		if(succeedIf)
			out.Write("succeed", succeedIf);
		if(failIf)
			out.Write("fail", failIf);
		if(mustEvade)
			out.Write("evade");
		if(mustAccompany)
			out.Write("accompany");
		
		if(government)
			out.Write("government", government->GetName());
		personality.Save(out);
		
		if(!waypoints.empty())
		{
			out.WriteToken(doPatrol ? "patrol" : "waypoint");
			for(const auto &waypoint : waypoints)
				out.WriteToken(waypoint->Name());
			out.Write();
		}
		
		if(!stopovers.empty())
		{
			out.WriteToken(doVisit ? "visit" : "land");
			for(const auto &stopover : stopovers)
				out.WriteToken(stopover->Name());
			out.Write();
		}
		
		if(!dialogText.empty())
		{
			out.Write("dialog");
			out.BeginChild();
			{
				// Break the text up into paragraphs.
				for(const string &line : Format::Split(dialogText, "\n\t"))
					out.Write(line);
			}
			out.EndChild();
		}
		if(!conversation.IsEmpty())
			conversation.Save(out);
		
		for(const shared_ptr<Ship> &ship : ships)
		{
			ship->Save(out);
			auto it = actions.find(ship.get());
			if(it != actions.end() && it->second)
			{
				// Append an "actions" tag to the end of the ship data.
				out.BeginChild();
				{
					out.Write("actions", it->second);
				}
				out.EndChild();
			}
		}
	}
	out.EndChild();
}



// Get the ships associated with this set of NPCs.
const list<shared_ptr<Ship>> NPC::Ships() const
{
	return ships;
}



// Handle the given ShipEvent. (may need updating for ShipEvent::LAND to not re-spawn ships)
void NPC::Do(const ShipEvent &event, PlayerInfo &player, UI *ui, bool isVisible)
{
	// First, check if this ship is part of this NPC. If not, do nothing. If it
	// is an NPC and it just got captured, replace it with a destroyed copy of
	// itself so that this class thinks the ship is destroyed.
	shared_ptr<Ship> ship;
	int type = event.Type();
	for(shared_ptr<Ship> &ptr : ships)
		if(ptr == event.Target())
		{
			// If a mission ship is captured, let it live on under its new
			// ownership but mark our copy of it as destroyed. This must be done
			// before we check the mission's success status because otherwise
			// momentarily reactivating a ship you're supposed to evade would
			// clear the success status and cause the success message to be
			// displayed a second time below. 
			if(event.Type() & ShipEvent::CAPTURE)
			{
				Ship *copy = new Ship(*ptr);
				copy->Destroy();
				actions[copy] = actions[ptr.get()];
				// Count this ship as destroyed, as well as captured.
				type |= ShipEvent::DESTROY;
				ptr.reset(copy);
			}
			ship = ptr;
			break;
		}
	if(!ship)
		return;
	
	// Check if this NPC is already in the succeeded state.
	bool hasSucceeded = HasSucceeded(player.GetSystem());
	bool hasFailed = HasFailed();
	
	// Apply this event to the ship and any ships it is carrying.
	actions[ship.get()] |= type;
	for(const Ship::Bay &bay : ship->Bays())
		if(bay.ship)
			actions[bay.ship.get()] |= type;
	
	// Check if the success status has changed. If so, display a message.
	if(HasFailed() && !hasFailed && isVisible)
		Messages::Add("Mission failed.");
	else if(ui && HasSucceeded(player.GetSystem()) && !hasSucceeded)
	{
		if(!conversation.IsEmpty())
			ui->Push(new ConversationPanel(player, conversation));
		else if(!dialogText.empty())
			ui->Push(new Dialog(dialogText));
	}
	// Permanently landed NPCs should be removed from the class.
	if(event.Type() & ShipEvent::LAND)
	{
		ship.reset();
	}
}



bool NPC::HasSucceeded(const System *playerSystem) const
{
	if(HasFailed())
		return false;
	
	// Check what system each ship is in, if there is a requirement that we
	// either evade them, or accompany them. If you are accompanying a ship, it
	// must not be disabled (so that it can land with you). If trying to evade
	// it, disabling it is sufficient (you do not have to kill it).
	if(mustEvade || mustAccompany)
		for(const shared_ptr<Ship> &ship : ships)
		{
			// Special case: if a ship has been captured, it counts as having
			// been evaded.
			auto it = actions.find(ship.get());
			bool isCapturedOrDisabled = ship->IsDisabled();
			if(it != actions.end())
				isCapturedOrDisabled |= (it->second & ShipEvent::CAPTURE);
			bool isHere = (!ship->GetSystem() || ship->GetSystem() == playerSystem);
			if((isHere && !isCapturedOrDisabled) ^ mustAccompany)
				return false;
		}
	
	if(!succeedIf)
		return true;
	
	for(const shared_ptr<Ship> &ship : ships)
	{
		auto it = actions.find(ship.get());
		if(it == actions.end() || (it->second & succeedIf) != succeedIf)
			return false;
	}
	
	return true;
}



// Check if the NPC is supposed to be accompanied and is not.
bool NPC::IsLeftBehind(const System *playerSystem) const
{
	if(HasFailed())
		return true;
	if(!mustAccompany)
		return false;
	
	for(const shared_ptr<Ship> &ship : ships)
		if(ship->IsDisabled() || ship->GetSystem() != playerSystem)
			return true;
	
	return false;
}



bool NPC::HasFailed() const
{
	for(const auto &it : actions)
	{
		if(it.second & failIf)
			return true;
	
		// If we still need to perform an action that requires the NPC ship be
		// alive, then that ship being destroyed or landed causes the mission to fail.
		if((~it.second & succeedIf) && (it.second & (ShipEvent::DESTROY | ShipEvent::LAND)))
			return true;
			
		// If this ship has landed permanently, the NPC has failed if
		// 1) it must accompany and is not in the destination system, or
		// 2) it must evade, and is in the destination system.
		if((it.second & ShipEvent::LAND) && !doVisit && it.first->GetSystem()
				&& ((mustAccompany && it.first->GetSystem() != destination)
					|| (mustEvade && it.first->GetSystem() == destination)))
			return true;
	}
	
	return false;
}



// Create a copy of this NPC but with the fleets replaced by the actual
// ships they represent, wildcards in the conversation text replaced, etc.
NPC NPC::Instantiate(map<string, string> &subs, const System *origin, const Planet *destinationPlanet) const
{
	NPC result;
	result.destination = destinationPlanet->GetSystem();
	result.government = government;
	if(!result.government)
		result.government = GameData::PlayerGovernment();
	result.personality = personality;
	result.succeedIf = succeedIf;
	result.failIf = failIf;
	result.mustEvade = mustEvade;
	result.mustAccompany = mustAccompany;
	result.waypoints = waypoints;
	result.stopovers = stopovers;
	result.doPatrol = doPatrol;
	result.doVisit = doVisit;
	
	// Pick the system for this NPC to start out in.
	result.system = system;
	if(!result.system && !location.IsEmpty())
	{
		// Find a destination that satisfies the filter.
		vector<const System *> options;
		for(const auto &it : GameData::Systems())
		{
			// Skip entries with incomplete data.
			if(it.second.Name().empty())
				continue;
			if(location.Matches(&it.second, origin))
				options.push_back(&it.second);
		}
		if(!options.empty())
			result.system = options[Random::Int(options.size())];
	}
	if(!result.system)
		result.system = (isAtDestination && destination) ? destination : origin;
	
	if(needsWaypoint && doPatrol)
	{
		// Create a patrol between the mission's origin and destination.
		result.waypoints.push_back(origin);
		result.waypoints.push_back(result.destination);
	}
	else if(needsWaypoint)
		result.waypoints.push_back(result.destination);
	else if(!waypointFilters.empty())
	{
		// NPC waypoint filters are incremental, to provide some sense of direction to the pathing.
		size_t index = result.waypoints.size();
		for(const LocationFilter &filter : waypointFilters)
		{
			// Find a system that satisfies the filter.
			vector<const System *> options;
			for(const auto &it : GameData::Systems())
			{
				// Skip entries with incomplete data, or that are being visited already.
				if(it.second.Name().empty() || std::find(result.waypoints.begin(), result.waypoints.end(),
						&it.second) != result.waypoints.end())
					continue;
				if(filter.Matches(&it.second, index < result.waypoints.size() ? result.waypoints[index] : origin))
					options.push_back(&it.second);
			}
			if(options.size())
				result.waypoints.emplace_back(options[Random::Int(options.size())]);
			else
				// no matching systems.
				continue;
			++index;
		}
	}
	
	if(needsStopover)
		result.stopovers.push_back(destinationPlanet);
	else if(!stopoverFilters.empty())
	{
		// NPC stopover filters are incremental, to provide some sense of direction to the pathing.
		size_t index = result.stopovers.size();
		for(const LocationFilter &filter : stopoverFilters)
		{
			// Find a planet matching the filter.
			vector<const Planet *> options;
			for(const auto &it : GameData::Planets())
			{
				// Skip entries with incomplete data, that the player can't land on, or wormholes.
				if(it.second.Name().empty() || !it.second.CanLand() || it.second.IsWormhole())
					continue;
				if(std::find(result.stopovers.begin(), result.stopovers.end(), &it.second) != result.stopovers.end())
					continue;
				if(filter.Matches(&it.second, index < result.stopovers.size() ? result.stopovers[index]->GetSystem() : origin))
					options.push_back(&it.second);
			}
			if(options.size())
				result.stopovers.emplace_back(options[Random::Int(options.size())]);
			else
				// no matching planets.
				continue;
			++index;
		}
	}
	
	// Convert fleets into instances of ships.
	for(const shared_ptr<Ship> &ship : ships)
		result.ships.push_back(make_shared<Ship>(*ship));
	auto shipIt = stockShips.begin();
	auto nameIt = shipNames.begin();
	for( ; shipIt != stockShips.end() && nameIt != shipNames.end(); ++shipIt, ++nameIt)
	{
		result.ships.push_back(make_shared<Ship>(**shipIt));
		result.ships.back()->SetName(*nameIt);
	}
	for(const Fleet &fleet : fleets)
		fleet.Place(*result.system, result.ships, false);
	for(const Fleet *fleet : stockFleets)
		fleet->Place(*result.system, result.ships, false);
	// Ships should either "enter" the system or start out there.
	for(const shared_ptr<Ship> &ship : result.ships)
	{
		ship->SetGovernment(result.government);
		ship->SetIsSpecial();
		ship->SetPersonality(result.personality);
		result.ships.back()->FinishLoading(true);
		// Use the destinations stored in the NPC copy, in case they were auto-generated.
		if(!result.stopovers.empty())
			ship->SetStopovers(result.stopovers, result.doVisit);
		if(!result.waypoints.empty())
			ship->SetWaypoints(result.waypoints, result.doPatrol);
		
		if(personality.IsEntering())
			Fleet::Enter(*result.system, *ship);
		else
			Fleet::Place(*result.system, *ship);
	}
	
	// String replacement:
	if(!result.ships.empty())
		subs["<npc>"] = result.ships.front()->Name();
	
	// Do string replacement on any dialog or conversation.
	if(!dialogText.empty())
		result.dialogText = Format::Replace(dialogText, subs);
	
	if(stockConversation)
		result.conversation = stockConversation->Substitute(subs);
	else if(!conversation.IsEmpty())
		result.conversation = conversation.Substitute(subs);
	
	return result;
}
