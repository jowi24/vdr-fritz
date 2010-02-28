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

#include <vector>
#include <vdr/remote.h>
#include <FonbookManager.h>
#include <Listener.h>
#include <Config.h>
#include "fritzbox.h"
#include "setup.h"
#include "notifyosd.h"
#include "menu.h"

static const char *VERSION        = "1.3.3pre2";
static const char *DESCRIPTION    = trNOOP("Fritz Plugin for AVM Fritz!Box");
static const char *MAINMENUENTRY  = trNOOP("Fritz!Box");

cPluginFritzbox::cPluginFritzbox(void)
: cThread("Fritz Plugin Initialization")
{
	// Initialize any member variables here.
	// DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
	// VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
	event = NULL;
}

cPluginFritzbox::~cPluginFritzbox()
{
	// Clean up after yourself!

}

const char *cPluginFritzbox::Version(void)
{
	return VERSION;
}

const char *cPluginFritzbox::Description(void)
{
	return tr(DESCRIPTION);
}

const char *cPluginFritzbox::CommandLineHelp(void)
{
	// Return a string that describes all known command line options.
	return NULL;
}

bool cPluginFritzbox::ProcessArgs(int argc, char *argv[])
{
	// Implement command line argument processing here if applicable.
	return true;
}

bool cPluginFritzbox::Initialize(void)
{
	fritzboxConfig.configDir = ConfigDirectory(Name());
	fritzboxConfig.pluginName = Name();
	return true;
}

bool cPluginFritzbox::Start(void)
{
	// first enable loggin to syslog
	dlog = new cLogStream(cLogBuf::DEBUG);
	elog = new cLogStream(cLogBuf::ERROR);
	ilog = new cLogStream(cLogBuf::INFO);
	// use logging objects with libfritz++
	fritz::Config::SetupLogging(dlog, ilog, elog);

	event = new cFritzEventHandler();
	// start new thread for plugin initialization (may take some time)
	cThread::Start();
	return true;
}


void cPluginFritzbox::Stop(void)
{
	// Store implicit setup parameters not visible / auto-detected in setup menu
	SetupStore("ActiveFonbook",       fritzboxConfig.activeFonbookID.c_str());
	SetupStore("CountryCode",         fritzboxConfig.countryCode.c_str());
	SetupStore("RegionCode",          fritzboxConfig.regionCode.c_str());
	// Stop any background activities the plugin shall perform.
	fritz::Listener::DeleteListener();
	fritz::CallList::DeleteCallList();
	fritz::FonbookManager::DeleteFonbookManager();
	if (event)
		delete event;
	if (dlog)
		delete dlog;
	if (ilog)
		delete ilog;
	if (elog)
		delete elog;
}

void cPluginFritzbox::Housekeeping(void)
{
	// Perform any cleanup or other regular tasks.
}

void cPluginFritzbox::MainThreadHook(void)
{
	if (!fritzboxConfig.useNotifyOsd && event) {
		fritz::sCallInfo *callInfo = event->GetCallInfo();
		if (callInfo)
			Skins.Message(mtInfo, event->ComposeCallMessage().c_str());
	}
}

cString cPluginFritzbox::Active(void)
{
	// Return a message string if shutdown should be postponed
	return NULL;
}

time_t cPluginFritzbox::WakeupTime(void)
{
	// Return custom wakeup time for shutdown script
	return 0;
}

const char *cPluginFritzbox::MainMenuEntry(void)
{
	std::ostringstream ssMainMenuEntry;
	ssMainMenuEntry << tr(MAINMENUENTRY);
	fritz::CallList *callList = fritz::CallList::getCallList(false);
	if (callList && callList->MissedCalls(fritzboxConfig.lastKnownMissedCall) > 0) {
		std::string buffer = (callList->MissedCalls(fritzboxConfig.lastKnownMissedCall) > 1) ? tr("missed calls") : tr("missed call");
		ssMainMenuEntry << " (" << callList->MissedCalls(fritzboxConfig.lastKnownMissedCall) << " " << buffer << ")";
	}
	mainMenuEntry = ssMainMenuEntry.str();
	return fritzboxConfig.hideMainMenu ? NULL : mainMenuEntry.c_str();
}

cOsdObject *cPluginFritzbox::MainMenuAction(void)
{
	if (event && event->GetCallInfo()) {
		// called by cRemote::CallPlugin
		return new cNotifyOsd(event);
	}
	else
		// called by the user
		if (this->Running()) {
			Skins.Message(mtError, tr("This phonebook is not yet available."));
			return NULL;
		}
		else
			return new cMenuFritzbox(this);
}

cMenuSetupPage *cPluginFritzbox::SetupMenu(void)
{
	// Return a setup menu in case the plugin supports one.
	return new cMenuSetupFritzbox(this);
}

bool cPluginFritzbox::SetupParse(const char *Name, const char *Value)
{
	// Parse your own setup parameters and store their values.
	return fritzboxConfig.SetupParse(Name, Value);
}

bool cPluginFritzbox::Service(const char *Id, void *Data)
{
	// Handle custom service requests from other plugins
	return false;
}

const char **cPluginFritzbox::SVDRPHelpPages(void)
{
	// Return help text for SVDRP commands this plugin implements
	return NULL;
}

cString cPluginFritzbox::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
	// Process SVDRP commands this plugin implements
	return NULL;
}

void cPluginFritzbox::Action() {

	// init libfritz++
	fritz::Config::Setup(fritzboxConfig.url, fritzboxConfig.password,
			&fritzboxConfig.locationSettingsDetected,
			&fritzboxConfig.countryCode, &fritzboxConfig.regionCode);
	fritz::Config::SetupConfigDir(fritzboxConfig.configDir);
	fritz::Config::SetupMsnFilter(fritzboxConfig.msn);
	fritz::FonbookManager::CreateFonbookManager(fritzboxConfig.selectedFonbookIDs, fritzboxConfig.activeFonbookID);
	fritz::CallList::CreateCallList();

	// Create FritzListener only if needed
	if (fritzboxConfig.showNumber || fritzboxConfig.pauseOnCall || fritzboxConfig.muteOnCall)
		fritz::Listener::CreateListener(event);
	else
		fritz::Listener::DeleteListener();
}

std::string cPluginFritzbox::FonbookEntryToName(const fritz::FonbookEntry::eType type) {
	switch (type) {
	case fritz::FonbookEntry::TYPE_HOME:
		//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "home"
		return tr("H");
	case fritz::FonbookEntry::TYPE_MOBILE:
		//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "mobile"
		return tr("M");
	case fritz::FonbookEntry::TYPE_WORK:
		//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "work"
		return tr("W");
	default:
		return "";
	}
}



VDRPLUGINCREATOR(cPluginFritzbox); // Don't touch this!
