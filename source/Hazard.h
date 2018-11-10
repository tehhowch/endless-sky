/* Hardpoint.h
Copyright (c) 2018 by tehhowch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef HAZARD_H_
#define HAZARD_H_

//#include "Position.h"

#include <string>


class Body;
class DataNode;
class Effect;
class System; // maybe not needed
class Weapon;

// Class that represents a widespread danger within a System, such as a solar storm.
// Hazards may be ever-present, or cycle between active and dormant states.
class Hazard {
public:
	Hazard() = default;
	// Construct and Load() at the same time.
	Hazard(const DataNode &node);
	
	void Load(const DataNode &node);
	// Called each time the Engine steps
	void Step();
	
	// If this hazard is currently posing a threat to in-system ships.
	bool IsActive() const;
	// Attempt to harm the given object. Returns true if it was affected. (could be void?)
	bool Harm(const Body &body) const; // minable, ship, ???


private:
	// Class that manages the activity of the associated Hazard.
	class Timer {
	public:
		Timer() = default;
		
		// Called each time the Engine advances while in-flight.
		bool Step();
		void Reset(); // Not sure.
		
	private:
		int delta = 10;
		int duration = 300;
		
	};


private:

	std::string name;
	bool isActive;
	Timer timer;
	// Position pos; // Do we want localized hazards?
	
	// A visual effect is used to convey the presence of this Hazard in a system.
	Effect *activeAppearance = nullptr;
	Effect *inactiveAppearance = nullptr;
	
	// The effect applied to objects taking damage (in lieu of projectile effects).
	Effect *damageEffect = nullptr;
	Weapon *weapon = nullptr;


};



#endif
