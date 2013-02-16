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

#ifndef FRITZEVENTHANDLER_H_
#define FRITZEVENTHANDLER_H_

#include <string>
#include <map>
#include <list>
#include <vdr/thread.h>
#include "libfritz++/Listener.h"

class cFritzEventHandler : public fritz::EventHandler {
private:
	bool muted;
	int volumeLevel;
	bool paused;
	bool getCallInfoCalled;
	std::string onCallCmd;

	struct sConnection {
		enum eConnState {
			IDLE,
			RINGING,
			ACTIVE
		} state;
		fritz::sCallInfo *callInfo;
		bool displayed;
	};
	// connId -> sConnection
	std::map<int, sConnection> connections;
	cMutex mutex;
	void Exec(const std::ostream & url) const;
	bool CareForCall(bool outgoing);
	void DoMute();
	void DoUnmute();
public:
	cFritzEventHandler(std::string onCallCmd = "");
	virtual ~cFritzEventHandler();
	size_t GetConnectionCount() { return connections.size(); }
	std::vector<int> GetPendingCallIds();
	fritz::sCallInfo GetCallInfo(int connId);
	void NotificationDone(int connId);
	std::string ComposeCallMessage(int connId);
	virtual void handleCall(bool outgoing, int connId, std::string remoteNumber, std::string remoteName, fritz::FonbookEntry::eType remoteType, std::string localParty, std::string medium, std::string mediumName) override;
	virtual void handleConnect(int connId) override;
	virtual void handleDisconnect(int connId, std::string duration) override;
};

#endif /* FRITZEVENTHANDLER_H_ */
