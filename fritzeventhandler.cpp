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
#include <vdr/device.h>
#include <vdr/remote.h>
#include <vdr/player.h>
#include <vdr/skins.h>
#include <vdr/shutdown.h>
#include <vdr/thread.h>

#include "libfritz++/Fonbook.h"
#include "liblog++/Log.h"
#include "setup.h"
#include "fritzeventhandler.h"
#include "notifyosd.h"

cFritzEventHandler::cFritzEventHandler(std::string onCallCmd) {
	muted = false;
	volumeLevel = 0;
	paused = false;
	getCallInfoCalled = false;
	this->onCallCmd = onCallCmd;
}

cFritzEventHandler::~cFritzEventHandler() {

}

fritz::sCallInfo cFritzEventHandler::GetCallInfo(int connId) {
	getCallInfoCalled = true;
	fritz::sCallInfo callInfo;
	mutex.Lock();
	if (connections.find(connId) != connections.end()) {
		sConnection &connection = connections[connId];
		if (connection.callInfo)
			callInfo = *(connection.callInfo);
	}
	mutex.Unlock();
	return callInfo;
}

// returns a vector of call ids of calls pending for display
std::vector<int> cFritzEventHandler::GetPendingCallIds() {
	std::vector<int> ids;
	mutex.Lock();
	for (std::map<int, sConnection>::iterator it = connections.begin(); it != connections.end(); it++) {
		if ((static_cast<sConnection>((*it).second).displayed == false) ||
			(static_cast<sConnection>((*it).second).state == sConnection::RINGING )) {
			ids.push_back((*it).first);
		}
	}
	mutex.Unlock();
	return ids;
}

void cFritzEventHandler::NotificationDone(int connId) {
	mutex.Lock();
	sConnection &connection = connections[connId];
	connection.displayed = true;
	if (connection.state == sConnection::IDLE) {
		delete connection.callInfo;
		connection.callInfo = NULL;
		connections.erase(connId);
	}
	mutex.Unlock();
}

std::string cFritzEventHandler::ComposeCallMessage(int connId) {
	std::string rMsg;
	int ret;
	// medium gets MSN appended if ISDN is used
	mutex.Lock();
	fritz::sCallInfo *callInfo = connections[connId].callInfo;
	std::string medium = callInfo->medium;
	if (callInfo->medium.find("ISDN") != std::string::npos)
		medium += " " + callInfo->localNumber;
	char *msg;
	// compose the message to be displayed
	if (callInfo->isOutgoing == true) {
		ret = asprintf(&msg, tr("Calling %s [%s]"),
				callInfo->remoteName.c_str(), medium.c_str());
		if (ret <= 0) {
			mutex.Unlock();
			return rMsg;
		}
	} else {
		if (callInfo->remoteNumber.size() == 0) {
			// unknown caller
			ret = asprintf(&msg, "%s [%s]", tr("Call"), medium.c_str());
			if (ret <= 0) {
				mutex.Unlock();
				return rMsg;
			}
		} else {
			// known caller
			ret = asprintf(&msg, "%s %s [%s]", tr("Call from"),
					callInfo->remoteName.c_str(), medium.c_str());
			if (ret <= 0) {
				mutex.Unlock();
				return rMsg;
			}
		}
	}
	mutex.Unlock();
	rMsg = msg;
	free(msg);
	return rMsg;
}

bool cFritzEventHandler::CareForCall(bool outgoing) {
	if (fritzboxConfig.reactOnDirection != fritzboxConfig.DIRECTION_ANY) {
		if (outgoing && fritzboxConfig.reactOnDirection
				!= fritzboxConfig.DIRECTION_OUT)
			return false;
		if (!outgoing && fritzboxConfig.reactOnDirection
				!= fritzboxConfig.DIRECTION_IN)
			return false;
	}
	return true;
}

