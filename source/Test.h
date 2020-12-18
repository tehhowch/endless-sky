/* Test.h
Copyright (c) 2019 by Peter van der Meer

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ENDLESS_SKY_AC_TEST_H_
#define ENDLESS_SKY_AC_TEST_H_

#include "Command.h"
#include "ConditionSet.h"

#include <set>
#include <string>
#include <vector>

class DataNode;
class Planet;
class PlayerInfo;
class UI;
class System;



// Class representing a single test.
class Test {
public:
	// Status indicators for the test that we selected (if any).
	enum class Status {ACTIVE, KNOWN_FAILURE, MISSING_FEATURE};


public:
	// Class representing a single step in a test
	class TestStep {
	public:
		// The different types of teststeps.
		enum class Type {
			// Step that assigns a value to a condition. Does not cause the game to step.
			ASSIGN,
			// Step that verifies if a certain condition is true. Does not cause the game to step.
			ASSERT,
			// Branch with a label to jump to when the condition in child is true.
			// When a second label is given, then the second is to jump to on false.
			// Does not cause the game to step, except when no step was done since last BRANCH or GOTO.
			BRANCH,
			// Step that adds game-data, either in the config-directories or in the game directly.
			INJECT,
			// Step that performs input (key, mouse, command). Does cause the game to step (to proces the inputs).
			INPUT,
			// Invalid test-step type, should not be used in tests. Used to detect issues in test-framework.
			INVALID,
			// Label to jump to (similar as is done in conversations). Does not cause the game to step.
			LABEL,
			// Instructs the game to set navigation / travel plan to a target system
			NAVIGATE,
			// Sets the watchdog timer. No value or zero disables the watchdog. Non-zero gives
			// a watchdog in number of frames/steps.
			WATCHDOG,
		};

		// Result returned from a TestStep.
		enum class Result {
			// Teststep succesfull. Proceed with next teststep.
			DONE,
			// Teststep failed. Fail test. Exit program with non-zero exitcode.
			FAIL,
			// Teststep incomplete (waiting for a condition). Retry teststep in next frame-step.
			RETRY,
			// Teststep ok, but triggered a jump (GOTO or BRANCH to a label).
			GOTO
		};
	
	
	public:
		TestStep(Type stepType);
	
	
	public:
		Type stepType = Type::INVALID;
	};
	
	class Context {
	friend class Test;
	
	public:
		// Pointer to the test we are running.
		const Test *testToRun = nullptr;
	
	
	protected:
		// Teststep to run.
		unsigned int stepToRun = 0;
	};
	
	
public:
	const std::string &Name() const;
	std::string StatusText() const;

	// PlayerInfo, the gamePanels and the MenuPanels together give the state of
	// the game. We just provide them as parameter here, because they are not
	// available when the test got created (and they can change due to loading
	// and saving of games).
	void Step(Context &context, UI &menuPanels, UI &gamePanels, PlayerInfo &player) const;

	void Load(const DataNode &node);
	
	
private:
	void LoadSequence(const DataNode &node);


private:
	std::string name;
	Status status = Status::ACTIVE;
	std::vector<TestStep> steps;
};

#endif
