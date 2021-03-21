/* Uuid.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Uuid.h"

#include "Files.h"
#include "Random.h"

#include <algorithm>



Uuid Uuid::FromString(const std::string &input)
{
	return Uuid(input);
}



bool Uuid::operator==(const Uuid &other) const
{
	return Value() == other.Value();
}



bool Uuid::operator!=(const Uuid &other) const
{
	return Value() != other.Value();
}



const std::string &Uuid::ToString() const
{
	return Value();
}



// Internal constructor. Note that the provided value may not be a valid v4 UUID, in which case an error is logged
// and we return a new UUID.
Uuid::Uuid(const std::string &input)
{
	// The input must have the correct number of characters and contain the correct subset
	// of characters. This validation isn't exact, nor do we really require it to be, since
	// this is not a networked application.
	bool isValid = input.size() == 36
			&& std::count(input.begin(), input.end(), '-') == 4
			&& std::all_of(input.begin(), input.end(), [](const char &c) noexcept -> bool
			{
				return c == '-' || std::isxdigit(static_cast<unsigned char>(c));
			});
	
	if(isValid)
		value = input;
	else
		Files::LogError("Warning: Replacing invalid v4 UUID string \"" + input + "\"");
}



const std::string &Uuid::Value() const
{
	if(value.empty())
		value = Random::UUID();
	
	return value;
}
