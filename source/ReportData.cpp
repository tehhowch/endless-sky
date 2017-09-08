/* ReportData.cpp
Copyright (c) 2017 by tehhowch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ReportData.h"

#include "File.h"
#include "Files.h"
#include "Government.h"
#include "Mission.h"
#include "NPC.h"
#include "Ship.h"
#include "System.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace {
	string ShipDataHeader(string name)
	{
		ostringstream out;
		out << "Ship: " << name << '\n'
			<< "System" << '\t'
			<< "X" << '\t' << "Y" << '\t' << "Vx" << '\t' << "Vy" << '\t'
			<< "Speed" << '\t' << "Facing" << '\t' << "%Hull" << '\t'
			<< "%Shield" << '\t' << "%Energy" << '\t' << "%Heat" << '\t'
			<< "%Fuel" << '\t' << "Hull" << '\t' << "Shields" << '\t'
			<< "Energy" << '\t' << "Temp." << '\t' << "Fuel" << '\t'
			<< "Target" << '\t' << "Ioniz." << '\t' << "Disrupt." << '\t'
			<< "Slowing"
			;
		out << '\n';
		return out.str();
	}
	
	string GetShipDataString(const shared_ptr<Ship> ship)
	{
		ostringstream out;
		if(ship->IsDestroyed())
		{
			out << "destroyed" << '\t'
				<< "" << '\t'
				<< "" << '\t'
				<< "" << '\t'
				<< "" << '\t'
				<< "" << '\t'
				<< "" << '\t';
		}
		else if(ship->GetSystem())
		{
			out << ship->GetSystem()->Name() << '\t'
				<< ship->Position().X() << '\t'
				<< ship->Position().Y() << '\t'
				<< ship->Velocity().X() << '\t'
				<< ship->Velocity().Y() << '\t'
				<< ship->Velocity().Length() << '\t'
				<< ship->Facing().Degrees() << '\t';
		}
		else
		{
			const shared_ptr<Ship> &parent = ship->GetParent();
			out << "Carried: " + parent->Name() << '\t'
				<< parent->Position().X() << '\t'
				<< parent->Position().Y() << '\t'
				<< parent->Velocity().X() << '\t'
				<< parent->Velocity().Y() << '\t'
				<< parent->Velocity().Length() << '\t'
				<< parent->Facing().Degrees() << '\t';
		}
		out << ship->Hull() * 100 << '\t'
			<< ship->Shields() * 100 << '\t'
			<< ship->Energy() * 100 << '\t'
			<< ship->Heat() * 100 << '\t'
			<< ship->Fuel() * 100 << '\t'
			<< ship->Attributes().Get("hull") * ship->Hull() << '\t'
			<< ship->Attributes().Get("shields") * ship->Shields() << '\t'
			<< ship->Attributes().Get("energy capacity") *ship->Energy() << '\t'
			<< ship->Mass() * 100 * ship->Heat() << '\t'
			<< ship->Attributes().Get("fuel capacity") * ship->Fuel() << '\t'
			<< (ship->GetTargetShip() ? (!ship->GetTargetShip()->Name().empty() ?
				ship->GetTargetShip()->Name() : ship->GetTargetShip()->GetGovernment()->GetName() + " ship")
				: "No target") << '\t'
			;
		return out.str();
	}
	
	// Ensure the ship has a valid logging filename. It will use the same
	// filename through every reset, since loggedShips is only cleared by
	// loading a new save (i.e. a new ReportData instance).
	string GetLoggableShipName(const shared_ptr<Ship> ship)
	{
		// Don't allow characters that can't be used in a file name.
		static const string FORBIDDEN = "/\\?*:|\"<>~";
		
		string shipLogName = ship->Name();
		if(shipLogName.empty())
			shipLogName = ship->GetGovernment()->GetName() + to_string(reinterpret_cast<uint64_t>(&*ship));
		
		// Strip out any unacceptable characters.
		string name;
		for(const auto c : shipLogName)
			if(FORBIDDEN.find(c) == string::npos)
				name += c;
		
		return name;
	}
	
	void Write(string &path, string &data)
	{
		// ES does not have 'append' as an opening method.
		if(Files::Exists(path))
			data = Files::Read(path) + '\n' + data;
		Files::Write(path, data);
	}
}



// Constructor. Each time a savegame is loaded, a new logger is created.
ReportData::ReportData(const PlayerInfo &player)
	: step(0), player(player)
{
	CleanData();
	// Prepare the information needed for writing to file.
	directoryPath = Files::Config() + "battlelogs/";
	if(!Files::Exists(directoryPath))
	{
		canWrite = false;
		Files::LogError("No directory for the battle logger files.");
	}
	// Prepare this logger's output prefix (using logic from PlayerInfo::SetName).
	// Each battle log is the prefix 'bl~', the specific logfile name, an integer
	// (if there are pre-existing battle logs), and then the player's savegame name.
	const string playerID = player.Identifier();
	string testLogName = "shipDamage";
	string prefix = "bl~" + testLogName + "~";
	string suffix = playerID + ".txt";
	int fileCount = 0;
	while(true)
	{
		string testPrefix = prefix;
		if(fileCount++)
			testPrefix += to_string(fileCount) + "~";
		if(!Files::Exists(directoryPath + testPrefix + suffix))
			break;
	}
	// Decrementing fileCount indicates the number of pre-existing battle
	// logs, and thus the needed suffix for non-overwriting logfiles.
	if(--fileCount)
		// This is not this player's first use of logging.
		suffix = to_string(++fileCount) + "~" + suffix;
	logSuffix = "~" + suffix;
	
	/* Written files:
		bl~shipDamage:
			The list of ships, the total damage they each took, the
			number of times targeted, and the number of shots fired.
		bl~governmentHits:
			The list of governments, the number of times they got
			hit, and the number of hits they gave out.
		bl~timeData~<shipName>:
			A timestepped tracker of a lot of data. Each step is a
			new row. Each ship in 'loggedShips' gets its own file.
			If the ship doesn't have a name, its government and its
			pointer is used instead.
	*/
}



