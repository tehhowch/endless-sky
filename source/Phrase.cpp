/* Phrase.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Phrase.h"

#include "DataNode.h"
#include "GameData.h"
#include "Random.h"

#include <numeric>

using namespace std;

namespace {
	// Replaces all occurrences of the target string with the given string in-place.
	void ReplaceAll(string &text, const string &target, const string &replacement)
	{
		// If the searched string is an empty string, do nothing.
		if(target.empty())
			return;
		
		string newString;
		newString.reserve(text.size());
		
		// Index at which to begin searching for the target string.
		size_t start = 0;
		size_t matchLength = target.size(); 
		// Index at which the target string was found.
		size_t findPos = string::npos;
		while((findPos = text.find(target, start)) != string::npos)
		{
			newString.append(text, start, findPos - start);
			newString += replacement;
			start = findPos + matchLength;
		}
		
		// Add the remaining text.
		newString += text.substr(start);
		
		text.swap(newString);
	}
}



void Phrase::Load(const DataNode &node)
{
	// Set the name of this phrase, so we know it has been loaded.
	name = node.Size() >= 2 ? node.Token(1) : "Unnamed Phrase";
	// To avoid a possible parsing ambiguity, the interpolation delimiters
	// may not be used in a Phrase's name.
	if(name.find("${") != string::npos || name.find('}') != string::npos)
	{
		node.PrintTrace("Phrase names may not contain '${' or '}':");
		return;
	}
	
	sentences.emplace_back(node, this);
	if(sentences.back().empty())
	{
		sentences.pop_back();
		node.PrintTrace("Skipping unparseable node:");
	}
}



// Get the name associated with the node this phrase was instantiated
// from, or "Unnamed Phrase" if it was anonymously defined.
const string &Phrase::Name() const
{
	return name;
}



// Get a random sentence's text.
string Phrase::Get() const
{
	string result;
	if(sentences.empty())
		return result;
	
	for(const auto &part : sentences[Random::Int(sentences.size())])
	{
		if(!part.choices.empty())
		{
			const auto &choice = part.choices[Random::Int(part.choices.size())];
			result += choice.Get();
		}
		else if(!part.replaceRules.empty())
			for(const auto &f : part.replaceRules)
				f(result);
	}
	
	return result;
}



// Inspect this phrase and all its subphrases to determine if a cyclic
// reference exists between this phrase and the other.
bool Phrase::ReferencesPhrase(const Phrase *other) const
{
	if(other == this)
		return true;
	
	for(const auto &sentence : sentences)
		for(const auto &part : sentence)
			for(const auto &choice : part.choices)
				for(const auto &element : choice.sequence)
					if(element.second && element.second->ReferencesPhrase(other))
						return true;
	
	return false;
}



Phrase::Choice::Choice(const DataNode &node, bool isPhraseName)
{
	// The given datanode should not have any children.
	if(node.HasChildren())
		node.begin()->PrintTrace("Skipping unrecognized child node:");

	if(isPhraseName)
	{
		sequence.emplace_back(string{}, GameData::Phrases().Get(node.Token(0)));
		return;
	}
	
	// This node is a text string that may contain an interpolation request.
	const string &entry = node.Token(0);
	if(entry.empty())
	{
		// A blank choice was desired.
		sequence.emplace_back();
		return;
	}
	
	size_t start = 0;
	while(start < entry.size())
	{
		// Determine if there is an interpolation request in this string.
		size_t left = entry.find("${", start);
		if(left == string::npos)
			break;
		size_t right = entry.find('}', left);
		if(right == string::npos)
			break;
		
		// Add the text up to the ${, and then add the contained phrase name.
		++right;
		size_t length = right - left;
		auto text = string{entry, start, left - start};
		auto phraseName = string{entry, left + 2, length - 3};
		sequence.emplace_back(text, nullptr);
		sequence.emplace_back(string{}, GameData::Phrases().Get(phraseName));
		start = right;
	}
	// Add the remaining text to the sequence.
	if(entry.size() - start > 0)
		sequence.emplace_back(string(entry, start, entry.size() - start), nullptr);
}



// Convert this non-empty Choice into a text representation.
string Phrase::Choice::Get() const
{
	return accumulate(sequence.begin(), sequence.end(), string{},
		[](string &a, const pair<string, const Phrase *> &item)
		{
			return a += (!item.second ? item.first : item.second->Get());
		});
}



// Forwarding constructor, for use with emplace/emplace_back.
Phrase::Sentence::Sentence(const DataNode &node, const Phrase *parent)
{
	Load(node, parent);
}



// Parse the children of the given node to populate the sentence's structure.
void Phrase::Sentence::Load(const DataNode &node, const Phrase *parent)
{
	for(const DataNode &child : node)
	{
		if(!child.HasChildren())
		{
			child.PrintTrace("Skipping node with no children:");
			continue;
		}
		
		(*this).emplace_back();
		auto &part = (*this).back();
		
		if(child.Token(0) == "word")
			for(const DataNode &grand : child)
				part.choices.emplace_back(grand);
		else if(child.Token(0) == "phrase")
			for(const DataNode &grand : child)
				part.choices.emplace_back(grand, true);
		else if(child.Token(0) == "replace")
		{
			for(const DataNode &grand : child)
			{
				const string oldStr = grand.Token(0);
				const string newStr = grand.Size() >= 2 ? grand.Token(1) : "";
				part.replaceRules.emplace_back([oldStr, newStr](string &s) -> void
				{
					ReplaceAll(s, oldStr, newStr);
				});
			}
		}
		else
			child.PrintTrace("Skipping unrecognized attribute:");
		
		// Require any newly added phrases have no recursive references. Any recursions
		// will instead yield an empty string, rather than possibly infinite text.
		for(auto &choice : part.choices)
			for(auto &textOrPhrase : choice.sequence)
				if(textOrPhrase.second && textOrPhrase.second->ReferencesPhrase(parent))
				{
					child.PrintTrace("Replaced recursive '" + textOrPhrase.second->Name() + "' phrase reference with \"\":");
					textOrPhrase.second = nullptr;
				}
		
		// If no words, phrases, or replaces were given, discard this part of the phrase.
		if(part.choices.empty() && part.replaceRules.empty())
			(*this).pop_back();
	}
}
