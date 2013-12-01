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

#include "setup.h"
#include "menu.h"
#include "libfritz++/FonbookManager.h"
#include "libfritz++/CallList.h"
#include "libfritz++/Listener.h"
#include "libfritz++/Config.h"
#include "liblog++/Log.h"
#include <vdr/menuitems.h>
#include <vdr/i18n.h>

#if VDRVERSNUM < 10509
#define trVDR(s) tr(s)
#endif

sFritzboxConfig fritzboxConfig;

// possible characters for Fritz!Box password, according to web interface
const char *PasswordChars = "abcdefghijklmnopqrstuvwxyz0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

std::string cMenuSetupFritzbox::StoreMsn(){
	std::vector<std::string>::iterator it;
	std::string msnLine;

	for (it = fritzboxConfig.msn.begin(); it < fritzboxConfig.msn.end(); it++){
		msnLine += *it;
		msnLine += ' ';
	}
	return msnLine;
}

std::string cMenuSetupFritzbox::StoreFonbooks() {
	std::vector<std::string>::iterator it;
	std::string fbLine;


	for (it = fritzboxConfig.selectedFonbookIDs.begin(); it < fritzboxConfig.selectedFonbookIDs.end(); it++){
		fbLine += *it;
		fbLine += ' ';
	}
	return fbLine;
}

