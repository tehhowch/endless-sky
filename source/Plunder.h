/* Plunder.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef PLUNDER_H
#define PLUNDER_H

#include <string>

class Outfit;
class Ship;


// This class represents one item in the list of outfits you can plunder.
class Plunder {
public:
	// Plunder can be either outfits or commodities.
	Plunder(const std::string &commodity, int count, int unitValue);
	Plunder(const Outfit *outfit, int count);
	
	// Sort by value per ton of mass.
	bool operator<(const Plunder &other) const;
	
	// Check how many of this item are left un-plundered. Once this is zero,
	// the item can be removed from the list.
	int Count() const;
	// Get the value of each unit of this plunder item.
	int64_t UnitValue() const;
	
	// Get the name of this item. If it is a commodity, this is its name.
	const std::string &Name() const;
	// Get the mass, in the format "<count> x <unit mass>". If this is a
	// commodity, no unit mass is given (because it is 1). If the count is
	// 1, only the unit mass is reported.
	const std::string &Size() const;
	// Get the total value (unit value times count) as a string.
	const std::string &Value() const;
	
	// If this is an outfit, get the outfit. Otherwise, this returns null.
	const Outfit *GetOutfit() const;
	// Determine if the ship can take this plunder as-is.
	bool CanTake(const Ship &ship) const;
	// Determine if this plunder can be decomposed into other plunder.
	bool CanSalvage() const;
	// Take some or all of this plunder item.
	void Take(int count);
	
private:
	void UpdateStrings();
	double UnitMass() const;
	
private:
	std::string name;
	const Outfit *outfit;
	int count;
	int64_t unitValue;
	std::string size;
	std::string value;
};



#endif
