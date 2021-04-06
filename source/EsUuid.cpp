/* EsUuid.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "EsUuid.h"

#include "Files.h"

// #if defined(__APPLE__)
// #include "Random.h"
// #include <algorithm>
// #endif

#include <stdexcept>

#if defined(_WIN32)
#include <windows.h>
// #elif !defined(__APPLE__)
#else
#include <uuid/uuid.h>
#if !defined(UUID_STR_LEN)
#define UUID_STR_LEN 37
#endif
#endif

namespace es_uuid {
namespace detail {
#if defined(_WIN32)
// #region TODO: export Files.cpp string helpers to avoid duplication.
std::wstring ToUTF16(const std::string &input)
{
	const auto page = CP_UTF8;
	std::wstring result;
	if(input.empty())
		return result;
	
	int size = MultiByteToWideChar(page, 0, &input[0], input.length(), nullptr, 0);
	result.resize(size);
	MultiByteToWideChar(page, 0, &input[0], input.length(), &result[0], size);
	
	return result;
}
std::string ToUTF8(const wchar_t *str)
{
	std::string result;
	if(!str || !*str)
		return result;
	
	const auto page = CP_UTF8;
	// The returned size will include the null character at the end.
	int size = WideCharToMultiByte(page, 0, str, -1, nullptr, 0, nullptr, nullptr) - 1;
	result.resize(size);
	WideCharToMultiByte(page, 0, str, -1, &result[0], size, nullptr, nullptr);
	
	return result;
}
// #endregion TODO

// Get a version 4 (random) Universally Unique Identifier (see IETF RFC 4122).
EsUuid::UuidType MakeUuid()
{
	EsUuid::UuidType value;
	RPC_STATUS status = UuidCreate(&value.id);
	if(status == RPC_S_UUID_LOCAL_ONLY)
		Files::LogError("Created locally unique UUID only");
	else if(status == RPC_S_UUID_NO_ADDRESS)
		throw std::runtime_error("Failed to create UUID");
	
	return value;
}

EsUuid::UuidType ParseUuid(const std::string &str)
{
	EsUuid::UuidType value;
	auto data = ToUTF16(str);
	RPC_STATUS status = UuidFromStringW(reinterpret_cast<RPC_WSTR>(&data[0]), &value.id);
	if(status == RPC_S_INVALID_STRING_UUID)
		throw std::invalid_argument("Cannot convert \"" + str + "\" into a UUID");
	else if(status != RPC_S_OK)
		throw std::runtime_error("Fatal error parsing \"" + str + "\" as a UUID");
	
	return value;
}

bool IsNil(const UUID &id)
{
	RPC_STATUS status;
	return UuidIsNil(const_cast<UUID *>(&id), &status);
}

std::string Serialize(const UUID &id)
{
	wchar_t *buf = nullptr;
	RPC_STATUS status = UuidToStringW(const_cast<UUID *>(&id), reinterpret_cast<RPC_WSTR *>(&buf));
	
	std::string result = (status == RPC_S_OK) ? ToUTF8(buf) : "";
	if(result.empty())
		Files::LogError("Failed to serialize UUID!");
	else
		RpcStringFreeW(reinterpret_cast<RPC_WSTR *>(&buf));
	
	return result;
}
// #elif defined(__APPLE__)
// // Get a version 4 (random) Universally Unique Identifier (see IETF RFC 4122).
// EsUuid::UuidType MakeUuid()
// {
// 	EsUuid::UuidType value;
// 	value.id = Random::UUID();
// 	return value;
// }

// EsUuid::UuidType ParseUuid(const std::string &input)
// {
// 	EsUuid::UuidType value;
// 	// The input must have the correct number of characters and contain the correct subset
// 	// of characters. This validation isn't exact, nor do we really require it to be, since
// 	// this is not a networked application.
// 	bool isValid = input.size() == 36
// 			&& std::count(input.begin(), input.end(), '-') == 4
// 			&& std::all_of(input.begin(), input.end(), [](const char &c) noexcept -> bool
// 			{
// 				return c == '-' || std::isxdigit(static_cast<unsigned char>(c));
// 			});
	
// 	if(isValid)
// 		value.id = input;
// 	else
// 		Files::LogError("Warning: Replacing invalid v4 UUID string \"" + input + "\"");
// 	return value;
// }

// bool IsNil(const std::string &s)
// {
// 	return s.empty();
// }

// std::string Serialize(const std::string &s)
// {
// 	return s;
// }
#else
EsUuid::UuidType MakeUuid()
{
	EsUuid::UuidType value;
	uuid_generate_random(value.id);
	return value;
}

EsUuid::UuidType ParseUuid(const std::string &str)
{
	EsUuid::UuidType value;
	auto result = uuid_parse(str.data(), value.id);
	if(result == -1)
		throw std::invalid_argument("Cannot convert \"" + str + "\" into a UUID");
	else if(result != 0)
		throw std::runtime_error("Fatal error parsing \"" + str + "\" as a UUID");
	
	return value;
}

bool IsNil(const uuid_t &id)
{
	return uuid_is_null(id) == 1;
}

std::string Serialize(const uuid_t &id)
{
	char buf[UUID_STR_LEN];
	uuid_unparse(id, buf);
	return std::string(buf);
}
#endif
}
}
using namespace es_uuid::detail;



EsUuid EsUuid::FromString(const std::string &input)
{
	return EsUuid(input);
}



// Copy-constructing a UUID returns an empty UUID.
EsUuid::EsUuid(const EsUuid &other)
	: value()
{
}



// Copy-assigning also results in an empty UUID.
EsUuid &EsUuid::operator=(const EsUuid &other)
{
	*this = EsUuid(other);
	return *this;
}



// Explicitly copy the value of the other UUID.
void EsUuid::clone(const EsUuid &other)
{
	value = other.Value();
}



bool EsUuid::operator==(const EsUuid &other) const
{
	return Value().id == other.Value().id;
}



bool EsUuid::operator!=(const EsUuid &other) const
{
	return Value().id != other.Value().id;
}



std::string EsUuid::ToString() const
{
	return Serialize(Value().id);
}



// Internal constructor. Note that the provided value may not be a valid v4 UUID, in which case an error is logged
// and we return a new UUID.
EsUuid::EsUuid(const std::string &input)
{
	try
	{
		value = ParseUuid(input);
	}
	catch (const std::invalid_argument &err)
	{
		Files::LogError(err.what());
	}
}



const EsUuid::UuidType &EsUuid::Value() const
{
	if(IsNil(value.id))
		value = MakeUuid();
	
	return value;
}