void cMenuSetupFritzbox::Setup(void) {
	// save current position
	int current = Current();
	// clear entries, if any
	Clear();
	int ret;
	//possible values for "React on calls"
	ret = asprintf(&directions[fritzboxConfig.DIRECTION_ANY], tr("any"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	ret = asprintf(&directions[fritzboxConfig.DIRECTION_IN],  tr("only incoming"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	ret = asprintf(&directions[fritzboxConfig.DIRECTION_OUT], tr("only outgoing"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	//possible values for "Default Menu"
	ret = asprintf(&menus[cMenuFritzbox::FONBUCH], tr("PB"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	ret = asprintf(&menus[cMenuFritzbox::IN], tr("incoming"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	ret = asprintf(&menus[cMenuFritzbox::OUT], tr("outgoing"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	ret = asprintf(&menus[cMenuFritzbox::MISSED], tr("missed"));
	if (ret <= 0) {
		ERR("Error allocating linebuffer for cOsdItem.");
		return;
	}
	// build up setup menu
	Add(new cMenuEditStrItem (tr("Fritz!Box URL"),                  		url,           			MaxFileName, tr(FileNameChars)));
	Add(new cMenuEditStrItem (tr("Username"),                  				username,           	MaxFileName, tr(FileNameChars)));
	Add(new cMenuEditStrItem (tr("Password"),                       		password, 	   			MaxFileName, PasswordChars));
	Add(new cMenuEditStrItem (tr("Country code"),                           countryCode,            5,           "0123456789"));
	Add(new cMenuEditStrItem (tr("Region code"),                            regionCode,             10,          "0123456789"));
	Add(new cMenuEditStraItem(tr("React on calls"),                         &reactOnDirection,      3,           directions  ));
	Add(new cMenuEditBoolItem(tr("Mute on call"),                   		&muteOnCall,   			trVDR("no"), trVDR("yes")));
	if (muteOnCall) {
		Add(new cMenuEditBoolItem(tr("Mute only after connect"),            &muteAfterConnect,      trVDR("no"), trVDR("yes")));
		Add(new cMenuEditIntItem(tr("Decrease volume by [1..100%]"),        &muteVolumeLevel,       1,           100));
	}
	Add(new cMenuEditBoolItem(tr("Pause on call"),   	             		&pauseOnCall,  			trVDR("no"), trVDR("yes")));
	if (pauseOnCall) {
		Add(new cMenuEditBoolItem(tr("Pause live tv"),                      &pauseLive,             trVDR("no"), trVDR("yes")));
		Add(new cMenuEditBoolItem(tr("Resume after call"),            		&resumeAfterCall,		trVDR("no"), trVDR("yes")));
	}
	Add(new cMenuEditBoolItem(tr("Show calls"),            		            &showNumber,   			trVDR("no"), trVDR("yes")));
	Add(new cMenuEditBoolItem(tr("Show detailed call information"),         &useNotifyOsd,          trVDR("no"), trVDR("yes")));
	Add(new cMenuEditBoolItem(tr("Detailed call lists"),                    &showNumberInCallList, 	trVDR("no"), trVDR("yes")));
	Add(new cMenuEditBoolItem(tr("Group call lists by date"),               &showDaySeparator,      trVDR("no"), trVDR("yes")));
	Add(new cMenuEditBoolItem(tr("Hide main menu entry"), 		    		&hideMainMenu, 			trVDR("no"), trVDR("yes")));
	Add(new cMenuEditStraItem(tr("Default menu"),                           &defaultMenu,           4,           menus       ));
	Add(new cOsdItem         (tr("Setup phonebooks to use..."),             osUser1                                          ));
	Add(new cMenuEditBoolItem(tr("Restrict monitor to certain extensions"), &msnFilter,    			trVDR("no"), trVDR("yes")));
	if (msnFilter) {
		Add(new cMenuEditIntItem (tr("Number of monitored extensions"), &msnCount, 0, MAX_MSN_COUNT));
		for (int p = 0; p < msnCount; p++)
			Add(new cMenuEditStrItem(tr("Extension"), msn[p], MaxFileName, "1234567890"));
	}
	SetHelp(tr("Reload"));
	// restore current position
	SetCurrent(Get(current));
	// refresh display
	Display();
}

eOSState cMenuSetupFritzbox::ProcessKey(eKeys Key) {

	eOSState state = cMenuSetupPage::ProcessKey(Key);

	if (state == osUser1) {
		return AddSubMenu(new cMenuSetupFritzboxFonbooks(&selectedFonbookIDs));
	}

	if (Key != kNone) {
		if (msnFilter != msnFilterBefore) {
			// deactivate MSN Filter
			if (msnFilter == 0) {
				msnCount = 0;
			} else {
				msnCount = 1;
			}
			msnFilterBefore = msnFilter;
		}
		if (msnCount != msnCountBefore) {
			// add new MSN elements
			if (msnCount > msnCountBefore) {
				for (int i=msnCountBefore; i<msnCount; i++) {
					msn[i] = (char *) malloc(MaxFileName * sizeof(char));
					msn[i][0] = 0;
				}
			}
			// remove existing MSN elements
			else {
				for (int i=msnCountBefore; i>msnCount; i--)
					free(msn[i-1]);
			}
			Setup();
			msnCountBefore = msnCount;
		}
		if (pauseOnCall != pauseOnCallBefore) {
			Setup();
			pauseOnCallBefore = pauseOnCall;
		}
		if (muteOnCall != muteOnCallBefore) {
			Setup();
			muteOnCallBefore = muteOnCall;
		}
	}

	if (state == osUnknown) {
		switch (Key) {
		case kRed:
			fritz::FonbookManager::GetFonbook()->reload();
			Skins.QueueMessage(mtInfo, tr("Retrieving phone book"));
			state = osContinue;
			break;
		default:
			break;
		}
	}
	return state;
}

void cMenuSetupFritzbox::Store(void) {

	fritzbox->Cancel();        // stop any pending initialization
	fritz::Config::Shutdown(); // clean up before changing the configuration

	fritzboxConfig.url            		= url;
	fritzboxConfig.username        		= username;
	int i = 0;
	// only store the password if it was changed
	while (password[i]) {
		if (password[i] != '*') {
			fritzboxConfig.password 	= password;
			break;
		}
		i++;
	}
	// accept empty password
	if (password[0] == 0)
		fritzboxConfig.password = "";
	//
	fritzboxConfig.reactOnDirection     = reactOnDirection;
	fritzboxConfig.muteOnCall     		= muteOnCall;
	fritzboxConfig.muteAfterConnect     = muteAfterConnect;
	fritzboxConfig.muteVolumeLevel      = muteVolumeLevel;
	fritzboxConfig.pauseOnCall     		= pauseOnCall;
	fritzboxConfig.pauseLive            = pauseLive;
	fritzboxConfig.resumeAfterCall 		= resumeAfterCall;
	fritzboxConfig.showNumber     		= showNumber;
	fritzboxConfig.useNotifyOsd         = useNotifyOsd;
	fritzboxConfig.showNumberInCallList = showNumberInCallList;
	fritzboxConfig.showDaySeparator     = showDaySeparator;
	fritzboxConfig.hideMainMenu   		= hideMainMenu;
	fritzboxConfig.defaultMenu          = defaultMenu;
	fritzboxConfig.msn.clear();
	for (int i=0; i < msnCount; i++) {
		std::string s = msn[i];
		fritzboxConfig.msn.push_back(s);
	}
	fritzboxConfig.selectedFonbookIDs   = selectedFonbookIDs;
    fritzboxConfig.countryCode          = countryCode;
    fritzboxConfig.regionCode           = regionCode;
	// remove any leading zeros from countryCode and regionCode,
	while (!fritzboxConfig.countryCode.empty() && fritzboxConfig.countryCode[0] == '0')
		fritzboxConfig.countryCode = fritzboxConfig.countryCode.substr(1);
	while (!fritzboxConfig.regionCode.empty() && fritzboxConfig.regionCode[0] == '0')
		fritzboxConfig.regionCode = fritzboxConfig.regionCode.substr(1);

	((cThread *)fritzbox)->Start(); // re-read configuration, notify libfritz++ about changes

	SetupStore("Url",          			url);
	SetupStore("Username",    			username);
	SetupStore("Password",     			""); // has been migrated to EncodedPassword
	SetupStore("EncodedPassword",       fritzboxConfig.string2hex(fritzboxConfig.password).c_str());
	SetupStore("ReactOnDirection",      reactOnDirection);
	SetupStore("MuteOnCall",   			muteOnCall);
	SetupStore("MuteAfterConnect",      muteAfterConnect);
	SetupStore("MuteVolumeLevel",       muteVolumeLevel);
	SetupStore("PauseOnCall",  			pauseOnCall);
	SetupStore("PauseLive",             pauseLive);
	SetupStore("ResumeAfterCall",       resumeAfterCall);
	SetupStore("ShowNumber",   			showNumber);
	SetupStore("UseNotifyOsd",          useNotifyOsd);
	SetupStore("ShowNumberInCallList", 	showNumberInCallList);
	SetupStore("ShowDaySeparator",      showDaySeparator);
	SetupStore("HideMainMenu", 			hideMainMenu);
	SetupStore("DefaultMenu",           defaultMenu);
	SetupStore("MsnList",      			StoreMsn().c_str());
	SetupStore("Fonbooks",              StoreFonbooks().c_str());
	SetupStore("CountryCode",           fritzboxConfig.countryCode.c_str());
	SetupStore("RegionCode",            fritzboxConfig.regionCode.c_str());
}

cMenuSetupFritzbox::cMenuSetupFritzbox(cPluginFritzbox *fritzbox)
{
	this->fritzbox = fritzbox;
	// copy setup to temporary parameters
	msn = (char **) malloc(MAX_MSN_COUNT * sizeof(char *));
	url             	 = strdup(fritzboxConfig.url.c_str());
	username          	 = strdup(fritzboxConfig.username.c_str());
	password        	 = strdup(fritzboxConfig.password.c_str());
	// the original password is not visible in the setup page
	// every single character of the password is displayed as "*"
	for (unsigned int i = 0; i < fritzboxConfig.password.length(); i++) {
		password[i] = '*';
	}
	reactOnDirection     = fritzboxConfig.reactOnDirection;
	muteOnCall      	 = fritzboxConfig.muteOnCall;
	muteOnCallBefore     = muteOnCall;
	muteAfterConnect     = fritzboxConfig.muteAfterConnect;
	muteVolumeLevel      = fritzboxConfig.muteVolumeLevel;
	pauseOnCall      	 = fritzboxConfig.pauseOnCall;
	pauseOnCallBefore    = pauseOnCall;
	pauseLive            = fritzboxConfig.pauseLive;
	resumeAfterCall      = fritzboxConfig.resumeAfterCall;
	showNumber      	 = fritzboxConfig.showNumber;
	useNotifyOsd         = fritzboxConfig.useNotifyOsd;
	showNumberInCallList = fritzboxConfig.showNumberInCallList;
	showDaySeparator     = fritzboxConfig.showDaySeparator;
	hideMainMenu    	 = fritzboxConfig.hideMainMenu;
	defaultMenu			 = fritzboxConfig.defaultMenu;
	msnCount        	 = fritzboxConfig.msn.size();
	msnCountBefore  	 = msnCount; // needed for menu refresh
	msnFilter       	 = fritzboxConfig.msn.empty() ? 0 : 1;
	msnFilterBefore 	 = msnFilter;
	selectedFonbookIDs   = fritzboxConfig.selectedFonbookIDs;
	countryCode          = strdup(fritzboxConfig.countryCode.c_str());
	regionCode           = strdup(fritzboxConfig.regionCode.c_str());

	size_t p = 0;
	for(std::vector<std::string>::iterator itStr = fritzboxConfig.msn.begin(); itStr < fritzboxConfig.msn.end(); itStr++) {
		msn[p] = (char *) malloc(MaxFileName * sizeof(char));
		snprintf(msn[p], MaxFileName, "%s", itStr->c_str());
		p++;
	}
	// build up menu entries
	Setup();
}

cMenuSetupFritzbox::~cMenuSetupFritzbox()
{
	// free up malloced space from constructor
	free(url);
	free(username);
	free(password);
	for (int i=0; i<msnCount; i++)
		free(msn[i]);
	free(msn);
	for (int i=0; i<3; i++)
		free(directions[i]);
	for (int i=0; i<4; i++)
		free(menus[i]);
	free(countryCode);
	free(regionCode);
}

cMenuSetupFritzboxFonbooks::cMenuSetupFritzboxFonbooks(std::vector<std::string> *selectedFonbookIDs)
:cOsdMenu(tr("Setup phonebooks to use"), 4)
{
	fonbooks = fritz::FonbookManager::GetFonbookManager()->getFonbooks();
	this->selectedFonbookIDs = selectedFonbookIDs;
	// copy setup to temporary parameters
	numberOfSelectedFonbooks = selectedFonbookIDs->size();
	selectedFonbookPos = (int **)  malloc(fonbooks->size() * sizeof(int *));
	fonbookTitles      = (char **) malloc(fonbooks->size() * sizeof(char *));
	for (size_t i=0; i<fonbooks->size(); i++) {
		int ret = asprintf(&fonbookTitles[i], "%s", tr((*fonbooks)[i]->getTitle().c_str()));
		if (ret <= 0) {
			ERR("Error allocating linebuffer for cOsdItem.");
		}
		selectedFonbookPos[i] = (int *) malloc(sizeof(int));
	}
	// build up menu entries
	SetHelp(tr("More"), tr("Less"), NULL, NULL);
	Setup();
}

cMenuSetupFritzboxFonbooks::~cMenuSetupFritzboxFonbooks()
{
	// free up malloced space from constructor
	for (size_t i=0; i<fonbooks->size(); i++) {
		free(fonbookTitles[i]);
		free(selectedFonbookPos[i]);
	}
	free(fonbookTitles);
	free(selectedFonbookPos);
}

void cMenuSetupFritzboxFonbooks::Setup(void) {
	size_t fbCount = fonbooks->size();
	// save current position
	int current = Current();
	// clear entries, if any
	Clear();
	// build up setup menu
	for (size_t i=0; i<numberOfSelectedFonbooks; i++) {
		char *numberStr;
		int ret = asprintf(&numberStr, "%i.", (int) (i+1));
		if (ret <= 0) {
			ERR("Error allocating linebuffer for cOsdItem.");
				return;
			}
		size_t pos = 0;
		if (i < selectedFonbookIDs->size())
			while (pos < fbCount &&
					(*fonbooks)[pos]->getTechId().compare((*selectedFonbookIDs)[i]) != 0)
				pos++;
		*(selectedFonbookPos[i]) = (int) pos;
		Add(new cMenuEditStraItem(numberStr, selectedFonbookPos[i], fbCount, fonbookTitles));
	}
	// restore current position
	SetCurrent(Get(current));
	// refresh display
	Display();
}

eOSState cMenuSetupFritzboxFonbooks::ProcessKey(eKeys Key) {

	eOSState state = cOsdMenu::ProcessKey(Key);

	if (Key != kNone) {
		switch (Key) {
		case kRed:
			if (numberOfSelectedFonbooks < fonbooks->size()) {
				numberOfSelectedFonbooks++;
				Setup();
			}
			state = osContinue;
			break;
		case kGreen:
			if (numberOfSelectedFonbooks > 0) {
				numberOfSelectedFonbooks--;
				Setup();
			}
			state = osContinue;
			break;
		case kOk:
			selectedFonbookIDs->clear();
			for (size_t i=0; i<numberOfSelectedFonbooks; i++) {
				std::string s = (*fonbooks)[*selectedFonbookPos[i]]->getTechId();
				selectedFonbookIDs->push_back(s);
			}
			state = osBack;
		default:
			break;
		}
	}
	return state;
}

sFritzboxConfig::sFritzboxConfig() {
	configDir               = "";
	pluginName              = "";
	lang                    = "";
	url             		= "fritz.box";
	username                = "";
	password        		= "";
	countryCode             = "49";
	regionCode              = "";
	reactOnDirection        = DIRECTION_IN;
	muteOnCall      		= 0;
	muteAfterConnect        = 0;
	muteVolumeLevel         = 100;
	pauseOnCall      		= 0;
	pauseLive               = 0;
	resumeAfterCall         = 1;
	showNumber      		= 1;
	useNotifyOsd            = 0;
	showNumberInCallList    = 0;
	lastKnownMissedCall     = 0;
	showDaySeparator        = 1;
	hideMainMenu    		= 0;
	defaultMenu             = cMenuFritzbox::FONBUCH;
	selectedFonbookIDs.push_back("FRITZ");
	activeFonbookID         = "FRITZ";
}

bool sFritzboxConfig::SetupParseMsn(const char *value){
	std::string currentMsn;
	unsigned int pos = 0;
	// walk through the complete value-line
	while (value[pos] != 0) {
		currentMsn.erase();
		// stop at each <space> or EOL
		while (value[pos] != ' ' && value[pos] != 0) {
			currentMsn += value[pos];
			pos++;
		}
		msn.push_back(currentMsn);
		// at a <space> we have to advance to the next MSN
		if (value[pos] != 0)
			pos++;
	}
	return true;
}

bool sFritzboxConfig::SetupParseFonbooks(const char *value){
	std::string currentFb;
	unsigned int pos = 0;
	selectedFonbookIDs.clear();
	// walk through the complete value-line
	while (value[pos] != 0) {
		currentFb.erase();
		// stop at each <space> or EOL
		while (value[pos] != ' ' && value[pos] != 0) {
			currentFb += value[pos];
			pos++;
		}
		selectedFonbookIDs.push_back(currentFb);
		// at a <space> we have to advance to the next MSN
		if (value[pos] != 0)
			pos++;
	}
	return true;
}

bool sFritzboxConfig::SetupParse(const char *name, const char *value) {
	if      (!strcasecmp(name, "Url"))          		url          		 = value;
	else if (!strcasecmp(name, "Username"))	            username         = value;
	else if (!strcasecmp(name, "Password"))	            password             = value;
	else if (!strcasecmp(name, "EncodedPassword"))      password             = hex2string(value);
	else if (!strcasecmp(name, "ReactOnDirection"))     reactOnDirection     = atoi(value);
	else if (!strcasecmp(name, "MuteOnCall"))   		muteOnCall   		 = atoi(value);
	else if (!strcasecmp(name, "MuteAfterConnect"))     muteAfterConnect   	 = atoi(value);
	else if (!strcasecmp(name, "MuteVolumeLevel"))      muteVolumeLevel   	 = atoi(value);
	else if (!strcasecmp(name, "PauseOnCall"))   		pauseOnCall   		 = atoi(value);
	else if (!strcasecmp(name, "PauseLive"))            pauseLive            = atoi(value);
	else if (!strcasecmp(name, "ResumeAfterCall"))      resumeAfterCall      = atoi(value);
	else if (!strcasecmp(name, "ShowNumber"))   		showNumber   		 = atoi(value);
	else if (!strcasecmp(name, "UseNotifyOsd"))         useNotifyOsd         = atoi(value);
	else if (!strcasecmp(name, "ShowNumberInCallList")) showNumberInCallList = atoi(value);
	else if (!strcasecmp(name, "LastKnownMissedCall"))  lastKnownMissedCall  = atoi(value);
	else if (!strcasecmp(name, "ShowDaySeparator"))     showDaySeparator     = atoi(value);
	else if (!strcasecmp(name, "HideMainMenu")) 		hideMainMenu 		 = atoi(value);
	else if (!strcasecmp(name, "DefaultMenu"))          defaultMenu          = atoi(value);
	else if (!strcasecmp(name, "ActiveFonbook"))        activeFonbookID      = value;
	else if (!strcasecmp(name, "MsnList"))      		return SetupParseMsn(value);
	else if (!strcasecmp(name, "Fonbooks"))      		return SetupParseFonbooks(value);
	else if (!strcasecmp(name, "CountryCode"))          countryCode          = value;
	else if (!strcasecmp(name, "RegionCode"))           regionCode           = value;
	else return false;
	return true;
}

std::string sFritzboxConfig::string2hex(std::string input) {
	std::stringstream output;
	for (std::string::iterator it = input.begin(); it < input.end(); it++)
		output << std::hex << static_cast<int>(*it);
	return output.str();
}

std::string sFritzboxConfig::hex2string(std::string input) {
	std::stringstream output;
	for (std::string::iterator it = input.begin(); it < input.end(); it += 2) {
		std::stringstream buffer;
		int value;
		buffer << it[0] << it[1] ;
		buffer >> std::hex >> value;
		output << static_cast<char>(value);
	}
	return output.str();
}
