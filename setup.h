/*
 * Fritz!Box plugin for VDR
 *
 * Copyright (C) 2007-2012 Joachim Wilke <vdr@joachim-wilke.de>
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

#ifndef SETUP_H_
#define SETUP_H_

#include <string>
#include <vector>
#include <vdr/menuitems.h>
#include "libfritz++/Fonbooks.h"
#include <iostream>
#include "fritzeventhandler.h"
#include "fritzbox.h"

#define MAX_MSN_COUNT 22

class cMenuSetupFritzbox : public cMenuSetupPage
{
private:
	cPluginFritzbox	 *fritzbox;
	char *url;
	char *username;
	char *password;
	char *directions[3];
	char *menus[4];
	int reactOnDirection;
	int muteOnCall;
	int muteOnCallBefore;
	int muteAfterConnect;
	int muteVolumeLevel;
	int pauseOnCall;
	int pauseOnCallBefore;
	int pauseLive;
	int resumeAfterCall;
	int showNumber;
	int useNotifyOsd;
	int hideMainMenu;
	int defaultMenu;
	int showNumberInCallList;
	int showDaySeparator;
	std::string StoreMsn(void);
	std::string StoreFonbooks(void);
	bool locationSettingsDetected;
	char *countryCode;
	char *regionCode;
	int msnFilter;
	int msnFilterBefore;
	int msnCount;
	int msnCountBefore;
	char **msn;
	std::vector <std::string> selectedFonbookIDs;
	void Setup(void);
	eOSState ProcessKey(eKeys Key);
protected:
	virtual void Store(void);
public:
	cMenuSetupFritzbox(cPluginFritzbox *fritzbox);
	virtual ~cMenuSetupFritzbox();
};

class cMenuSetupFritzboxFonbooks : public cOsdMenu
{
private:
	fritz::Fonbooks *fonbooks;
	int  **selectedFonbookPos;
	char **fonbookTitles;
	size_t numberOfSelectedFonbooks;
	std::vector<std::string> *selectedFonbookIDs;
	void Setup(void);
	eOSState ProcessKey(eKeys Key);
public:
	cMenuSetupFritzboxFonbooks(std::vector<std::string> *selectedFonbookIDs);
	virtual ~cMenuSetupFritzboxFonbooks();
};

struct sFritzboxConfig {
public:
	enum eDirection {
		DIRECTION_IN,
		DIRECTION_OUT,
		DIRECTION_ANY,
	};
	sFritzboxConfig(void);
	bool SetupParseMsn(const char *value);
	bool SetupParseFonbooks(const char *value);
	bool SetupParse(const char *Name, const char *Value);
	std::string string2hex(std::string input);
	std::string hex2string(std::string input);
	std::string configDir;              // path to plugins' config files (e.g., local phone book)
	std::string pluginName;             // name of this plugin (e.g., for cRemote::CallPlugin)
	std::string lang;                   // webinterface language
	std::string url;                    // fritz!box url
	std::string username;               // fritz!box web interface username
	std::string password;               // fritz!box web interface password
	bool locationSettingsDetected;      // if true, location settings were autodetected by libfritz
	std::string countryCode;            // fritz!box country-code
	std::string regionCode;             // fritz!box region-code
	int reactOnDirection;               // what type of calls are we interested in (eDirection)?
	int muteOnCall;                     // mute audio on calls
	int muteAfterConnect;               // mute only after call connects
	int muteVolumeLevel;                // on mute, decrease volume by muteVolumeLevel %
	int pauseOnCall;                    // pause playback on calls
	int pauseLive;                      // pause livetv on calls, too
	int resumeAfterCall;                // resume playback after all calls have finished
	int showNumber;                     // show notification on osd on calls
	int useNotifyOsd;                   // use the extended notification osd and not Skins.Message
	int showNumberInCallList;           // simple or extended details in call lists
	time_t lastKnownMissedCall;         // the time of the last missed call the user is aware of
	int showDaySeparator;               // separate call lists by day
	int hideMainMenu;                   // hide plugins' main menu entry
	int defaultMenu;					// the menu that is displayed first when selecting the main menu entry
	std::string activeFonbookID;        // last shown phone book
	std::vector <std::string> msn;      // msn's we are interesed in
	std::vector <std::string> selectedFonbookIDs; // active phone books
};

extern sFritzboxConfig fritzboxConfig;

#endif /*SETUP_H_*/
