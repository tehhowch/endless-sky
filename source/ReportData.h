/* ReportData.h
Copyright (c) 2017 by tehhowch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef REPORTING_H_
#define REPORTING_H_

#include "PlayerInfo.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Government;
class Ship;

// Class used to report data from a competition battle between mission NPCs.
// Only reports data for mission NPCs, and outputs data whenever mission NPCs
// can be deleted (to avoid holding stale pointers).
class ReportData {
public:
	explicit ReportData(const PlayerInfo &player);
	// ~ReportData();
	
	// Set the output directory for the given player. Numerous files are made.
	void Init(const PlayerInfo &player);
	// Update the logger with the ships to be logged.
	void Reset();//const std::list<std::shared_ptr<Ship>> ships);
	// Append the recorded data to the output files (and reset the internal loggers).
	// This should be called every time the Engine::Ships list may be changed (i.e.
	// on TakeOff and after boarding/assisting missions insert ships).
	void WriteData();
	
	// Update the timestep being logged.
	void Step(const bool isActive = false);
	
	// Called from Engine::Step's collision detection section.
	void RecordHit(const Government *source, const Government *target);
	// Called from Ship::Fire.
	void RecordFire(const Ship *actor, const std::shared_ptr<Ship> targeted, int timesFired);
	// Called from Ship::TakeDamage.
	void RecordDamage(const Ship *hit, const Government *source, const std::vector<double> damageValues, double ion, double disrupt, double slow);

private:
	// Reset the internal loggers.
	void CleanData();

private:
	class TimeStepLog {
	public:
		TimeStepLog(int64_t step, std::map<std::shared_ptr<Ship>, const std::string> shipList);
		std::map<const std::shared_ptr<Ship>, std::string> &Ships();
		const std::map<const std::shared_ptr<Ship>, std::string> &Ships() const;
		std::map<const std::shared_ptr<Ship>, double> &ShipIonization();
		const std::map<const std::shared_ptr<Ship>, double> &ShipIonization() const;
		std::map<const std::shared_ptr<Ship>, double> &ShipDisruption();
		const std::map<const std::shared_ptr<Ship>, double> &ShipDisruption() const;
		std::map<const std::shared_ptr<Ship>, double> &ShipSlowness();
		const std::map<const std::shared_ptr<Ship>, double> &ShipSlowness() const;
	private:
		int64_t timestep;
		// Ship log: name, positional data
		std::map<const std::shared_ptr<Ship>, std::string> shipData;
		std::map<const std::shared_ptr<Ship>, double> shipIon;
		std::map<const std::shared_ptr<Ship>, double> shipDisrupt;
		std::map<const std::shared_ptr<Ship>, double> shipSlow;
	};

private:
	bool canWrite = true;
	std::string directoryPath;
	std::string logSuffix;
	
	int64_t step = 0;
	const PlayerInfo &player;
	// Time-dependent loggers
	// Ship Pointer : filename.
	std::map<std::shared_ptr<Ship>, const std::string> loggedShips;
	// Step # : step#-map<ships, ships' data>
	std::map<int64_t, TimeStepLog> timeData;
	
	// "Totals" loggers
	std::map<const Government *, int> didHit;
	std::map<const Government *, int> gotHit;
	std::map<const Government *, std::map<const Government *, int>> hitGotHit;
	std::map<const Ship *, std::map<int, double>> damageReceived;
	std::map<const Ship *, std::map<const Government *, std::map<int, std::pair<double, int>>>> dmgReceivedByGovt;
	// Holds a ptr to even 'common' ships so that they can be ID'd at writing time.
	// After writing, the pointer will be released.
	std::map<std::shared_ptr<Ship>, int> firedAt;
	std::map<const Ship *, int> shotsFired;
};


#endif
