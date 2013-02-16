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

#include <sstream>
#include <vector>
#include <vdr/menu.h>
#include <vdr/status.h>
#include "liblog++/Log.h"
#include "libfritz++/FonbookManager.h"
#include "libfritz++/FritzClient.h"
#include "menu.h"
#include "setup.h"

cMenuFritzbox::cMenuFritzbox(cPluginFritzbox *plugin)
:cOsdMenu("Fritz!Box", 1) // just dummy values
{
	this->plugin = plugin;
	switch(fritzboxConfig.defaultMenu) {
	case FONBUCH:
		DisplayFonbuch();
		break;
	case IN:
	case OUT:
	case MISSED:
		DisplayCalls((fritz::CallEntry::eCallType)(fritzboxConfig.defaultMenu));
		break;
	}
}

cMenuFritzbox::~cMenuFritzbox()
{
}

eOSState cMenuFritzbox::ProcessKey (eKeys Key) {
	fritz::Fonbook  *fonbook  = fritz::FonbookManager::GetFonbook();
	fritz::CallList *callList = fritz::CallList::GetCallList();

	eOSState state = cOsdMenu::ProcessKey(Key);
	fritz::CallEntry *ce = NULL;
	cKeyOsdItem* currentKeyItem = dynamic_cast<cKeyOsdItem*>(this->Get(Current()));
	if (state == osUnknown) {
		switch (Key) {
		case kOk:
			switch (currentMode) {
			case FONBUCH:
				if (currentKeyItem && fonbook->isDisplayable() && fonbook->isInitialized())
					state = AddSubMenu(new cMenuFonbuchDetail(fonbook->retrieveFonbookEntry(currentKeyItem->key)));
				break;
			case IN:
				if (currentKeyItem)
					ce = callList->retrieveEntry(fritz::CallEntry::INCOMING, currentKeyItem->key);
				if (ce)
					state = AddSubMenu(new cMenuCallDetail(ce, currentMode, fonbook));
				break;
			case OUT:
				if (currentKeyItem)
					ce = callList->retrieveEntry(fritz::CallEntry::OUTGOING, currentKeyItem->key);
				if (ce)
					state = AddSubMenu(new cMenuCallDetail(ce, currentMode, fonbook));
				break;
			case MISSED:
				if (currentKeyItem)
					ce = callList->retrieveEntry(fritz::CallEntry::MISSED, currentKeyItem->key);
				if (ce)
					state = AddSubMenu(new cMenuCallDetail(ce, currentMode, fonbook));
				break;
			}
			break;
		case kRed:
			if (currentMode == FONBUCH) {
				fritz::FonbookManager::GetFonbookManager()->nextFonbook();
			}
			DisplayFonbuch();
			state = osContinue;
			break;
		case kGreen:
		case kYellow:
		case kBlue:
			DisplayCalls((fritz::CallEntry::eCallType)(Key - kRed));
			state = osContinue;
			break;
		default:
			break;
		}
	}
	return state;
}

void cMenuFritzbox::DisplayFonbuch() {
	unsigned int nameWidth = 0;
	fritz::Fonbook  *fonbook  = fritz::FonbookManager::GetFonbook();
	currentMode = FONBUCH;
	SetTitle(tr(fonbook->getTitle().c_str()));
	Clear();
	if (fonbook->isInitialized() == false) {
		Add(new cOsdItem(tr("This phonebook is not yet available."), osUnknown, false));
		Add(new cOsdItem(tr("You may need to wait some minutes,"), osUnknown, false));
		Add(new cOsdItem(tr("otherwise there may be a network problem."), osUnknown, false));
	}
	else if (fonbook->isDisplayable() == false) {
		Add(new cOsdItem(tr("This phonebook is not displayable"), osUnknown, false));
	}
	else {
		for (size_t pos=0; pos < fonbook->getFonbookSize(); pos++) {
			const fritz::FonbookEntry *fe = fonbook->retrieveFonbookEntry(pos);
			if (fe) {
				bool firstEntry = true;
				for (size_t numberPos = 0; numberPos < fe->getSize(); numberPos++) {
					if (fe->getNumber(numberPos).empty())
						continue;
					// build the menu entries
					char *line;
					int ret = asprintf(&line,"%s\t%s\t%s", !firstEntry ? "" : fe->getName().c_str(), cPluginFritzbox::FonbookEntryTypeToName(fe->getType(numberPos)).c_str(), fe->getNumber(numberPos).c_str());
					if (ret <= 0) {
						ERR("Error allocating line buffer for cOsdItem.");
						continue;
					}
					if (fe->getName().length() > nameWidth)
						nameWidth = fe->getName().length();
					Add(new cKeyOsdItem(line, osUnknown, true, pos));
					firstEntry = false;
				}
			}
		}
	}
	SetCols(nameWidth+1, 2);
	SetHelp(tr("> PB"), "|<-", "?|<-", "|->");
	Display();
}

