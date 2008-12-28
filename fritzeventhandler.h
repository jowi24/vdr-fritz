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

#ifndef FRITZEVENTHANDLER_H_
#define FRITZEVENTHANDLER_H_

#include <string>
#include <list>
#include <Listener.h>

class cFritzEventHandler : public fritz::EventHandler {
private:
	bool muted;
	bool paused;
	std::list<int> connIdList;
	int displayedConnId;
	fritz::sCallInfo *callInfo;


public:
	cFritzEventHandler();
	virtual ~cFritzEventHandler();

	fritz::sCallInfo *GetCallInfo() { return callInfo; }
	std::string ComposeCallMessage();

	virtual void HandleCall(bool outgoing, int connId, std::string remoteNumber, std::string remoteName, std::string remoteType, std::string localParty, std::string medium);
	virtual void HandleConnect(int connId);
	virtual void HandleDisconnect(int connId, std::string duration);
};

#endif /* FRITZEVENTHANDLER_H_ */
