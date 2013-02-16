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

#ifndef FRITZBOX_H_
#define FRITZBOX_H_

#include <vdr/plugin.h>
#include <sstream>
#include "libfritz++/Fonbook.h"
#include "libfritz++/Listener.h"
#include "libfritz++/CallList.h"
#include "fritzeventhandler.h"

class cPluginFritzbox : public cPlugin, cThread {
	friend class cMenuSetupFritzbox;
private:
	cFritzEventHandler *event;
	std::string mainMenuEntry;
	bool logPersonalInfo;
	std::string onCallCmd;
	bool migratePassword;
public:
	cPluginFritzbox(void);
	virtual ~cPluginFritzbox();
	virtual const char *Version(void);
	virtual const char *Description(void);
	virtual const char *CommandLineHelp(void);
	virtual bool ProcessArgs(int argc, char *argv[]);
	virtual bool Initialize(void);
	virtual bool Start(void);
	virtual void Stop(void);
	virtual void Housekeeping(void);
	virtual void MainThreadHook(void);
	virtual cString Active(void);
	virtual time_t WakeupTime(void);
	virtual const char *MainMenuEntry(void);
	virtual cOsdObject *MainMenuAction(void);
	virtual cMenuSetupPage *SetupMenu(void);
	virtual bool SetupParse(const char *Name, const char *Value);
	virtual bool Service(const char *Id, void *Data = NULL);
	virtual const char **SVDRPHelpPages(void);
	virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
	// from cThread:
	virtual void Action();
	// various things
	static std::string FonbookEntryTypeToName(const fritz::FonbookEntry::eType type, bool longName = false);

};

#endif /*FRITZBOX_H_*/
