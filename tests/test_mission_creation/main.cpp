/* main.cpp - test_mission_creation
Copyright (c) 2019 by tehhowch

Test function for Endless Sky, a space exploration and combat RPG.

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "GameData.h"
#include "Mission.h"
#include "PlayerInfo.h"
#include "Planet.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
	// Only expected argument is the number of times to perform the desired function.
	long executions = 100;
	for(const char *const *it = argv + 1; *it; ++it)
	{
		std::string arg = *it;
		if(arg == "-n")
		{
			if(++it && *it)
				executions = std::atol(*it);
		}
	}
	std::cout << "Commencing test with n=" << executions << std::endl;
	
	PlayerInfo player;
	
	// Begin loading the game data. Exit early if we are not using the UI.
	if(!GameData::BeginLoad(argv))
		return 0;
	
	// Load player data, including reference-checking.
	player.LoadRecent();
	
	auto validPlanets = std::vector<const Planet *>();
	for(const auto &it : GameData::Planets())
	{
		const Planet &p = it.second;
		if(p.IsWormhole())
			continue;
		validPlanets.emplace_back(&p);
	}
	//auto matchedPlanets = std::map<const Mission *, int>();
	auto start = std::chrono::high_resolution_clock::now();
	size_t successes = 0;
	size_t failures = 0;
	while(executions-- > 0)
	{
		for(const auto &p : validPlanets)
		{
			player.SetPlanet(p);
			// Try to offer every mission.
			for(const auto &it : GameData::Missions())
			{
				const Mission &m = it.second;
				if(m.CanOffer(player))
				{
					// ++matchedPlanets[&m];
					++successes;
				}
				else if (!m.IsAtLocation(Mission::BOARDING) && !m.IsAtLocation(Mission::ASSISTING))
					++failures;
			}
		}
	}
	auto end = std::chrono::high_resolution_clock::now();
	// Print results.
	auto output = std::ostringstream();
	// auto results = std::vector<std::pair<std::string, int>>();
	// for(const auto &it : matchedPlanets)
	// 	results.emplace_back(it.first->Name(), it.second);
	// std::sort(results.begin(), results.end(), [](const std::pair<std::string, int> &r1, const std::pair<std::string, int> &r2) { return r1.second > r2.second;});
	
	// output << '\n' << "Mission Name                " << '\t' << "Successful Offers" << '\n';
	// for(const auto &it : results)
	// {
	// 	std::string text = it.first.substr(0, std::min(it.first.size(), static_cast<size_t>(28)));
	// 	while(text.size() < 28)
	// 		text.append(" ");
	// 	output << text << '\t' << it.second << '\n';
	// }
	std::cout << output.str() << '\n' << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms for "
		<< successes << " successful offers (" << (successes + failures) << " valid tries)" << std::endl;
	return 0;
}
