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
#include "fritztools.h"
#include "setup.h"
#include "tcpclient.h"

cMutex* cFritzTools::fritzBoxMutex = new cMutex();

cFritzTools::cFritzTools()
{
}

cFritzTools::~cFritzTools()
{
}

void cFritzTools::Login() {
	std::ostringstream sMsg;

	dsyslog("fritztools.c: sending login request to fritz.box.");
	cHttpClient tc(fritzboxConfig.url, PORT_WWW);
	tc   <<		"POST /cgi-bin/webcm HTTP/1.1\n"
	     <<  	"Content-Type: application/x-www-form-urlencoded\n"
	     <<  	"Content-Length: "
	     <<  	23 + UrlEncode(fritzboxConfig.password).size()
	     <<  	"\n\nlogin:command/password="
	     <<  	UrlEncode(fritzboxConfig.password)
	     <<  	"\n";
	tc >> sMsg;
	if (sMsg.str().find("FEHLER") != std::string::npos) { // is the Fritz!Box-Webinterface always in German?
		dsyslog("fritztools.c: login failed.");
		throw cToolsException(cToolsException::ERR_LOGIN_FAILED);
	}
	dsyslog("fritztools.c: login successful.");
}

std::string cFritzTools::UrlEncode(std::string &s_input) {
	std::string result;
	std::string s;
	std::string hex = "0123456789abcdef";
	cCharSetConv *conv = new cCharSetConv(cCharSetConv::SystemCharacterTable(), "ISO-8859-15");
	s = conv->Convert(s_input.c_str());
	delete(conv);
	for (unsigned int i=0; i<s.length(); i++) {
		if( 'a' <= s[i] && s[i] <= 'z'
			|| 'A' <= s[i] && s[i] <= 'Z'
			|| '0' <= s[i] && s[i] <= '9' ) {
			result += s[i];
		} else {
			result += '%';
			result += hex[(unsigned char) s[i] >> 4];
			result += hex[(unsigned char) s[i] & 0x0f];
		}
	}
	return result;
}

bool cFritzTools::InitCall(std::string &number) {
	std::string msg;
	try {
		Login();
		dsyslog("fritztools.c: sending fonbuch request.");
		cHttpClient tc(fritzboxConfig.url, PORT_WWW);
		tc  <<	"POST /cgi-bin/webcm HTTP/1.1\n"
			<<	"Content-Type: application/x-www-form-urlencoded\n"
			<<	"Content-Length: "
			<<	107 + number.length()
			<<	"\n\n"
			<<	"getpage=..%2Fhtml%2Fde%2Fmenus%2Fmenu2.html&var%3Apagename=fonbuch&var%3Amenu=home&telcfg%3Acommand%2FDial="
			<<	number
			<<	"\n";
		tc >> msg;
	} catch (cTcpException te) {
		dsyslog("fritztools.c: Exception - %s", te.what());
		return false;
	}
	return true;	
}

std::string cFritzTools::NormalizeNumber(std::string number) {
	GetPhoneSettings();
	// Only for Germany: Remove Call-By-Call Provider Selection Codes 010(0)xx
	if (fritzboxConfig.countryCode == "49") {
		if (number[0] == '0' && number[1] == '1' && number[2] == '0') {
			if (number[3] == '0') 
				number.erase(0, 6);
			else
				number.erase(0, 5);
		}
	}
	// Modifies 'number' to the following format
	// '00' + countryCode + regionCode + phoneNumber
	if (number[0] == '+') {
		//international prefix given in form +49 -> 0049
		number.replace(0, 1, "00");
	} else if (number[0] == '0' && number[1] != '0') {
		//national prefix given 089 -> 004989
		number.replace(0, 1, fritzboxConfig.countryCode.c_str());
		number = "00" + number;
	} else if (number[0] != '0') {
		// number without country or region code, 1234 -> +49891234
		number = "00" + fritzboxConfig.countryCode + fritzboxConfig.regionCode + number; 		
	} // else: number starts with '00', do not change
	return number;
}

int cFritzTools::CompareNormalized(std::string number1, std::string number2) {
	return NormalizeNumber(number1).compare(NormalizeNumber(number2));
}

void cFritzTools::GetPhoneSettings() {
	// if countryCode or regionCode are already set, exit here...
	if (fritzboxConfig.countryCode.size() > 0 || fritzboxConfig.regionCode.size() > 0)
		return;		
	// ...otherwise get settings from Fritz!Box.
	dsyslog("fritztools.c: Looking up Phone Settings...");
	std::string msg;
	try {
		Login();
		cHttpClient hc(fritzboxConfig.url, PORT_WWW);
		hc << "GET /cgi-bin/webcm?getpage=..%2Fhtml%2Fde%2Fmenus%2Fmenu2.html&var%3Alang=de&var%3Apagename=sipoptionen&var%3Amenu=fon HTTP/1.1\n\n";
		hc >> msg;
	} catch (cTcpException te) {
		dsyslog("fritztools.c: cTcpException - %s", te.what());
		return;
	} catch (cToolsException te) {
		dsyslog("fritztools.c: cToolsException - %s", te.what());
		return;
	}
	unsigned int lkzStart = msg.find("telcfg:settings/Location/LKZ");
	if (lkzStart == std::string::npos) {
		esyslog("fritztools.c: Parser error in GetPhoneSettings(). Could not find LKZ.");
		esyslog("fritztools.c: LKZ not set! Assuming 49 (Germany).");
		esyslog("fritztools.c: OKZ not set! Resolving phone numbers may not always work.");
		fritzboxConfig.countryCode = "49";
		return;
	}
	lkzStart += 37;
	unsigned int lkzStop  = msg.find("\"", lkzStart);
	unsigned int okzStart = msg.find("telcfg:settings/Location/OKZ");
	if (okzStart == std::string::npos) {
		esyslog("fritztools.c: Parser error in GetPhoneSettings(). Could not find OKZ.");
		esyslog("fritztools.c: OKZ not set! Resolving phone numbers may not always work.");
		return;
	}
	okzStart += 37;
	unsigned int okzStop = msg.find("\"", okzStart);
	fritzboxConfig.countryCode = msg.substr(lkzStart, lkzStop - lkzStart);
	fritzboxConfig.regionCode  = msg.substr(okzStart, okzStop - okzStart);
	if (fritzboxConfig.countryCode.size() > 0) {
		dsyslog("fritztools.c: Found LKZ %s.", fritzboxConfig.countryCode.c_str());
	} else {
		esyslog("fritztools.c: LKZ not set! Assuming 49 (Germany).");
		fritzboxConfig.countryCode = "49";
	}
	if (fritzboxConfig.regionCode.size() > 0) {
		dsyslog("fritztools.c: Found OKZ %s.", fritzboxConfig.regionCode.c_str());
	} else {
		esyslog("fritztools.c: OKZ not set! Resolving phone numbers may not always work.");
	}
}
