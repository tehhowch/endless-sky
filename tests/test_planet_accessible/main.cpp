/* main.cpp - test_planet_accessible
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
#include "PlayerInfo.h"
#include "Planet.h"
#include "Ship.h"

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
std::pair<int, int> Test(std::vector<const Ship *> &shipList, std::vector<const Planet *> &planetList);

int main(int argc, char *argv[])
{
	// Only expected argument is the number of times to perform the desired function.
	unsigned long executions = 10000ul;
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
	
	auto shipList = std::vector<const Ship *>{};
	for(const auto &it : GameData::Ships())
		if(!it.second.ModelName().empty())
			shipList.emplace_back(&it.second);
	
	auto planetList = std::vector<const Planet *>{};
	for(const auto &it : GameData::Planets())
		if(!it.second.TrueName().empty())
			planetList.emplace_back(&it.second);
	
	auto output = std::ostringstream();
	size_t successes = 0;
	size_t failures = 0;
	auto MASK = executions / 10;
	
	auto start = std::chrono::high_resolution_clock::now();
	while(executions-- > 0)
	{
		if(!(executions % MASK))
			std::cout << executions << " remaining" << std::endl;
		auto res = Test(shipList, planetList);
		successes += res.first;
		failures += res.second;
	}
	auto end = std::chrono::high_resolution_clock::now();
	// Print results.
	std::cout << output.str() << '\n' << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms for "
		<< successes << " accessible (" << (successes + failures) << " valid tries)" << '\n'
		<< planetList.size() << " planets, "<< shipList.size() << " ships" << std::endl;
	return 0;
}

std::pair<int, int> Test(std::vector<const Ship *> &shipList, std::vector<const Planet *> &planetList)
{
	int successes = 0;
	int failures = 0;
	
	for(const auto &p : planetList)
		for(const auto &s : shipList)
		{
			if(p->IsAccessible(s))
				++successes;
			else
				++failures;
		}
	
	return { successes, failures };
}
