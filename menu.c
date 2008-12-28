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

#include <sstream>
#include <vector>
#include <vdr/menu.h>
#include <vdr/status.h>
#include "FonbookManager.h"
#include "Tools.h"
#include "menu.h"
#include "setup.h"

#ifdef NOT_DEFINED
// some translations of strings provided by libfritz++
//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "home"
trNOOP("H");
//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "mobile"
trNOOP("M");
//TRANSLATORS: telephonebook number type: this is a one char abbreviation for "work"
trNOOP("W");

trNOOP("Local phone book");
trNOOP("Fritz!Box phone book");
trNOOP("nummerzoeker.com");
trNOOP("das-oertliche.de");
#endif

cMenuFritzbox::cMenuFritzbox()
:cOsdMenu("Fritz!Box", 1) // just dummy values
{
	DisplayFonbuch();
}

cMenuFritzbox::~cMenuFritzbox()
{
}

eOSState cMenuFritzbox::ProcessKey (eKeys Key) {
	fritz::Fonbook  *fonbook  = fritz::FonbookManager::GetFonbook();
	fritz::CallList *callList = fritz::CallList::getCallList();

	eOSState state = cOsdMenu::ProcessKey(Key);
	fritz::CallEntry *ce = NULL;
	cKeyOsdItem* currentKeyItem = (cKeyOsdItem*) this->Get(Current());
	if (state == osUnknown) {
		switch (Key) {
		case kOk:
			switch (currentMode) {
			case FONBUCH:
				if (Current() >= 0 && fonbook->isDisplayable() && fonbook->isInitialized())
					state = AddSubMenu(new cMenuFonbuchDetail(fonbook->RetrieveFonbookEntry(Current()), fonbook));
				break;
			case IN:
				if (currentKeyItem)
					ce = callList->RetrieveEntry(fritz::CallEntry::INCOMING, currentKeyItem->key);
				if (ce)
					state = AddSubMenu(new cMenuCallDetail(ce, currentMode, fonbook));
				break;
			case OUT:
				if (currentKeyItem)
					ce = callList->RetrieveEntry(fritz::CallEntry::OUTGOING, currentKeyItem->key);
				if (ce)
					state = AddSubMenu(new cMenuCallDetail(ce, currentMode, fonbook));
				break;
			case MISSED:
				if (currentKeyItem)
					ce = callList->RetrieveEntry(fritz::CallEntry::MISSED, currentKeyItem->key);
				if (ce)
					state = AddSubMenu(new cMenuCallDetail(ce, currentMode, fonbook));
				break;
			}
			break;
		case kRed:
			if (currentMode == FONBUCH) {
				fritz::FonbookManager::GetFonbookManager()->NextFonbook();
			}
			DisplayFonbuch();
			state = osContinue;
			break;
		case kGreen:
		case kYellow:
		case kBlue:
			if (callList->isValid()) {
				DisplayCalls((fritz::CallEntry::callType)(Key - kRed));
			} else {
				Skins.Message(mtError, tr("Error fetching the call list"));
			}
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
	SetTitle(tr(fonbook->GetTitle().c_str()));
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
		std::string lastName;
		for (size_t pos=0; pos < fonbook->GetFonbookSize(); pos++) {
			fritz::FonbookEntry *fe = fonbook->RetrieveFonbookEntry(pos);
			if (fe) {
				// build the menu entries
				char *line;
				int ret = asprintf(&line,"%s\t%s\t%s", lastName == fe->getName() ? "" : fe->getName().c_str(), tr(fe->getTypeName().c_str()), fe->getNumber().c_str());
				if (ret <= 0) {
					*elog << __FILE__ << ": Error allocating line buffer for cOsdItem." << std::endl;
					continue;
				}
				if (fe->getName().length() > nameWidth)
					nameWidth = fe->getName().length();
				Add(new cOsdItem(line));
				lastName = fe->getName();
			}
		}
	}
	SetCols(nameWidth+1, 2);
	SetHelp(tr("> PB"), "|<-", "?|<-", "|->");
	Display();
}

void cMenuFritzbox::DisplayCalls(fritz::CallEntry::callType ct) {
	currentMode = (mode) ct;
	std::string title=tr("Fritz!Box call list");
	fritz::CallList *callList = fritz::CallList::getCallList();
	Clear();
	title += " (";
	switch(ct) {
	case fritz::CallEntry::INCOMING:
		title += tr("incoming");
		break;
	case fritz::CallEntry::MISSED:
		title += tr("missed");
		fritzboxConfig.lastKnownMissedCall = callList->LastMissedCall();
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
	for (unsigned int pos=0; pos < callList->GetSize(ct); pos++) {
		fritz::CallEntry *ce = callList->RetrieveEntry(ct, pos);
		// build the menu entries

		if ( !ce->MatchesFilter())
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

	std::string name = "";
	fritz::FonbookEntry fe(name, ce->remoteNumber);
	fe = fonbook->ResolveToName(fe);

	if (ce->remoteName.size() == 0 && ce->remoteNumber.size() > 0) {
		if (fe.getName().compare(ce->remoteNumber) != 0) {
			ce->remoteName = fe.getName();
			if (fe.getTypeName().size() > 0) {
				ce->remoteName += " ";
				ce->remoteName += tr(fe.getTypeName().c_str());
			}
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
	        tr("Caller"))       << "\t" << ce->remoteName
	     << (ce->remoteName.size() > 0 ? "\n\t" : "")
	     << (ce->remoteNumber.size() > 0 ? ce->remoteNumber :
	                                      tr("unknown")) << "\n";

	//TRANSLATORS: this is the label for the button to initiate a call
	SetHelp(tr("Button$Call"));
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
				if (fritz::Tools::InitCall(ce->remoteNumber))
					Skins.Message(mtInfo, tr("Pick up your phone now"));
				else
					Skins.Message(mtError, tr("Error while initiating call"));
			}
			state = osContinue;
			break;
		default:
			break;
		}
	}
	return state;
}

// cMenuFonbuchDetail *********************************************************

cMenuFonbuchDetail::cMenuFonbuchDetail(fritz::FonbookEntry *fe, fritz::Fonbook *fonbook)
:cOsdMenu(tr("Phone book details"), 15)
{
//  search for other entries with same name but different type
	for(size_t i = 0; i < fonbook->GetFonbookSize(); i++) {
		fritz::FonbookEntry  *fe2 = fonbook->RetrieveFonbookEntry(i);
		if (fe2->getName() == fe->getName())
			numbers[fe2->getType()] = fe2->getNumber();
	}

	std::ostringstream sText;
	// if a number of TYPE_NONE is given, a simple version of the details screen is shown
	// this type is set, e.g., with old Fritz!Boxes
	if (numbers[fritz::FonbookEntry::TYPE_NONE].length() > 0) {
	  sText << tr("Name")          << "\t" << fe->getName()      				   << "\n"
	        << tr("Numbers")       << "\t\n"
		    << tr("Default")       << "\t" << numbers[fritz::FonbookEntry::TYPE_NONE]    << "\n";
	  numbers[fritz::FonbookEntry::TYPE_HOME] = numbers[fritz::FonbookEntry::TYPE_NONE];

	} else {
	  sText << tr("Name")          << "\t" << fe->getName()      				   << "\n"
	        << tr("Numbers")       << "\t\n"
		    << tr("Private")       << "\t" << numbers[fritz::FonbookEntry::TYPE_HOME]    << "\n"
		    << tr("Mobile")        << "\t" << numbers[fritz::FonbookEntry::TYPE_MOBILE]  << "\n"
		    << tr("Business")      << "\t" << numbers[fritz::FonbookEntry::TYPE_WORK]    << "\n";
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
	if (numbers[fritz::FonbookEntry::TYPE_NONE].length() > 0)
		SetCurrent(Get(2));
    else
	  SetCurrent(Get(1 + fe->getType()));
	//TRANSLATORS: this is the label for the button to initiate a call
	SetHelp(tr("Button$Call"));
	Display();
}

eOSState cMenuFonbuchDetail::ProcessKey (eKeys Key) {
	eOSState state = cOsdMenu::ProcessKey(Key);
	if (state == osUnknown) {
		switch (Key) {
		case kRed:
			// initiate a call
			if (numbers[Current() - 1].empty()) {
				Skins.Message(mtError, tr("No number to call"));
			} else {
				if (fritz::Tools::InitCall(numbers[Current() - 1]))
					Skins.Message(mtInfo, tr("Pick up your phone now"));
				else
					Skins.Message(mtError, tr("Error while initiating call"));
			}
			state = osContinue;
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