void cMenuFritzbox::DisplayCalls(fritz::CallEntry::eCallType ct) {
	currentMode = (mode) ct;
	std::string title=tr("Fritz!Box call list");
	fritz::CallList *callList = fritz::CallList::GetCallList();
	Clear();
	title += " (";
	switch(ct) {
	case fritz::CallEntry::INCOMING:
		title += tr("incoming");
		break;
	case fritz::CallEntry::MISSED:
		title += tr("missed");
		if (fritzboxConfig.lastKnownMissedCall != callList->getLastMissedCall()) {
			fritzboxConfig.lastKnownMissedCall = callList->getLastMissedCall();
			// save this change as soon as possible, that it is not lost if VDR crashes later on
			plugin->SetupStore("LastKnownMissedCall", fritzboxConfig.lastKnownMissedCall);
			Setup.Save();
		}
		break;
	case fritz::CallEntry::OUTGOING:
		title += tr("outgoing");
		break;
	case fritz::CallEntry::ALL:
		// just a "meta-value", the plugin does not use it
		break;
	}
	title += ")";
	unsigned int destWidth = 0;
	std::string oldDate;
	if (callList->isValid()) {
		for (unsigned int pos=0; pos < callList->getSize(ct); pos++) {
			fritz::CallEntry *ce = callList->retrieveEntry(ct, pos);
			// build the menu entries

			if ( !ce->matchesFilter())
				continue;

			// show remote name, remote number or "unknown"
			std::string sLine = ce->remoteName.length()   > 0 ? ce->remoteName   :
			ce->remoteNumber.length() > 0 ? ce->remoteNumber :
			tr("unknown");
			// determine destWidth
			if (destWidth < sLine.length())
				destWidth = sLine.length();
			// show local number if enabled
			if (fritzboxConfig.showNumberInCallList == true) {
				sLine += "\t" + ce->localNumber;
			}
			sLine = ce->time + "\t" + sLine;
			if (fritzboxConfig.showDaySeparator == false) {
				sLine = ce->date + " " + sLine;
			} else {
				if (ce->date.compare(oldDate) != 0) {
					oldDate = ce->date;
					Add(new cKeyOsdItem(ce->date.c_str(), osUnknown, false, pos));
				}
			}
			Add(new cKeyOsdItem(sLine.c_str(), osUnknown, true, pos));
		}
		// dynamic column layout
		// ugly dirty hack for maybe better column setup,
		// VDR shouldn't set width in chars when using a proportional font :-(
		destWidth++;
		if (fritzboxConfig.showDaySeparator == false)
			SetCols(14, destWidth);
		else
			SetCols(6,  destWidth);
	} else {
		Add(new cOsdItem(tr("The call list is not yet available."),       osUnknown, false));
		Add(new cOsdItem(tr("You may need to wait some minutes,"),        osUnknown, false));
		Add(new cOsdItem(tr("otherwise there may be a network problem."), osUnknown, false));
	}

	SetTitle(title.c_str());
	//TRANSLATORS: this is the short form of "phone book"
	SetHelp(tr("PB"), "|<-", "?|<-", "|->");
	Display();
}

// cMenuCallDetail ************************************************************

cMenuCallDetail::cMenuCallDetail(fritz::CallEntry *ce, cMenuFritzbox::mode mode, fritz::Fonbook *fonbook)
:cOsdMenu(tr("Call details"), 15)
{
	this->ce = ce;

	if (ce->remoteNumber.size() > 0 && ce->remoteName.compare(ce->remoteNumber) == 0) {
		fritz::Fonbook::sResolveResult rr = fonbook->resolveToName(ce->remoteNumber);
		ce->remoteName = rr.name;
		if (cPluginFritzbox::FonbookEntryTypeToName(rr.type).size() > 0) {
			ce->remoteName += " ";
			ce->remoteName += tr(cPluginFritzbox::FonbookEntryTypeToName(rr.type).c_str());
		}
	}

	std::ostringstream text;
	text << tr("Date")          << "\t" << ce->date      << "\n"
	     << tr("Time")          << "\t" << ce->time      << "\n"
	     << tr("Duration")      << "\t"
	     << (mode == cMenuFritzbox::MISSED ? tr("call was not accepted") :
	    	                               ce->duration) << "\n"
	     << tr("Extension")     << "\t" << ce->localName
	     << (ce->localName.size() > 0 ? " (" : "")
	     <<                                ce->localNumber
	     << (ce->localName.size() > 0 ? ")" : "")        << "\n"
	     << (mode == cMenuFritzbox::OUT ?
	        tr("Callee") :
	        tr("Caller"))       << "\t"
	     << ((ce->remoteName.compare(ce->remoteNumber) != 0) ? (ce->remoteName + "\n\t") : "" )
	     << (ce->remoteNumber.size() > 0 ? ce->remoteNumber : tr("unknown")) << "\n";

	//TRANSLATORS: these are labels for color keys in the CallDetails menu
	SetHelp(tr("Button$Call")/*, tr("Button$To PB")*/); // TODO: implement feature
	SetText(text.str());
	Display();
}