// Update the ships being logged in TimeStepLogs.
void ReportData::Reset()
{
	// All mission NPCs are logged.
	for(const Mission &mission : player.Missions())
		for(const NPC &npc : mission.NPCs())
			for(const shared_ptr<Ship> &ship : npc.Ships())
				loggedShips.emplace(ship, GetLoggableShipName(ship));
	
	// All unparked player ships are logged.
	for(const shared_ptr<Ship> &ship : player.Ships())
		if(!ship->IsParked())
			loggedShips.emplace(ship, GetLoggableShipName(ship));
}


void ReportData::WriteData()
{
	// We use various file names to write certain data only to certain places,
	// for improved organization.
	if(canWrite)
	{
		// Write the summary data: government hits
		string govtOutput;
		map<const Government *, string> govtTable;
		if(!didHit.empty())
		{
			govtOutput += "Timestep:\t" + to_string(step) + "\nSource Gov't\tHits Given\tHits Taken\n";
			for(const auto &govt : didHit)
				govtTable.emplace(govt.first, govt.first->GetName() + '\t' + to_string(govt.second));
		}
		if(!gotHit.empty())
		{
			if(govtOutput.empty())
				govtOutput += "Timestep:\t" + to_string(step) + "\nSource Gov't\tHits Given\tHits Taken\n";
			// Some governments may have gotten hit, but not attacked anyone.
			for(const auto &govt : gotHit)
				govtTable.emplace(govt.first, govt.first->GetName() + '\t' + to_string(0));
			// For all hit governments, add the number of times they were hit.
			for(const auto &govt : gotHit)
				govtTable[govt.first] += '\t' + to_string(govt.second) + '\n';
			// Some governments may have attacked others, but not been hit.
			for(auto &govt : govtTable)
			{
				if(govt.second.back() != '\n')
					govt.second += "\t0\n";
				govtOutput += govt.second;
			}
			govtOutput += '\n';
		}
		if(!hitGotHit.empty())
		{
			govtOutput += "Source Gov't\tTarget Gov't\tSuccessful Hits\n";
			for(const auto &source : hitGotHit)
				for(const auto &target : source.second)
					govtOutput += source.first->GetName() + '\t' + target.first->GetName() + '\t' + to_string(target.second) + '\n';
		}
		if(!govtOutput.empty())
		{
			string fileName = directoryPath + "bl~governmentHits" + logSuffix;
			Write(fileName, govtOutput);
		}
		
		// Write the summary data: ship hits/damagetaken
		string shipSummary;
		map<const Ship *, string> shipTable;
		if(!shotsFired.empty())
		{
			shipSummary += "Timestep:\t" + to_string(step) + "\t\t\t\t\tReceived:"
				+ "\nShip\tModel\tShots Fired\tShots Fired At\t\tShield Dmg\tHull Dmg\tHeat Dmg\tIon Dmg\tDisruption Dmg\tSlowing Dmg\n";
			for(const auto &ship : shotsFired)
				shipTable.emplace(ship.first,
						(ship.first->Name().empty() ? "Unnamed " + ship.first->GetGovernment()->GetName() : ship.first->Name())
						+ '\t' + ship.first->ModelName() + '\t' + to_string(ship.second));
		}
		if(!firedAt.empty())
		{
			if(shipSummary.empty())
				shipSummary += "Timestep:\t" + to_string(step) + "\t\t\t\t\t"
					+ "\nShip\tModel\tShots Fired\tShots Fired At\t\tShield Dmg\tHull Dmg\tHeat Dmg\tIon Dmg\tDisruption Dmg\tSlowing Dmg\n";
			// Some ships may have not fired any shots, but been targeted anyway.
			for(const auto &ship : firedAt)
				shipTable.emplace(ship.first.get(),
						(ship.first->Name().empty() ? "Unnamed " + ship.first->GetGovernment()->GetName() : ship.first->Name())
						+ '\t' + ship.first->ModelName() + "\t0");
			// For all targeted ships, indicate how many times they were targeted.
			for(const auto &ship : firedAt)
				shipTable[ship.first.get()] += '\t' + to_string(ship.second) + '\t';
			// Some ships may have attacked others, but not been targeted.
			for(auto &ship : shipTable)
				if(ship.second.back() != '\t')
					ship.second += "\t0\t";
		}
		if(!damageReceived.empty())
		{
			if(shipSummary.empty())
				shipSummary += "Timestep:\t" + to_string(step) + "\t\t\t\t\t"
					+ "\nShip\tModel\tShots Fired\tShots Fired At\t\tShield Dmg\tHull Dmg\tHeat Dmg\tIon Dmg\tDisruption Dmg\tSlowing Dmg\n";
			// Some ships may have been damaged, but not targeted nor attacked others.
			for(const auto &ship : damageReceived)
				shipTable.emplace(ship.first,
						(ship.first->Name().empty() ? "Unnamed " + ship.first->GetGovernment()->GetName() : ship.first->Name())
						+ '\t' + ship.first->ModelName() + "\t0\t0\t");
			// For all ships which took damage, record the total damage taken.
			for(const auto &ship : damageReceived)
			{
				for(const auto &damage : ship.second)
					shipTable[ship.first] += '\t' + to_string(lround(damage.second));
				shipTable[ship.first] += '\n';
			}
			// Some ships may not have been targeted or taken damage, but fired shots.
			for(auto &ship : shipTable)
			{
				if(ship.second.back() != '\n')
					ship.second += "\t0\t0\t0\t0\t0\t0\n";
				shipSummary += ship.second;
			}
		}
		if(!shipSummary.empty())
		{
			string fileName = directoryPath + "bl~shipDamage" + logSuffix;
			Write(fileName, govtOutput);
		}
		
		// Write the many ships' time-dependent data.
		for(const auto &sit : loggedShips)
		{
			const shared_ptr<Ship> &ship = sit.first;
			// Assemble data into a string for the filewrite.
			string output;
			for(const auto &ts : timeData)
			{
				// Log 6 times each second rather than 60.
				if(ts.first % 10)
					continue;
				output += to_string(ts.first) + '\t';
				const auto shipIt = ts.second.Ships().find(ship);
				if(shipIt != ts.second.Ships().end())
					output += shipIt->second;
				const auto ionIt = ts.second.ShipIonization().find(ship);
				if(ionIt != ts.second.ShipIonization().end())
					output += '\t' + to_string(ionIt->second);
				const auto disIt = ts.second.ShipDisruption().find(ship);
				if(disIt != ts.second.ShipDisruption().end())
					output += '\t' + to_string(disIt->second);
				const auto slowIt = ts.second.ShipSlowness().find(ship);
				if(slowIt != ts.second.ShipSlowness().end())
					output += '\t' + to_string(slowIt->second);
				output += '\n';
			}
			//	if file is empty, start with the header. Otherwise, just add data.
			string fileName = directoryPath + "bl~timeData~" + sit.second + logSuffix;
			if(!Files::Exists(fileName))
				output = ShipDataHeader(ship->Name()) + output;
			Write(fileName, output);
		}
	}
	// Now that all the data has been written, wipe it from memory.
	CleanData();
}



