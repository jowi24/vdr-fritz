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

#include <vector>
#include <unistd.h>
#include <getopt.h>
#include <vdr/remote.h>
#include <vdr/config.h>
#include "libfritz++/FonbookManager.h"
#include "libfritz++/Listener.h"
#include "libfritz++/Config.h"
#include "liblog++/Log.h"
#include "fritzbox.h"
#include "setup.h"
#include "notifyosd.h"
#include "menu.h"

static const char *VERSION        = "1.5.3";
static const char *DESCRIPTION    = trNOOP("Fritz Plugin for AVM Fritz!Box");
static const char *MAINMENUENTRY  = trNOOP("Fritz!Box");

cPluginFritzbox::cPluginFritzbox(void)
: cThread("Fritz Plugin Initialization")
{
	// Initialize any member variables here.
	// DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
	// VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
	event = NULL;
	logPersonalInfo = false;
	migratePassword = false;

	logger::Log::setPrefix("fritzbox");
	logger::Log::setCustomLogger(
		[](const std::string &message) { esyslog(message.c_str()); },
		[](const std::string &message) { isyslog(message.c_str()); },
		[](const std::string &message) { dsyslog(message.c_str()); }
	);
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
	return "  -p,     --log-personal-info   log personal information (e.g. passwords, phone numbers, ...)\n"
	       "  -c cmd, --on-call=cmd         call cmd on incoming or outgoing call events (see README)\n";
}

bool cPluginFritzbox::ProcessArgs(int argc, char *argv[])
{
	// Implement command line argument processing here if applicable.
    static struct option long_options[] = {
            { "log-personal-info", no_argument,       NULL, 'p' },
            { "on-call",           required_argument, NULL, 'c' },
            { NULL }
    };

    int c;
    while ((c = getopt_long(argc, argv, "pc:", long_options, NULL)) != -1) {
    switch (c) {
        case 'p':
				logPersonalInfo = true;
				DBG("Logging personal information requested");
                break;
        case 'c':
        		onCallCmd = optarg;
        		DBG("User defined command " << onCallCmd << " registered");
        		break;
        default:
                ERR("unknown command line option" << (char)c);
                return false;
        }
    }

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
	event = new cFritzEventHandler(onCallCmd);
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
	// Migrate old setup parameters
	if (migratePassword) {
		SetupStore("EncodedPassword", fritzboxConfig.string2hex(fritzboxConfig.password).c_str());
		SetupStore("Password",        "");
	}
	// Stop any background activities the plugin shall perform.
	fritz::Config::Shutdown();
	if (event)
		delete event;
//	if (dlog)
//		delete dlog;
//	if (ilog)
//		delete ilog;
//	if (elog)
//		delete elog;
}

void cPluginFritzbox::Housekeeping(void)
{
	// Perform any cleanup or other regular tasks.
}

void cPluginFritzbox::MainThreadHook(void)
{
	if (!fritzboxConfig.useNotifyOsd && event) {
		std::vector<int> ids = event->GetPendingCallIds();
		for (std::vector<int>::iterator it = ids.begin(); it < ids.end(); it++) {
			fritz::sCallInfo callInfo = event->GetCallInfo(*it);
			if (callInfo.localNumber.length() > 0) {
				Skins.Message(mtInfo, event->ComposeCallMessage(*it).c_str());
				event->NotificationDone(*it);
			}
		}
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
	fritz::CallList *callList = fritz::CallList::GetCallList(false);
	if (callList && callList->missedCalls(fritzboxConfig.lastKnownMissedCall) > 0) {
		std::string buffer = (callList->missedCalls(fritzboxConfig.lastKnownMissedCall) > 1) ? tr("missed calls") : tr("missed call");
		ssMainMenuEntry << " (" << callList->missedCalls(fritzboxConfig.lastKnownMissedCall) << " " << buffer << ")";
	}
	mainMenuEntry = ssMainMenuEntry.str();
	return fritzboxConfig.hideMainMenu ? NULL : mainMenuEntry.c_str();
}

cOsdObject *cPluginFritzbox::MainMenuAction(void)
{
	if (event && event->GetPendingCallIds().size() && !cNotifyOsd::isOpen()) {
		// called by cRemote::CallPlugin
		return new cNotifyOsd(event);
	}
	else
		// called by the user
		if (this->Running()) {
			Skins.Message(mtError, tr("Data not yet available."));
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
	if (!strcasecmp(Name, "Password")) {
		if (fritzboxConfig.password.size() > 0) {
			migratePassword = true;
			return true;
		}
		if (strlen(Value) > 0) {
			migratePassword = true;
		}
	}
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
	fritz::Config::Setup(fritzboxConfig.url, fritzboxConfig.username, fritzboxConfig.password, logPersonalInfo);
	fritz::Config::Init(&fritzboxConfig.locationSettingsDetected, &fritzboxConfig.countryCode, &fritzboxConfig.regionCode);
	fritz::Config::SetupConfigDir(fritzboxConfig.configDir);
	fritz::Config::SetupMsnFilter(fritzboxConfig.msn);
	fritz::FonbookManager::CreateFonbookManager(fritzboxConfig.selectedFonbookIDs, fritzboxConfig.activeFonbookID);
	fritz::CallList::CreateCallList();

	// Create FritzListener only if needed
	if (fritzboxConfig.showNumber || fritzboxConfig.pauseOnCall || fritzboxConfig.muteOnCall || onCallCmd.size())
		fritz::Listener::CreateListener(event);
}

std::string cPluginFritzbox::FonbookEntryTypeToName(const fritz::FonbookEntry::eType type, bool longName) {
	switch (type) {
	case fritz::FonbookEntry::TYPE_HOME:
		return longName ?
				tr("Private") :
				//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "home"
				tr("H");
	case fritz::FonbookEntry::TYPE_MOBILE:
		return longName ?
				tr("Mobile") :
				//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "mobile"
				tr("M");
	case fritz::FonbookEntry::TYPE_WORK:
		return longName ?
				tr("Business") :
				//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "work"
				tr("W");
	default:
		return "";
	}
}



VDRPLUGINCREATOR(cPluginFritzbox); // Don't touch this!
