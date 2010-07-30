/*
 * libfritz++
 *
 * Copyright (C) 2007-2010 Joachim Wilke <libfritz@joachim-wilke.de>
 * TelLocalChFonbook created by Christian Richter <cr@crichter.net>
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


#include <unistd.h>
#include <TcpClient++.h>
#include "TelLocalChFonbook.h"
#include "Config.h"
#include "Tools.h"

namespace fritz{

TelLocalChFonbook::TelLocalChFonbook()
{
	title = I18N_NOOP("tel.local.ch");
	techId = "LOCCH";
	displayable = false;
}

TelLocalChFonbook::~TelLocalChFonbook()
{
}

bool TelLocalChFonbook::Initialize() {
	setInitialized(true);
	return true;
}

TelLocalChFonbook::sResolveResult TelLocalChFonbook::ResolveToName(std::string number) {
	TelLocalChFonbook::sResolveResult result;
	result.name = number;
	result.type = FonbookEntry::TYPE_NONE;

	// resolve only (swiss) phone numbers
	if (number.length() == 0 || Tools::NormalizeNumber(number).find("0041") != 0)
		return result;
	
	std::string msg;
	std::string name;
	try {
		DBG("sending reverse lookup request for " << Tools::NormalizeNumber(number) << " to tel.local.ch");
		std::string host = "tel.local.ch";
		tcpclient::HttpClient tc(host);
		tc << tcpclient::get
		   << "/de/q/" <<  Tools::NormalizeNumber(number)
		   << ".html"
		   << "\nAccept-Charset: ISO-8859-1\nUser-Agent: Lynx/2.8.5"
		   << std::flush;
		tc >> msg;
	} catch (tcpclient::TcpException te) {
		ERR("Exception - " << te.what());
		return result;
	}
	// parse answer
	size_t start = msg.find("<h2");

	if (start == std::string::npos) {
		INF("no entry found.");
		return result;
	}
	// add the length of search pattern
	start += 15;

	size_t stop  = msg.find("</h2>", start);
	name = msg.substr(start, stop - start);
	
	// convert the string from latin1 to current system character table
	CharSetConv *conv = new CharSetConv("ISO-8859-1", CharSetConv::SystemCharacterTable());
	const char *s_converted = conv->Convert(name.c_str());
	name = s_converted;
	delete (conv);
	INF("resolves to " << name.c_str());
	result.name = name;
	return result;
}

}