// Called after Engine::Step, by MainPanel.
void ReportData::Step(bool isActive)
{
	if(isActive)
	{
		++step;
		// Since the map sorts by the step, always hint the end.
		// Create a new timestep log at the end of the time-dependent data.
		timeData.emplace_hint(timeData.cend(), step, TimeStepLog(step, loggedShips));
	}
}



// Records hits from and by all governments, of all ships - does not discriminate
// based on special / non-special ships.
void ReportData::RecordHit(const Government *source, const Government *target)
{
	// Explosion weapons do not have governments attached.
	if(source)
	{
		++didHit[source];
		++hitGotHit[source][target];
	}
	++gotHit[target];
}



// Only called for special ships.
void ReportData::RecordFire(const Ship *actor, const shared_ptr<Ship> targeted, int timesFired)
{
	shotsFired[actor] += timesFired;
	if(targeted)
		++firedAt[targeted];
}



// Only called for special ships.
void ReportData::RecordDamage(const Ship *hit, const vector<double> damageValues, double ion, double disrupt, double slow)
{
	// ints declared in class Weapon:
	// static const int SHIELD_DAMAGE = 0;
	// static const int HULL_DAMAGE = 1;
	// static const int HEAT_DAMAGE = 2;
	// static const int ION_DAMAGE = 3;
	// static const int DISRUPTION_DAMAGE = 4;
	// static const int SLOWING_DAMAGE = 5;
	for(size_t i = 0; i < damageValues.size(); ++i)
		damageReceived[hit][i] += damageValues[i];
	
	// Will throw out-of-range if step doesn't exist yet.
	TimeStepLog &ts = timeData.at(step);
	// Get the mapping ptr to the ship.
	shared_ptr<Ship> hit_ptr = (const_cast<Ship *>(hit))->shared_from_this();
	// Log the ionization, disruption, and slowness of the hit ship. A ship
	// may be hit multiple times in a given frame, so only the most recent
	// value should be kept.
	ts.ShipIonization()[hit_ptr] = ion;
	ts.ShipDisruption()[hit_ptr] = disrupt;
	ts.ShipSlowness()[hit_ptr] = slow;
}



