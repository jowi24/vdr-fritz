/*
 * Fritz!Box plugin for VDR
 *
 * Copyright (C) 2012 Joachim Wilke <vdr@joachim-wilke.de>
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

#include "gtest/gtest.h"

#include <fritzeventhandler.h>
#include <setup.h>

namespace test {

class FritzEventHandler : public ::testing::Test {
protected:
	cFritzEventHandler *feh;

	void SetUp() {
		feh = new cFritzEventHandler();
	}

	void TearDown() {
		delete feh;
	}
};

TEST_F(FritzEventHandler, OneIncomingCallReactOnAny) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_ANY;

	feh->HandleCall(false, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->NotificationDone(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}

TEST_F(FritzEventHandler, OneIncomingCallReactOnIncoming) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_IN;

	feh->HandleCall(false, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->NotificationDone(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}

TEST_F(FritzEventHandler, OneIncomingCallReactOnOutgoing) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_OUT;

	feh->HandleCall(false, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(0, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(0, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}

TEST_F(FritzEventHandler, OneOutgoingCallReactOnAny) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_ANY;

	feh->HandleCall(true, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->NotificationDone(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}

TEST_F(FritzEventHandler, OneOutgoingCallReactOnOutgoing) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_OUT;

	feh->HandleCall(true, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->NotificationDone(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}

TEST_F(FritzEventHandler, OneOutgoingCallReactOnIncoming) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_IN;

	feh->HandleCall(true, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(0, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(0, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}


TEST_F(FritzEventHandler, MultipleCallsReactOnAny) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_ANY;

	feh->HandleCall(true, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleCall(false, 1, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(2, feh->GetConnectionCount());
	feh->HandleCall(true, 3, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(3, feh->GetConnectionCount());
	feh->NotificationDone(0);
	ASSERT_EQ(3, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(2, feh->GetConnectionCount());
	feh->HandleDisconnect(1, "0:01");
	ASSERT_EQ(2, feh->GetConnectionCount());
	feh->NotificationDone(1);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->NotificationDone(3);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(3, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}

TEST_F(FritzEventHandler, MultipleCallsReactOnOutgoing) {
	fritzboxConfig.reactOnDirection = sFritzboxConfig::DIRECTION_OUT;

	feh->HandleCall(true, 0, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleCall(false, 1, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleCall(true, 3, "4711", "", fritz::FonbookEntry::TYPE_HOME, "1", "SIP0", "2&2 Internetz");
	ASSERT_EQ(2, feh->GetConnectionCount());
	feh->NotificationDone(0);
	ASSERT_EQ(2, feh->GetConnectionCount());
	feh->HandleDisconnect(0, "0:01");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(1, "0:01");
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->NotificationDone(3);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleConnect(0);
	ASSERT_EQ(1, feh->GetConnectionCount());
	feh->HandleDisconnect(3, "0:01");
	ASSERT_EQ(0, feh->GetConnectionCount());
}



}


