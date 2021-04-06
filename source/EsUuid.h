/* EsUuid.h
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

#if defined(_WIN32)
// Don't include <windows.h>, which will shadow our Rectangle class.
#define RPC_NO_WINDOWS_H
#include <rpc.h>
// #elif !defined(__APPLE__)
#else
#include <uuid/uuid.h>
#endif



// Class wrapping IETF v4 GUIDs, providing lazy initialization.
class EsUuid final {
public:
	// Used to represent a UUID across supported platforms.
	struct UuidType final {
#if defined(_WIN32)
		UUID id;
#elif defined(__APPLE__)
		std::string id;
#else
		uuid_t id;
#endif
	};
	
	
public:
	static EsUuid FromString(const std::string &input);
	EsUuid() noexcept = default;
	~EsUuid() noexcept = default;
	// Copying a UUID does not copy its value. (This allows us to use simple copy operations on stock
	// ship definitions when spawning fleets, etc.)
	EsUuid(const EsUuid &other);
	EsUuid &operator=(const EsUuid &other);
	// UUIDs can be moved as-is.
	EsUuid(EsUuid &&other) noexcept = default;
	EsUuid &operator=(EsUuid &&other) noexcept = default;
	
	// UUIDs can be compared against other UUIDs.
	bool operator==(const EsUuid &other) const;
	bool operator!=(const EsUuid &other) const;
	
	// Explicitly clone this UUID.
	void clone(const EsUuid &other);
	
	// Get a string representation of this ID, e.g. for serialization.
	std::string ToString() const;
	
	
private:
	// Internal constructor, from a string.
	explicit EsUuid(const std::string &input);
	// Lazy initialization getter.
	const UuidType &Value() const;
	
	
private:
	mutable UuidType value;
};



#endif