// Empty all the logging containers, but do not alter the step.
void ReportData::CleanData()
{
	didHit.clear();
	gotHit.clear();
	hitGotHit.clear();
	shotsFired.clear();
	damageReceived.clear();
	firedAt.clear();
	timeData.clear();
}



// TimeStepLogs report the data of the ships at each given point in time.
// This level of data could be used to draw heatmaps or other graphical
// representations of the battle.
ReportData::TimeStepLog::TimeStepLog(int64_t step, map<shared_ptr<Ship>, const string> shipList)
	: timestep(step)
{
	for(const auto &sit : shipList)
		shipData[sit.first] = GetShipDataString(sit.first);
}



map<const shared_ptr<Ship>, string> &ReportData::TimeStepLog::Ships()
{
	return shipData;
}



const map<const shared_ptr<Ship>, string> &ReportData::TimeStepLog::Ships() const
{
	return shipData;
}



map<const shared_ptr<Ship>, double> &ReportData::TimeStepLog::ShipIonization()
{
	return shipIon;
}



const map<const shared_ptr<Ship>, double> &ReportData::TimeStepLog::ShipIonization() const
{
	return shipIon;
}



map<const shared_ptr<Ship>, double> &ReportData::TimeStepLog::ShipDisruption()
{
	return shipDisrupt;
}



const map<const shared_ptr<Ship>, double> &ReportData::TimeStepLog::ShipDisruption() const
{
	return shipDisrupt;
}



map<const shared_ptr<Ship>, double> &ReportData::TimeStepLog::ShipSlowness()
{
	return shipSlow;
}



const map<const shared_ptr<Ship>, double> &ReportData::TimeStepLog::ShipSlowness() const
{
	return shipSlow;
}
