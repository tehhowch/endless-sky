/* Uuid.h
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_UUID_H_
#define ES_UUID_H_

#include <string>



// Class wrapping IETF v4 GUIDs, providing lazy initialization.
class Uuid final {
public:
	static Uuid FromString(const std::string &input);
	Uuid() noexcept = default;
	~Uuid() noexcept = default;
	// Copying a UUID does not copy its value. (This allows us to use simple copy operations on stock
	// ship definitions when spawning fleets, etc.)
	Uuid(const Uuid &other);
	Uuid &operator=(const Uuid &other);
	// UUIDs can be moved as-is.
	Uuid(Uuid &&other) noexcept = default;
	Uuid &operator=(Uuid &&other) noexcept = default;
	
	// UUIDs can be compared against other UUIDs.
	bool operator==(const Uuid &other) const;
	bool operator!=(const Uuid &other) const;
	
	// Explicitly clone this UUID.
	void clone(const Uuid &other);
	
	// Get a string representation of this ID, e.g. for serialization.
	const std::string &ToString() const;
	
	
private:
	// Internal constructor, from a string.
	explicit Uuid(const std::string &input);
	// Lazy initialization getter.
	const std::string &Value() const;
	
	
private:
	// The internal representation of the UUID. For now, we store the UUID as an
	// arbitrary-length string, rather than the more correct collection of bytes.
	mutable std::string value;
};



#endif
