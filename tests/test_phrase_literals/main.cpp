/* main.cpp - test phrase creation & get
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
#include "Phrase.h"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
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
	
	// Begin loading the game data. Exit early if we are not using the UI.
	if(!GameData::BeginLoad(argv))
		return 0;
	
	auto validPhrases = std::vector<const Phrase *>();
	for(const auto &it : GameData::Phrases())
		validPhrases.emplace_back(&it.second);
	
	auto start = std::chrono::high_resolution_clock::now();
	auto text = std::map<std::string, std::set<std::string>>{};
	for(const auto &p : validPhrases)
	{
		auto &set = text[p->Name()];
		for(int i = 0; i < executions; ++i)
			set.emplace(p->Get());
	}
	auto end = std::chrono::high_resolution_clock::now();
	// Print results.
	auto output = std::ostringstream();
	std::cout << output.str() << '\n' << "Took " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms for "
		<< executions << " Get() from every named Phrase" << std::endl;
	return 0;
}
