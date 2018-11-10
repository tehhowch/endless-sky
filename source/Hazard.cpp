/* Hazard.cpp
Copyright (c) 2018 by tehhowch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Hazard.h"

using namespace std;



// Construct and Load() at the same time.
Hazard::Hazard(const DataNode &node)
{
	Load(node);
}



void Hazard::Load(const DataNode &node)
{
	
}



void Hazard::Step()
{
	
}



bool Hazard::IsActive() const
{
	return true;
}



bool Hazard::Harm(const Body &body) const
{
	return true;
}


// Timer sub-class
//Hazard::Timer::Timer() {}
void Hazard::Timer::Reset() {}
bool Hazard::Timer::Step() 
{
	return true;
}