void cMenuCallDetail::SetText(std::string text) {
	std::string::size_type pos = 0;
	std::string::size_type npos = text.find('\n', pos);
	do {
		Add(new cOsdItem(text.substr(pos, npos-pos).c_str(), osUnknown, false));
		pos = npos +1;
		npos = text.find('\n', pos);
	} while (npos != std::string::npos);
}

eOSState cMenuCallDetail::ProcessKey (eKeys Key) {
	eOSState state = cOsdMenu::ProcessKey(Key);
	if (state == osUnknown) {
		switch (Key) {
		case kRed:
			// initiate a call
			if (ce->remoteNumber.empty()) {
				Skins.Message(mtError, tr("No number to call"));
			} else {
				fritz::FritzClient fc;
				if (fc.initCall(ce->remoteNumber))
					Skins.Message(mtInfo, tr("Pick up your phone now"));
				else
					Skins.Message(mtError, tr("Error while initiating call"));
			}
			state = osContinue;
			break;
//		case kGreen:
//			// add to active phonebook
//			if (ce->remoteNumber.empty())
//				Skins.Message(mtError, tr("No number to add"));
//			else
//			{
//				fritz::FonbookEntry fe(ce->remoteName);
//				fe.AddNumber(ce->remoteNumber);
//				fritz::FonbookManager::GetFonbookManager()->AddFonbookEntry(fe);
//				Skins.Message(mtInfo, "Added new entry to phone book");
//			}
//			state = osContinue;
//			break;
		case kOk:
			state = osBack;
			break;
		default:
			break;
		}
	}
	return state;
}

// cMenuFonbuchDetail *********************************************************

cMenuFonbuchDetail::cMenuFonbuchDetail(const fritz::FonbookEntry *fe)
:cOsdMenu(tr("Phone book details"), 15)
{
	this->fe = fe;

	std::ostringstream sText;
	// if a number of TYPE_NONE is given, a simple version of the details screen is shown
	// this type is set, e.g., with old Fritz!Boxes
	if (fe->getType(0) == fritz::FonbookEntry::TYPE_NONE) {
	  sText << tr("Name")          << "\t" << fe->getName()      				   << "\n"
	        << tr("Numbers")       << "\t\n"
		    << tr("Default")       << "\t" << fe->getNumber(0) << "\n";
	} else {
	  sText << tr("Name")          << "\t" << fe->getName()      				   << "\n"
	        << tr("Numbers")       << "\t\n";
	  for (size_t pos = 0; pos < fe->getSize(); pos++)
		  sText << cPluginFritzbox::FonbookEntryTypeToName(fe->getType(pos), true) << "\t" << fe->getNumber(pos) << "\n";
	}
	std::string text = sText.str();
	std::string::size_type pos = 0;
	std::string::size_type npos = text.find('\n', pos);
	size_t line = 0;
	do {
		Add(new cOsdItem(text.substr(pos, npos-pos).c_str(), osUnknown, line < 2 ? false : true));
		pos = npos +1;
		npos = text.find('\n', pos);
		line++;
	} while (npos != std::string::npos);
//	if (numbers[fritz::FonbookEntry::TYPE_NONE].length() > 0)
//		SetCurrent(Get(2));
//    else
//	  SetCurrent(Get(1 + fe->getType()));
	//TRANSLATORS: this is the label for the button to initiate a call
	SetHelp(tr("Button$Call"));
	Display();
}

eOSState cMenuFonbuchDetail::ProcessKey (eKeys Key) {
	eOSState state = cOsdMenu::ProcessKey(Key);
	if (state == osUnknown) {
		std::string numberToCall;
		switch (Key) {
		case kRed:
			// determine which number to call
			if (fe->getType(0) == fritz::FonbookEntry::TYPE_NONE){
				numberToCall = fe->getNumber(0);
			}
			else {
				numberToCall = fe->getNumber(Current() - 2);
			}
			// initiate a call
			if (numberToCall.empty()) {
				Skins.Message(mtError, tr("No number to call"));
			} else {
				fritz::FritzClient fc;
				if (fc.initCall(numberToCall))
					Skins.Message(mtInfo, tr("Pick up your phone now"));
				else
					Skins.Message(mtError, tr("Error while initiating call"));
			}
			state = osContinue;
			break;
		case kOk:
			state = osBack;
			break;
		default:
			break;
		}
	}
	return state;
}

// cKeyOsdItem ****************************************************************

cKeyOsdItem::cKeyOsdItem(const char * text, enum eOSState state, bool selectable, unsigned int key)
	:cOsdItem(text, state, selectable) {
	this->key = key;
}