void cFritzEventHandler::handleCall(bool outgoing, int connId,
		std::string remoteNumber, std::string remoteName,
		fritz::FonbookEntry::eType remoteType, std::string localParty,
		std::string medium, std::string mediumName) {

	if (!CareForCall(outgoing))
		return;

	bool currPlay, currForw;
	int currSpeed;
	cControl *control = cControl::Control();
	if (control) {
		control->GetReplayMode(currPlay, currForw, currSpeed);
	}

	// check for muting
	if (fritzboxConfig.muteOnCall && !fritzboxConfig.muteAfterConnect && !cDevice::PrimaryDevice()->IsMute()) {
		INF((outgoing ? "outgoing": "incoming") << " call, muting.");
		DoMute();
	}
	// check for pausing replay or live tv
	if (fritzboxConfig.pauseOnCall && !paused &&
	    ((control && currPlay) || (fritzboxConfig.pauseLive && !ShutdownHandler.IsUserInactive()))) {
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
		mutex.Lock();
		connections.insert(std::pair<int, sConnection>(connId, connection));
		mutex.Unlock();

		// trigger notification using own osd
		if (fritzboxConfig.useNotifyOsd && !cNotifyOsd::isOpen()) {
			DBG("triggering NotifyOsd");
			cRemote::CallPlugin(fritzboxConfig.pluginName.c_str());
		}
	}

	if (onCallCmd.size())
		Exec(std::stringstream().flush() << onCallCmd << " CALL "
									     << (outgoing ? "OUT " : "IN ")
			                             << connId << " "
			                             << remoteNumber << " \"" << remoteName << "\" "
			                             << localParty << " "
			                             << medium << " \"" << mediumName << "\"");
}

void cFritzEventHandler::handleConnect(int connId) {
	if (connections.find(connId) == connections.end())
		return;
	bool outgoing = connections[connId].callInfo->isOutgoing;
	if (!CareForCall(outgoing))
		return;

	if (fritzboxConfig.muteOnCall && fritzboxConfig.muteAfterConnect && !cDevice::PrimaryDevice()->IsMute()) {
		INF("muting connected call");
		DoMute();
	}

	mutex.Lock();
	sConnection &connection = connections[connId];
	connection.state = sConnection::ACTIVE;
	mutex.Unlock();
	if (onCallCmd.size())
		Exec(std::stringstream().flush() << onCallCmd << " CONNECT " << connId);
}

void cFritzEventHandler::handleDisconnect(int connId, std::string duration) {
	if (connections.find(connId) == connections.end())
		return;
	bool outgoing = connections[connId].callInfo->isOutgoing;
	if (!CareForCall(outgoing))
		return;

	bool currPlay, currForw;
	int currSpeed;
	cControl *control = cControl::Control();
	if (control) {
		control->GetReplayMode(currPlay, currForw, currSpeed);
	}

	// stop call notification
	mutex.Lock();
	sConnection &connection = connections[connId];
	connection.state = sConnection::IDLE;
	if (connection.displayed) {
		if (connection.callInfo) {
			delete connection.callInfo;
			connection.callInfo = NULL;
		}
		// remove current connection from list
		connections.erase(connId);
	}
	bool activeCallsPending = false;
	for (std::map<int, sConnection>::iterator it = connections.begin(); it != connections.end(); it++) {
		if (static_cast<sConnection>((*it).second).state != sConnection::IDLE)
			activeCallsPending = true;
	}
	mutex.Unlock();
	// unmute, if applicable
	if (!activeCallsPending && muted) {
		INF("Finished all calls, unmuting.");
		DoUnmute();
	}
	// resume, if applicable
	if (!activeCallsPending && paused) {
		if (fritzboxConfig.resumeAfterCall && control && currPlay == false) {
			INF("Finished all calls, pressing kPlay.");
			cRemote::Put(kPlay); // this is an ugly workaround, but it should work
			cRemote::Put(kPlay);
		}
		paused = false;
	}

	if (onCallCmd.size()) {
		Exec(std::stringstream().flush() << onCallCmd << " DISCONNECT "
				<< connId << " " << duration);
		if (!activeCallsPending)
			Exec(std::stringstream().flush() << onCallCmd << " FINISHED");
	}
}

void cFritzEventHandler::Exec(const std::ostream & cmd) const {
	const std::stringstream &sCmd = static_cast<const std::stringstream&>(cmd);
	SystemExec(sCmd.str().c_str(), false);
}

void cFritzEventHandler::DoMute() {
	if (fritzboxConfig.muteVolumeLevel < 100) {
		volumeLevel = cDevice::PrimaryDevice()->CurrentVolume();
		cDevice::PrimaryDevice()->SetVolume(volumeLevel * (100 - fritzboxConfig.muteVolumeLevel) / 100, true);
	} else if (!cDevice::PrimaryDevice()->IsMute())
		cDevice::PrimaryDevice()->ToggleMute();
	muted = true;
}

void cFritzEventHandler::DoUnmute() {
	if (fritzboxConfig.muteVolumeLevel < 100) {
		cDevice::PrimaryDevice()->SetVolume(volumeLevel, true);
	} else if (cDevice::PrimaryDevice()->IsMute())
		cDevice::PrimaryDevice()->ToggleMute();
	muted = false;
}
