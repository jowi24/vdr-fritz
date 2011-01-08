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
#include <vdr/device.h>
#include <vdr/remote.h>
#include <vdr/player.h>
#include <vdr/skins.h>

#include <Fonbook.h>
#include "setup.h"
#include "fritzeventhandler.h"
#include "notifyosd.h"

cFritzEventHandler::cFritzEventHandler() {
	muted = false;
	paused = false;
	getCallInfoCalled = false;
}

cFritzEventHandler::~cFritzEventHandler() {

}

fritz::sCallInfo *cFritzEventHandler::GetCallInfo(int connId) {
	getCallInfoCalled = true;
	sConnection &connection = connections[connId];
	return connection.callInfo;
}

// returns a vector of call ids of calls pending for display
std::vector<int> cFritzEventHandler::GetPendingCallIds() {
	std::vector<int> ids;
	for (std::map<int, sConnection>::iterator it = connections.begin(); it != connections.end(); it++) {
		if (static_cast<sConnection>((*it).second).callInfo) {
			ids.push_back((*it).first);
		}
	}
	return ids;
}

void cFritzEventHandler::NotificationDone(int connId) {
	sConnection &connection = connections[connId];
	connection.displayed = true;
	if (connection.state == sConnection::IDLE) {
		delete connection.callInfo;
		connection.callInfo = NULL;
		connections.erase(connId);
	}
}

std::string cFritzEventHandler::ComposeCallMessage(int connId) {
	std::string rMsg;
	int ret;
	// medium gets MSN appended if ISDN is used
	fritz::sCallInfo *callInfo = connections[connId].callInfo;
	std::string medium = callInfo->medium;
	if (callInfo->medium.find("ISDN") != std::string::npos)
		medium += " " + callInfo->localNumber;
	char *msg;
	// compose the message to be displayed
	if (callInfo->isOutgoing == true) {
		ret = asprintf(&msg, tr("Calling %s [%s]"),
				callInfo->remoteName.c_str(), medium.c_str());
		if (ret <= 0)
			return rMsg;
	} else {
		if (callInfo->remoteNumber.size() == 0) {
			// unknown caller
			ret = asprintf(&msg, "%s [%s]", tr("Call"), medium.c_str());
			if (ret <= 0)
				return rMsg;
		} else {
			// known caller
			ret = asprintf(&msg, "%s %s [%s]", tr("Call from"),
					callInfo->remoteName.c_str(), medium.c_str());
			if (ret <= 0)
				return rMsg;
		}
	}
	rMsg = msg;
	free(msg);
	return rMsg;
}

void cFritzEventHandler::HandleCall(bool outgoing, int connId,
		std::string remoteNumber, std::string remoteName,
		fritz::FonbookEntry::eType remoteType, std::string localParty,
		std::string medium, std::string mediumName) {

	if (fritzboxConfig.reactOnDirection != fritzboxConfig.DIRECTION_ANY) {
		if (outgoing && fritzboxConfig.reactOnDirection
				!= fritzboxConfig.DIRECTION_OUT)
			return;
		if (!outgoing && fritzboxConfig.reactOnDirection
				!= fritzboxConfig.DIRECTION_IN)
			return;
	}

	bool currPlay, currForw;
	int currSpeed;
	cControl *control = cControl::Control();
	if (control) {
		control->GetReplayMode(currPlay, currForw, currSpeed);
	}

//	connIdList.push_back(connId);
	if (fritzboxConfig.muteOnCall && !cDevice::PrimaryDevice()->IsMute()) {
		INF((outgoing ? "outgoing": "incoming") << " call, muting.");
		cDevice::PrimaryDevice()->ToggleMute();
		muted = true;
	}
	if (fritzboxConfig.pauseOnCall && !paused && control && currPlay) {
		INF((outgoing ? "outgoing": "incoming") << " call, pressing kPause.");
		cRemote::Put(kPause);
		paused = true;
	}
	if (medium.compare(mediumName) == 0) {
		if (mediumName.find("SIP") != std::string::npos)
			mediumName.replace(0, 3, "VoIP ");
		if (mediumName.find("POTS") != std::string::npos)
			mediumName = tr("POTS");
	}
	if (fritzboxConfig.showNumber) {
		// save the message into "message", MainThreadHook or MainMenuAction will take care of it
//		displayedConnId = connId;
#ifdef DO_NOT_SET
		// trigger translation of string coming from the Fritz!Box - do not compile!
		trNOOP("ISDN")
		trNOOP("VoIP")
#endif

		fritz::sCallInfo *callInfo = new fritz::sCallInfo();
		callInfo->isOutgoing = outgoing;
		callInfo->remoteNumber = remoteNumber;
		callInfo->remoteName = remoteName;
		if (cPluginFritzbox::FonbookEntryTypeToName(remoteType).size() > 0) {
			callInfo->remoteName += " ";
			callInfo->remoteName += cPluginFritzbox::FonbookEntryTypeToName(
					remoteType);
		}
		callInfo->localNumber = localParty;
		callInfo->medium = mediumName;

		sConnection connection;
		connection.displayed = false;
		connection.state = sConnection::RINGING;
		connection.callInfo = callInfo;
		connections.insert(std::pair<int, sConnection>(connId, connection));

		// trigger notification using own osd
		if (fritzboxConfig.useNotifyOsd && !cNotifyOsd::isOpen()) {
			DBG("triggering NotifyOsd");
			cRemote::CallPlugin(fritzboxConfig.pluginName.c_str());
		}
	}
}

void cFritzEventHandler::HandleConnect(int connId) {
	sConnection &connection = connections[connId];
	connection.state = sConnection::ACTIVE;
	if (connection.displayed) {
		delete connection.callInfo;
		connection.callInfo = NULL;
	}
}

void cFritzEventHandler::HandleDisconnect(int connId, std::string duration) {
	bool currPlay, currForw;
	int currSpeed;
	cControl *control = cControl::Control();
	if (control) {
		control->GetReplayMode(currPlay, currForw, currSpeed);
	}

	// stop call notification
	sConnection &connection = connections[connId];
	connection.state = sConnection::IDLE;
	if (connection.displayed) {
		delete connection.callInfo;
		connection.callInfo = NULL;
		// remove current connection from list
		connections.erase(connId);
	}
	bool activeCallsPending = false;
	for (std::map<int, sConnection>::iterator it = connections.begin(); it != connections.end(); it++) {
		if (static_cast<sConnection>((*it).second).state != sConnection::IDLE)
			activeCallsPending = true;
	}
	// unmute, if applicable
	if (!activeCallsPending && muted && cDevice::PrimaryDevice()->IsMute()) {
		INF("Finished all incoming calls, unmuting.");
		cDevice::PrimaryDevice()->ToggleMute();
		muted = false;
	}
	// resume, if applicable
	if (!activeCallsPending && paused && control && currPlay == false) {
		if (fritzboxConfig.resumeAfterCall) {
			INF("Finished all incoming calls, pressing kPlay.");
			cRemote::Put(kPlay); // this is an ugly workaround, but it should work
			cRemote::Put(kPlay);
		}
		paused = false;
	}
}
