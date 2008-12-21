/*
 * Fritz!Box plugin for VDR
 *
 * Copyright (C) 2007 Joachim Wilke <vdr@joachim-wilke.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef MENU_H_
#define MENU_H_

#include <vdr/osdbase.h>
//#include <CallList.h>
//#include <FritzFonbook.h>

class cMenuFritzbox : public cOsdMenu
{
public:
	enum mode {
		IN     = 1,
		MISSED = 2,
		OUT    = 3,
		FONBUCH
	};
private:
	fritz::Fonbook *fonbuch;
	fritz::CallList *callList;
    mode currentMode;
public:
	cMenuFritzbox(fritz::CallList *callList);
	virtual ~cMenuFritzbox();
	virtual eOSState ProcessKey (eKeys Key);
	void DisplayFonbuch();
	void DisplayCalls(fritz::CallList::callType ct);
};

class cMenuCallDetail : public cOsdMenu
{
private:
	fritz::CallEntry *ce;
	void SetText(std::string text);
public:
	cMenuCallDetail(fritz::CallEntry *ce, cMenuFritzbox::mode mode, fritz::Fonbook *fonbuch);
	virtual eOSState ProcessKey (eKeys Key);
};

class cMenuFonbuchDetail : public cOsdMenu
{
private:
	std::string numbers[4];
public:
	cMenuFonbuchDetail(fritz::FonbookEntry *fe, fritz::Fonbook *fonbuch);
	virtual eOSState ProcessKey (eKeys Key);
};


class cKeyOsdItem : public cOsdItem
{
public:
	unsigned int key;
	cKeyOsdItem(const char * text, enum eOSState state, bool selectable, unsigned int key);
};

#endif /*MENU_H_*/
