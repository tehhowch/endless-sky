/* main.cpp - test_fleet_enter
Copyright (c) 2019 by tehhowch

Test function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Fleet.h"
#include "GameData.h"
#include "PlayerInfo.h"
#include "Planet.h"
#include "StellarObject.h"
#include "System.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class Ship;

int main(int argc, char *argv[])
{
	// Only expected argument is the number of times to perform the desired function.
	unsigned long executions = 6ul;
	for(const char *const *it = argv + 1; *it; ++it)
	{
		std::string arg = *it;
		if(arg == "-n")
		{
			if(++it && *it)
				executions = std::stoul(*it);
		}
	}
	std::cout << "Commencing test with n=" << executions << std::endl;
	
	PlayerInfo player;
	
	// Begin loading the game data. Exit early if we are not using the UI.
	if(!GameData::BeginLoad(argv))
		return 0;
	
	// Load player data, including reference-checking.
	player.LoadRecent();
	
	auto validSystems = std::vector<std::pair<const System *, std::set<const Planet *>>>{};
	for(const auto &it : GameData::Systems())
		if(!it.second.Name().empty())
		{
			// Collect the planets in this system.
			auto planets = std::set<const Planet *>{ nullptr };
			for(const auto &obj : it.second.Objects())
				if(obj.GetPlanet() && !obj.GetPlanet()->TrueName().empty() && !obj.GetPlanet()->IsWormhole())
					planets.emplace(obj.GetPlanet());
			
			validSystems.emplace_back(&it.second, planets);
		}
	
	auto validFleets = std::vector<const Fleet *>{};
	for(const auto &it : GameData::Fleets())
		if(it.second.GetGovernment())
			validFleets.emplace_back(&it.second);
	
	auto output = std::ostringstream();
	size_t successes = 0;
	size_t failures = 0;
	auto start = std::chrono::high_resolution_clock::now();
	while(executions-- > 0)
	{
		if(!(executions & 2))
			std::cout << executions << " remaining" << std::endl;
		
		auto shipList = std::list<std::shared_ptr<Ship>>{};
		for(const auto &it : validSystems)
		{
			const auto &s = it.first;
			// Spawn each fleet multiple times for each planet reference.
			for(const auto &p : it.second)
				for(const auto &fleet : validFleets)
					for(int i = 0; i < 50; ++i)
					{
						// Ships are added to the front of the list, so success means the start iterator changed.
						auto start = shipList.cbegin();
						fleet->Enter(*s, shipList, p);
						if(start != shipList.cbegin())
							++successes;
						else
							++failures;
					}
			
			shipList.clear(); // Avoid excessive list size.
		}
	}
	auto end = std::chrono::high_resolution_clock::now();
	// Print results.
	std::cout << output.str() << '\n' << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms for "
		<< successes << " successful spawns (" << (successes + failures) << " valid tries)" << std::endl;
	return 0;
}
