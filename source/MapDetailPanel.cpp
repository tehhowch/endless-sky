/* MapDetailPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapDetailPanel.h"

#include "Color.h"
#include "Command.h"
#include "Engine.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Government.h"
#include "MapOutfitterPanel.h"
#include "MapShipyardPanel.h"
#include "pi.h"
#include "Person.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "PointerShader.h"
#include "Politics.h"
#include "Radar.h"
#include "RingShader.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "System.h"
#include "Trade.h"
#include "UI.h"
#include "WrappedText.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

using namespace std;

namespace {
	// Convert the angle between two vectors into a sortable angle, i.e an angle
	// plus a length that is used as a tie-breaker.
	pair<double, double> SortAngle(const Point &reference, const Point &point)
	{
		// Rotate the given point by the reference amount.
		Point rotated(reference.Dot(point), reference.Cross(point));
		
		// This will be the tiebreaker value: the length, squared.
		double length = rotated.Dot(rotated);
		// Calculate the angle, but rotated 180 degrees so that the discontinuity
		// comes at the reference angle rather than directly opposite it.
		double angle = atan2(-rotated.Y(), -rotated.X());
		
		// Special case: collinear with the reference vector. If the point is
		// a longer vector than the reference, it's the very best angle.
		// Otherwise, it is the very worst angle. (Note: this also is applied if
		// the angle is opposite (angle == 0) but then it's a no-op.)
		if(!rotated.Y())
			angle = copysign(angle, rotated.X() - reference.Dot(reference));
		
		// Return the angle, plus the length as a tie-breaker.
		return make_pair(angle, length);
	}
}



MapDetailPanel::MapDetailPanel(PlayerInfo &player, const System *system)
	: MapPanel(player, system ? MapPanel::SHOW_REPUTATION : player.MapColoring(), system)
{
	shipSystems = GetSystemShipsDrawList();
}



MapDetailPanel::MapDetailPanel(const MapPanel &panel)
	: MapPanel(panel)
{
	// Use whatever map coloring is specified in the PlayerInfo.
	commodity = player.MapColoring();
	shipSystems = GetSystemShipsDrawList();
}



void MapDetailPanel::Draw()
{
	MapPanel::Draw();
	
	DrawKey();
	DrawInfo();
	DrawOrbits();
}



// Only override the ones you need; the default action is to return false.
bool MapDetailPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command)
{
	if((key == SDLK_TAB || command.Has(Command::JUMP)) && player.Flagship())
	{
		// Clear the selected planet, if any.
		selectedPlanet = nullptr;
		// Toggle to the next link connected to the "source" system. If the
		// shift key is down, the source is the end of the travel plan; otherwise
		// it is one step before the end.
		vector<const System *> &plan = player.TravelPlan();
		const System *source = plan.empty() ? player.GetSystem() : plan.front();
		const System *next = nullptr;
		Point previousUnit = Point(0., -1.);
		if(!plan.empty() && !(mod & KMOD_SHIFT))
		{
			previousUnit = plan.front()->Position();
			plan.erase(plan.begin());
			next = source;
			source = plan.empty() ? player.GetSystem() : plan.front();
			previousUnit = (previousUnit - source->Position()).Unit();
		}
		Point here = source->Position();
		const System *original = next;
		
		// Depending on whether the flagship has a jump drive, the possible links
		// we can travel along are different:
		bool hasJumpDrive = player.Flagship()->Attributes().Get("jump drive");
		const set<const System *> &links = hasJumpDrive ? source->Neighbors() : source->Links();
		
		// For each link we can travel from this system, check whether the link
		// is closer to the current angle (while still being larger) than any
		// link we have seen so far.
		auto bestAngle = make_pair(4., 0.);
		for(const System *it : links)
		{
			// Skip the currently selected link, if any. Also skip links to
			// systems the player has not seen, and skip hyperspace links if the
			// player has not visited either end of them.
			if(it == original)
				continue;
			if(!player.HasSeen(it))
				continue;
			if(!(hasJumpDrive || player.HasVisited(it) || player.HasVisited(source)))
				continue;
			
			// Generate a sortable angle with vector length as a tiebreaker.
			// Otherwise if two systems are in exactly the same direction it is
			// not well defined which one comes first.
			auto angle = SortAngle(previousUnit, it->Position() - here);
			if(angle < bestAngle)
			{
				next = it;
				bestAngle = angle;
			}
		}
		if(next)
		{
			plan.insert(plan.begin(), next);
			Select(next);
		}
	}
	else if((key == SDLK_DELETE || key == SDLK_BACKSPACE) && player.HasTravelPlan())
	{
		vector<const System *> &plan = player.TravelPlan();
		plan.erase(plan.begin());
		Select(plan.empty() ? player.GetSystem() : plan.front());
	}
	else if(key == SDLK_DOWN)
	{
		if(commodity < 0 || commodity == 9)
			SetCommodity(0);
		else
			SetCommodity(commodity + 1);
	}
	else if(key == SDLK_UP)
	{
		if(commodity <= 0)
			SetCommodity(9);
		else
			SetCommodity(commodity - 1);
	}
	else
		return MapPanel::KeyDown(key, mod, command);
	
	return true;
}



bool MapDetailPanel::Click(int x, int y, int clicks)
{
	if(x < Screen::Left() + 160)
	{
		if(y >= tradeY && y < tradeY + 200)
		{
			SetCommodity((y - tradeY) / 20);
			return true;
		}
		else if(y < governmentY)
			SetCommodity(SHOW_REPUTATION);
		else if(y >= governmentY && y < governmentY + 20)
			SetCommodity(SHOW_GOVERNMENT);
		else
		{
			for(const auto &it : planetY)
				if(y >= it.second && y < it.second + 110)
				{
					selectedPlanet = it.first;
					if(y >= it.second + 30 && y < it.second + 110)
					{
						// Figure out what row of the planet info was clicked.
						int row = (y - (it.second + 30)) / 20;
						static const int SHOW[4] = {
							SHOW_REPUTATION, SHOW_SHIPYARD, SHOW_OUTFITTER, SHOW_VISITED};
						SetCommodity(SHOW[row]);
						
						if(clicks > 1 && SHOW[row] == SHOW_SHIPYARD)
						{
							GetUI()->Pop(this);
							GetUI()->Push(new MapShipyardPanel(*this, true));
						}
						if(clicks > 1 && SHOW[row] == SHOW_OUTFITTER)
						{
							GetUI()->Pop(this);
							GetUI()->Push(new MapOutfitterPanel(*this, true));
						}
					}
					return true;
				}
		}
	}
	else if(x >= Screen::Right() - 240 && y >= Screen::Top() + 280 && y <= Screen::Top() + 520)
	{
		// The player clicked within the orbits scene. Select either a
		// planet or a ship, depending which is closest.
		Point click = Point(x, y);
		selectedPlanet = nullptr;
		selectedShip = nullptr;
		double distance = numeric_limits<double>::infinity();
		for(const auto &it : planets)
		{
			double d = click.Distance(it.second);
			if(d < distance)
			{
				distance = d;
				selectedPlanet = it.first;
			}
		}
		shared_ptr<Ship> newTargetShip;
		for(const auto &shipIt : drawnShips)
		{
			double d = click.Distance(shipIt.second);
			if(d < distance)
			{
				distance = d;
				selectedShip = shipIt.first.get();
				newTargetShip = const_cast<Ship *>(&*shipIt.first)->shared_from_this();
			}
		}
		// Set the clicked ship as the player's new targeted ship.
		if(selectedShip && player.Flagship() && newTargetShip.get() != player.Flagship())
			player.Flagship()->SetTargetShip(newTargetShip);
		if(selectedShip)
		{
			SetCommodity(SHOW_SHIP_LOCATIONS);
			selectedPlanet = nullptr;
		}
		if(selectedPlanet && player.Flagship())
			player.SetTravelDestination(selectedPlanet);
		
		return true;
	}
	else if(y >= Screen::Bottom() - 40 && x >= Screen::Right() - 335 && x < Screen::Right() - 265)
	{
		// The user clicked the "done" button.
		return DoKey(SDLK_d);
	}
	else if(y >= Screen::Bottom() - 40 && x >= Screen::Right() - 415 && x < Screen::Right() - 345)
	{
		// The user clicked the "missions" button.
		return DoKey(SDLK_PAGEDOWN);
	}
	
	MapPanel::Click(x, y, clicks);
	if(selectedPlanet && !selectedPlanet->IsInSystem(selectedSystem))
		selectedPlanet = nullptr;
	if(selectedShip && selectedShip->GetSystem() != selectedSystem)
		selectedShip = nullptr;
	return true;
}



void MapDetailPanel::DrawKey()
{
	const Sprite *back = SpriteSet::Get("ui/map key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));
	
	Color bright(.6, .6);
	Color dim(.3, .3);
	const Font &font = FontSet::Get(14);
	
	Point pos(Screen::Left() + 10., Screen::Bottom() - 7. * 20. + 5.);
	Point headerOff(-5., -.5 * font.Height());
	Point textOff(10., -.5 * font.Height());
	
	static const string HEADER[] = {
		"Trade prices:",
		"Ships for sale:",
		"Outfits for sale:",
		"You have visited:",
		"", // Special should never be active in this mode.
		"Government:",
		"System:",
		"System Fleets:"
	};
	const string &header = HEADER[-min(0, max(-7, commodity))];
	font.Draw(header, pos + headerOff, bright);
	pos.Y() += 20.;
	
	if(commodity >= 0)
	{
		const vector<Trade::Commodity> &commodities = GameData::Commodities();
		const auto &range = commodities[commodity];
		if(static_cast<unsigned>(commodity) >= commodities.size())
			return;
		
		for(int i = 0; i <= 3; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(i * (2. / 3.) - 1.));
			int price = range.low + ((range.high - range.low) * i) / 3;
			font.Draw(Format::Number(price), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity >= SHOW_OUTFITTER)
	{
		static const string LABEL[2][4] = {
			{"None", "1", "5", "10+"},
			{"None", "1", "30", "60+"}};
		static const double VALUE[4] = {-1., 0., .5, 1.};
		
		for(int i = 0; i < 4; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(VALUE[i]));
			font.Draw(LABEL[commodity == SHOW_OUTFITTER][i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_VISITED)
	{
		static const string LABEL[3] = {
			"All planets",
			"Some",
			"None"
		};
		for(int i = 0; i < 3; ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, MapColor(1 - i));
			font.Draw(LABEL[i], pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_GOVERNMENT)
	{
		vector<pair<double, const Government *>> distances;
		for(const auto &it : closeGovernments)
			distances.emplace_back(it.second, it.first);
		sort(distances.begin(), distances.end());
		for(unsigned i = 0; i < 4 && i < distances.size(); ++i)
		{
			RingShader::Draw(pos, OUTER, INNER, GovernmentColor(distances[i].second));
			font.Draw(distances[i].second->GetName(), pos + textOff, dim);
			pos.Y() += 20.;
		}
	}
	else if(commodity == SHOW_REPUTATION)
	{
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(1e-1, true, false));
		RingShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(1e2, true, false));
		RingShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(1e4, true, false));
		font.Draw("Friendly", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(-1e-1, false, false));
		RingShader::Draw(pos + Point(12., 0.), OUTER, INNER, ReputationColor(-1e2, false, false));
		RingShader::Draw(pos + Point(24., 0.), OUTER, INNER, ReputationColor(-1e4, false, false));
		font.Draw("Hostile", pos + textOff + Point(24., 0.), dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, false));
		font.Draw("Restricted", pos + textOff, dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ReputationColor(0., false, true));
		font.Draw("Dominated", pos + textOff, dim);
		pos.Y() += 20.;
	}
	else if(commodity == SHOW_SHIP_LOCATIONS)
	{
		RingShader::Draw(pos, OUTER, INNER, ShipColor(1, 0, 1));
		font.Draw("Escort only", pos + textOff, dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ShipColor(3, 0, 4));
		RingShader::Draw(pos + Point(9.,0.), OUTER, INNER, ShipColor(3, 2, 6));
		RingShader::Draw(pos + Point(18.,0.), OUTER, INNER, ShipColor(3, 3, 6));
		RingShader::Draw(pos + Point(27.,0.), OUTER, INNER, ShipColor(2, 3, 6));
		RingShader::Draw(pos + Point(36.,0.), OUTER, INNER, ShipColor(0, 3, 4));
		font.Draw("Mixed", pos + textOff + Point(36., 0.), dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ShipColor(0, 1, 1));
		font.Draw("Hostile only", pos + textOff, dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ShipColor(0, 0, 1));
		font.Draw("Neutral only", pos + textOff, dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, ShipColor(0, 0, 0));
		font.Draw("Unknown", pos + textOff, dim);
		pos.Y() += 20.;
		
		RingShader::Draw(pos, OUTER, INNER, UnexploredColor());
		font.Draw("Unexplored", pos + textOff, dim);
		pos.Y() += 20.;
		
		return;
	}
	
	RingShader::Draw(pos, OUTER, INNER, UninhabitedColor());
	font.Draw("Uninhabited", pos + textOff, dim);
	pos.Y() += 20.;
	
	RingShader::Draw(pos, OUTER, INNER, UnexploredColor());
	font.Draw("Unexplored", pos + textOff, dim);
}



void MapDetailPanel::DrawInfo()
{
	Color dimColor(.1, 0.);
	Color closeColor(.6, .6);
	Color farColor(.3, .3);
	
	Point uiPoint(Screen::Left() + 100., Screen::Top() + 45.);
	
	// System sprite goes from 0 to 90.
	const Sprite *systemSprite = SpriteSet::Get("ui/map system");
	SpriteShader::Draw(systemSprite, uiPoint);
	
	const Font &font = FontSet::Get(14);
	string systemName = player.KnowsName(selectedSystem) ?
		selectedSystem->Name() : "Unexplored System";
	font.Draw(systemName, uiPoint + Point(-90., -7.), closeColor);
	
	governmentY = uiPoint.Y() + 10.;
	string gov = player.HasVisited(selectedSystem) ?
		selectedSystem->GetGovernment()->GetName() : "Unknown Government";
	font.Draw(gov, uiPoint + Point(-90., 13.), (commodity == SHOW_GOVERNMENT) ? closeColor : farColor);
	if(commodity == SHOW_GOVERNMENT)
		PointerShader::Draw(uiPoint + Point(-90., 20.), Point(1., 0.),
			10., 10., 0., closeColor);
	
	uiPoint.Y() += 115.;
	
	planetY.clear();
	if(player.HasVisited(selectedSystem))
	{
		set<const Planet *> shown;
		const Sprite *planetSprite = SpriteSet::Get("ui/map planet");
		for(const StellarObject &object : selectedSystem->Objects())
			if(object.GetPlanet())
			{
				// Allow the same "planet" to appear multiple times in one system.
				const Planet *planet = object.GetPlanet();
				if(planet->IsWormhole() || !planet->IsAccessible(player.Flagship()) || shown.count(planet))
					continue;
				shown.insert(planet);
				
				SpriteShader::Draw(planetSprite, uiPoint);
				planetY[planet] = uiPoint.Y() - 60;
			
				font.Draw(object.Name(),
					uiPoint + Point(-70., -52.),
					planet == selectedPlanet ? closeColor : farColor);
				
				bool hasSpaceport = planet->HasSpaceport();
				string reputationLabel = !hasSpaceport ? "No Spaceport" :
					GameData::GetPolitics().HasDominated(planet) ? "Dominated" :
					planet->GetGovernment()->IsEnemy() ? "Hostile" :
					planet->CanLand() ? "Friendly" : "Restricted";
				font.Draw(reputationLabel,
					uiPoint + Point(-60., -32.),
					hasSpaceport ? closeColor : dimColor);
				if(commodity == SHOW_REPUTATION)
					PointerShader::Draw(uiPoint + Point(-60., -25.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				font.Draw("Shipyard",
					uiPoint + Point(-60., -12.),
					planet->HasShipyard() ? closeColor : dimColor);
				if(commodity == SHOW_SHIPYARD)
					PointerShader::Draw(uiPoint + Point(-60., -5.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				font.Draw("Outfitter",
					uiPoint + Point(-60., 8.),
					planet->HasOutfitter() ? closeColor : dimColor);
				if(commodity == SHOW_OUTFITTER)
					PointerShader::Draw(uiPoint + Point(-60., 15.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				bool hasVisited = player.HasVisited(planet);
				font.Draw(hasVisited ? "(has been visited)" : "(not yet visited)",
					uiPoint + Point(-70., 28.),
					farColor);
				if(commodity == SHOW_VISITED)
					PointerShader::Draw(uiPoint + Point(-70., 35.), Point(1., 0.),
						10., 10., 0., closeColor);
				
				uiPoint.Y() += 130.;
			}
	}
	
	uiPoint.Y() += 45.;
	tradeY = uiPoint.Y() - 95.;
	
	// Trade sprite goes from 310 to 540.
	const Sprite *tradeSprite = SpriteSet::Get("ui/map trade");
	SpriteShader::Draw(tradeSprite, uiPoint);
	
	uiPoint.X() -= 90.;
	uiPoint.Y() -= 97.;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		bool isSelected = false;
		if(static_cast<unsigned>(this->commodity) < GameData::Commodities().size())
			isSelected = (&commodity == &GameData::Commodities()[this->commodity]);
		Color &color = isSelected ? closeColor : farColor;
		
		font.Draw(commodity.name, uiPoint, color);
		
		string price;
		
		bool hasVisited = player.HasVisited(selectedSystem);
		if(hasVisited && selectedSystem->IsInhabited(player.Flagship()))
		{
			int value = selectedSystem->Trade(commodity.name);
			int localValue = (player.GetSystem() ? player.GetSystem()->Trade(commodity.name) : 0);
			// Don't "compare" prices if the current system is uninhabited and
			// thus has no prices to compare to.
			bool noCompare = (!player.GetSystem() || !player.GetSystem()->IsInhabited(player.Flagship()));
			if(!value)
				price = "----";
			else if(noCompare || player.GetSystem() == selectedSystem || !localValue)
				price = to_string(value);
			else
			{
				value -= localValue;
				price += "(";
				if(value > 0)
					price += '+';
				price += to_string(value);
				price += ")";
			}
		}
		else
			price = (hasVisited ? "n/a" : "?");
		
		Point pos = uiPoint + Point(140. - font.Width(price), 0.);
		font.Draw(price, pos, color);
		
		if(isSelected)
			PointerShader::Draw(uiPoint + Point(0., 7.), Point(1., 0.), 10., 10., 0., color);
		
		uiPoint.Y() += 20.;
	}
	
	// Display the selected ship or planet's description, if known.
	string fillText;
	if(selectedShip)
	{
		const Government *gov = selectedShip->GetGovernment();
		fillText += selectedShip->ModelName() + ": '" + selectedShip->Name() + "'\n";
		fillText += "Allegiance: " + (selectedShip->IsYours() ? "yours" : gov->GetName());
		fillText += (gov->Reputation() < 0. ? " (hostile)\n" : "\n");
		// Newly instantiated ships will properly display their description, but any ships loaded from
		// a savegame can only show a base model found in the store (i.e. the game loses track of
		// variant-specific descriptions). To fix this requires altering how ships are loaded.
		fillText += "\t" + (!selectedShip->Description().empty() ? selectedShip->Description()
				: GameData::Ships().Get(selectedShip->ModelName())->Description());
	}
	else if(selectedPlanet && !selectedPlanet->Description().empty() && player.HasVisited(selectedPlanet))
		fillText = selectedPlanet->Description();
	if(!fillText.empty())
	{
		const Sprite *panelSprite = SpriteSet::Get("ui/description panel");
		Point pos(Screen::Right() - .5 * panelSprite->Width(),
			Screen::Top() + .5 * panelSprite->Height());
		SpriteShader::Draw(panelSprite, pos);
		
		WrappedText text;
		text.SetFont(FontSet::Get(14));
		text.SetAlignment(WrappedText::JUSTIFIED);
		text.SetWrapWidth(480);
		text.Wrap(fillText);
		text.Draw(Point(Screen::Right() - 500, Screen::Top() + 20), closeColor);
	}
	
	DrawButtons("is ports");
}



void MapDetailPanel::DrawOrbits()
{
	// Draw the planet orbits in the currently selected system.
	const Sprite *orbitSprite = SpriteSet::Get("ui/orbits");
	Point orbitCenter(Screen::Right() - 120, Screen::Top() + 430);
	SpriteShader::Draw(orbitSprite, orbitCenter - Point(5., 0.));
	
	if(!selectedSystem || !player.HasVisited(selectedSystem))
		return;
	
	const Font &font = FontSet::Get(14);
	
	// Figure out what the largest orbit in this system is.
	double maxDistance = 0.;
	for(const StellarObject &object : selectedSystem->Objects())
		maxDistance = max(maxDistance, object.Position().Length() + object.Radius());
	
	// 2400 -> 120.
	double scale = .03;
	maxDistance *= scale;
	
	if(maxDistance > 115.)
		scale *= 115. / maxDistance;
	
	static const Color habitColor[7] = {
		Color(.4, .2, .2, 1.),
		Color(.3, .3, 0., 1.),
		Color(0., .4, 0., 1.),
		Color(0., .3, .4, 1.),
		Color(.1, .2, .5, 1.),
		Color(.2, .2, .2, 1.),
		Color(1., 1., 1., 1.)
	};
	// Draw orbital rings for each StellarObject, and a selection ring for the
	// selected planet, if any.
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point parentPos;
		int habit = 5;
		if(object.Parent() >= 0)
			parentPos = selectedSystem->Objects()[object.Parent()].Position();
		else
		{
			double warmth = object.Distance() / selectedSystem->HabitableZone();
			habit = (warmth > .5) + (warmth > .8) + (warmth > 1.2) + (warmth > 2.0);
		}
		
		double radius = object.Distance() * scale;
		RingShader::Draw(orbitCenter + parentPos * scale,
			radius + .7, radius - .7,
			habitColor[habit]);
		
		if(selectedPlanet && object.GetPlanet() == selectedPlanet)
			RingShader::Draw(orbitCenter + object.Position() * scale,
				object.Radius() * scale + 5., object.Radius() * scale + 4.,
				habitColor[6]);
	}
	
	planets.clear();
	// Shade the interior of any known landable planet.
	for(const StellarObject &object : selectedSystem->Objects())
	{
		if(object.Radius() <= 0.)
			continue;
		
		Point pos = orbitCenter + object.Position() * scale;
		if(object.GetPlanet() && object.GetPlanet()->IsAccessible(player.Flagship()))
			planets[object.GetPlanet()] = pos;
		
		const float *rgb = Radar::GetColor(object.RadarType(player.Flagship())).Get();
		// Darken and saturate the color, and make it opaque.
		Color color(max(0., rgb[0] * 1.2 - .2), max(0., rgb[1] * 1.2 - .2), max(0., rgb[2] * 1.2 - .2), 1.);
		RingShader::Draw(pos, object.Radius() * scale + 1., 0., color);
	}
	
	// Draw the name of the selected planet or ship in the orbits scene label.
	const string &name = selectedShip ? selectedShip->Name()
			: (selectedPlanet ? selectedPlanet->Name() : selectedSystem->Name());
	int width = font.Width(name);
	width = (width / 2) + 75;
	Point namePos(Screen::Right() - width - 5., Screen::Top() + 293.);
	Color nameColor(.6, .6);
	font.Draw(name, namePos, nameColor);
	
	// Draw any known ships in this system.
	DrawShips(orbitCenter, scale);
	
	// Draw the selected ship's sprite attached to the orbits panel.
	if(selectedShip && selectedShip->HasSprite())
	{
		static const double HEIGHT = 90.;
		static const double PAD = 9.;
		static const Color *overlayColors[4] = {
			GameData::Colors().Get("overlay friendly shields"),
			GameData::Colors().Get("overlay hostile shields"),
			GameData::Colors().Get("overlay friendly hull"),
			GameData::Colors().Get("overlay hostile hull")
		};
		const Sprite *boxSprite = SpriteSet::Get("ui/thumb box");
		const Sprite *shipSprite = selectedShip->GetSprite();
		Point boxPos(Screen::Right() - orbitSprite->Width() - .5 * boxSprite->Width() + PAD, orbitCenter.Y());
		Point shipPos(Screen::Right() - orbitSprite->Width() - .5 * HEIGHT + 5., boxPos.Y());
		// Scale to fit the sprite inside the 90x90 thumb box.
		double scale = min(.5, min((HEIGHT - 2.) / shipSprite->Height(), (HEIGHT - 2.) / shipSprite->Width()));
		SpriteShader::Draw(boxSprite, boxPos);
		SpriteShader::Draw(shipSprite, shipPos, scale, selectedShip->GetSwizzle());
		
		// Draw the ship's shields and hull as rings, as the targets interface does in-flight.
		const bool isEnemy = selectedShip->GetGovernment()->Reputation() < 0.;
		RingShader::Draw(shipPos, .5 * HEIGHT, 1.25, selectedShip->Shields(), *overlayColors[isEnemy], 0.);
		RingShader::Draw(shipPos, .5 * HEIGHT - 2., 1.25, selectedShip->Hull(), *overlayColors[2 + isEnemy], 20.);
	}
}


// Draw ships in the selected system as pointers, if the player has or
// knows of at least one ship in this system.
// TODO: Hook to the Engine instance to also draw non-special NPCs.
void MapDetailPanel::DrawShips(const Point &center, const double &scale)
{
	if(shipSystems.empty())
		return;
	
	// The player may have selected a new system with no known ships present.
	drawnShips.clear();
	
	const auto &it = shipSystems.find(selectedSystem);
	if(it == shipSystems.end())
		return;
	
	const vector<shared_ptr<const Ship>> &shipList = it->second;
	for(const shared_ptr<const Ship> &ship : shipList)
	{
		Point facing = ship->Facing().Unit();
		Point pos = center + (!ship->GetPlanet() ? ship->Position()
				: selectedSystem->FindStellar(ship->GetPlanet())->Position()) * scale;
		// Ship sprite radii range from 18 (Combat Drone) to 180 (World-Ship).
		// Scale the pointer by the sprite size, into the range (6 - 15).
		// TODO: Mod ships or new content may be larger than the World-Ship,
		// so this scaling equation should perhaps be dynamic or nonlinear.
		double size = 5 + ship->Radius() / 18;
		
		// If ships move outside the planetary orbits, draw the pointers at the edge
		// and dim them in accordance with how far from the edge they are.
		double alpha = 1.;
		if((pos - center).Length() > 115.)
		{
			alpha = 115. / (pos - center).Length();
			pos = alpha * (pos - center) + center;
		}
		// Allow clicking this ship to know its name:
		drawnShips[ship] = pos;
		
		// Use the ship's radar colors, after darkening and saturating.
		// Ships beyond the display radius are more translucent and less saturated.
		const float *rgb = Radar::GetColor(Engine::RadarType(*ship, step)).Get();
		const Color color(max(0., rgb[0] * 1.2 - .2) * alpha, max(0., rgb[1] * 1.2 - .2) * alpha,
				max(0., rgb[2] * 1.2 - .2) * alpha, alpha);
		const Color back(0., .85);
		// The pointer offset is half its height to center the body of the pointer
		// with the body of the ship. Outline each pointer with black for visibility.
		double edge = ship.get() == player.Flagship() ? 4. : 2.;
		PointerShader::Draw(pos, facing, size + edge, size + edge, (size + edge) / 2, back);
		PointerShader::Draw(pos, facing, size, size, size / 2, color);
		
		if(selectedShip && ship.get() == selectedShip)
			RingShader::Draw(pos, size, size - 1., color);
	}
}



// Find player ships and ships with personality escort. For systems with these
// ships, also find any other NPCs. Used to help color systems based on known ship locations.
// TODO: Hook to the Engine instance to find non-special NPCs in these known systems.
map<const System *, vector<shared_ptr<const Ship>>> MapDetailPanel::GetSystemShipsDrawList()
{
	map<const System *, vector<shared_ptr<const Ship>>> knownShipSystems;
	for(const shared_ptr<const Ship> &ship : player.Ships())
		if(ship->GetSystem() && !ship->IsParked())
			knownShipSystems[ship->GetSystem()].emplace_back(ship);
	for(const Mission &mission : player.Missions())
		for(const NPC &npc : mission.NPCs())
			for(const shared_ptr<const Ship> &ship : npc.Ships())
				if(ship->GetSystem() && !ship->IsDestroyed() && ship->GetPersonality().IsEscort())
					knownShipSystems[ship->GetSystem()].emplace_back(ship);
	
	// Add non-escort NPCs that are in "known" systems to the ship vectors.
	for(const Mission &mission : player.Missions())
		for(const NPC &npc : mission.NPCs())
			for(const shared_ptr<const Ship> &ship : npc.Ships())
				if(ship->GetSystem() && !ship->IsDestroyed() && ship->Cloaking() < 1.
						&& !ship->GetPersonality().IsEscort())
				{
					auto it = knownShipSystems.find(ship->GetSystem());
					if(it != knownShipSystems.end())
						it->second.emplace_back(ship);
				}
	
	// Also add persons that are also in known systems.
	for(const auto &pit : GameData::Persons())
		if(!pit.second.IsDestroyed() && pit.second.GetShip()->GetSystem())
		{
			auto it = knownShipSystems.find(pit.second.GetShip()->GetSystem());
			if(it != knownShipSystems.end())
				it->second.emplace_back(pit.second.GetShip());
		}
	
	return knownShipSystems;
}



// Set the commodity coloring, and update the player info as well.
void MapDetailPanel::SetCommodity(int index)
{
	commodity = index;
	player.SetMapColoring(commodity);
}
