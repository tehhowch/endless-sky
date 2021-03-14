/* test_uuid.cpp
Copyright (c) 2021 by Benjamin Hauch

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/Uuid.h"

// ... and any system includes needed for the test file.
#include <string>
#include <sstream>
#include <type_traits>

namespace { // test namespace

// #region mock data
struct Identifiable {
	Uuid id;
};
// #endregion mock data



// #region unit tests
TEST_CASE( "Uuid class", "[uuid]" ) {
	using T = Uuid;
	SECTION( "Class Traits" ) {
		CHECK_FALSE( std::is_trivial<T>::value );
		CHECK( std::is_standard_layout<T>::value );
		CHECK( std::is_nothrow_destructible<T>::value );
		// Class stores a string, and thus has the same behavior as that string.
		CHECK_FALSE( std::is_trivially_destructible<T>::value );
		CHECK( std::is_trivially_destructible<T>::value ==
			std::is_trivially_destructible<std::string>::value );
	}
	SECTION( "Construction Traits" ) {
		CHECK( std::is_default_constructible<T>::value );
		CHECK( std::is_nothrow_default_constructible<T>::value );
		CHECK_FALSE( std::is_copy_constructible<T>::value );
		CHECK_FALSE( std::is_trivially_copy_constructible<T>::value );
		CHECK_FALSE( std::is_nothrow_copy_constructible<T>::value );
		CHECK( std::is_move_constructible<T>::value );
		// Class stores a string, and thus has the same behavior as that string.
		CHECK_FALSE( std::is_trivially_move_constructible<T>::value );
		CHECK( std::is_trivially_move_constructible<T>::value ==
			std::is_trivially_move_constructible<std::string>::value );
		CHECK( std::is_nothrow_move_constructible<T>::value );
	}
	SECTION( "Copy Traits" ) {
		CHECK_FALSE( std::is_copy_assignable<T>::value );
		CHECK_FALSE( std::is_trivially_copyable<T>::value );
		CHECK_FALSE( std::is_trivially_copy_assignable<T>::value );
		CHECK_FALSE( std::is_nothrow_copy_assignable<T>::value );
	}
	SECTION( "Move Traits" ) {
		CHECK( std::is_move_assignable<T>::value );
		// Class stores a string, and thus has the same behavior as that string.
		CHECK_FALSE( std::is_trivially_move_assignable<T>::value );
		CHECK( std::is_trivially_move_assignable<T>::value ==
			std::is_trivially_move_assignable<std::string>::value );
		CHECK( std::is_nothrow_move_assignable<T>::value );
	}
}

SCENARIO( "Creating a UUID", "[uuid]") {
	GIVEN( "No arguments" ) {
		Uuid id;
		THEN( "it takes a random value" ) {
			CHECK_FALSE( id.ToString().empty() );
		}
	}
	GIVEN( "a reference string" ) {
		WHEN( "the string is valid" ) {
			const std::string valid = "5be91256-f6ba-47cd-96df-1ce1cb4fee86";
			THEN( "it takes the given value" ) {
				auto id = Uuid::FromString(valid);
				CHECK( id.ToString() == valid );
			}
		}
		WHEN( "the string is invalid" ) {
			THEN( "it takes a random value" ) {
				auto invalid = GENERATE(as<std::string>{}
					, "abcdef"
					, "ZZZZZZZZ-ZZZZ-ZZZZ-ZZZZ-ZZZZZZZZZZZZ"
					, "5be91256-f6ba-47cd-96df-1ce1cb-fee86"
				);
				auto id = Uuid::FromString(invalid);
				CHECK( id.ToString() != invalid );
			}
		}
	}
}

SCENARIO( "Comparing IDs", "[uuid]" ) {
	GIVEN( "a UUID" ) {
		Uuid id;
		THEN( "it always has the same string representation" ) {
			std::string value(id.ToString());
			CHECK( value == id.ToString() );
		}
		THEN( "it compares equal to itself" ) {
			CHECK( id == id );
		}
		AND_GIVEN( "a second UUID" ) {
			Uuid other;
			THEN( "the two are never equal" ) {
				CHECK( id != other );
			}
		}
	}
}

// Test code goes here. Preferably, use scenario-driven language making use of the SCENARIO, GIVEN,
// WHEN, and THEN macros. (There will be cases where the more traditional TEST_CASE and SECTION macros
// are better suited to declaration of the public API.)

// When writing assertions, prefer the CHECK and CHECK_FALSE macros when probing the scenario, and prefer
// the REQUIRE / REQUIRE_FALSE macros for fundamental / "validity" assertions. If a CHECK fails, the rest
// of the block's statements will still be evaluated, but a REQUIRE failure will exit the current block.

// #endregion unit tests



} // test namespace
