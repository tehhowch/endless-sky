/* ItemInfoDisplay.h
Copyright (c) 2016 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ITEM_INFO_DISPLAY_H_
#define ITEM_INFO_DISPLAY_H_

#include "Point.h"
#include "WrappedText.h"

#include <string>
#include <vector>

class Sprite;
class Table;



// Base class representing a selected item in the shop, including a sprite
// to display and three panels of information about it. The first shows
// a text description, one shows the item's attributes, and a third may be
// different depending on what kind of item it is (a ship or an outfit).
class ItemInfoDisplay {
public:
	ItemInfoDisplay();
	virtual ~ItemInfoDisplay() = default;
	
	// Get/set the image used to display this item.
	const Sprite *Image() const;
	void SetImage(const Sprite *newImage);
	
	// Get the panel width.
	static int PanelWidth();
	// Get the height of each of the panels.
	int MaximumHeight() const;
	int DescriptionHeight() const;
	int AttributesHeight() const;
	
	// Draw each of the panels.
	void DrawDescription(const Point &topLeft) const;
	virtual void DrawAttributes(const Point &topLeft) const;
	// Draw information unique to the derived item info class.
	virtual void DrawOthers(const Point &topLeft) const = 0;
	
	void DrawTooltips() const;
	
	// Update the location where the mouse is hovering.
	void Hover(const Point &point);
	void ClearHover();
	
	
protected:
	void UpdateDescription(const std::string &text, const std::vector<std::string> &licenses, bool isShip);
	Point Draw(Point point, const std::vector<std::string> &labels, const std::vector<std::string> &values) const;
	void CheckHover(const Table &table, const std::string &label) const;
	
	
protected:
	static const int WIDTH = 250;
	
	const Sprite *itemImage = nullptr;
	
	int descriptionHeight = 0;
	WrappedText description;
	
	std::vector<std::string> attributeLabels;
	std::vector<std::string> attributeValues;
	int attributesHeight = 0;
	
	int maximumHeight = 0;
	
	// For tooltips:
	Point hoverPoint;
	mutable std::string hover;
	mutable int hoverCount = 0;
	bool hasHover = false;
	mutable WrappedText hoverText;
};



#endif

