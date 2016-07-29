/*
 * Copyright (c) 2014-2015, Hewlett-Packard Development Company, LP.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details. You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * HP designates this particular file as subject to the "Classpath" exception
 * as provided by HP in the LICENSE.txt file that accompanied this code.
 *
 */

#include "polynodes.hpp"
#include <bitset>


using namespace std;

polynode::~polynode(){};
polydata::~polydata(){};

string to_string(const key_or_value* kv){
	if(kv==NULL){return string("NULL");}
	return string(kv->contents(), kv->len());
}


string to_hex(const key_or_value* kv){

	static constexpr char hexchars[] = 
	 "0123456789abcdef";

	if(kv==NULL){return string("NULL");}
	const char* arr = kv->contents();
	size_t len = kv->len();
	string str(len*2,' ');
	for (size_t i = 0; i<len; i++) {
		str[2*i] = hexchars[(arr[i]&0xf0)>>4];
		str[(2*i)+1] = hexchars[arr[i]&0x0f];
	}
	return str;
}

