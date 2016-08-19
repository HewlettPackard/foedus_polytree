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

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <time.h>
#include <typeinfo>

#include "polytree.hpp"
#include "radixtree/radixtree.hpp"
#include "hashtrie/hashtrie.hpp"
#include "chromatictree/chromatictree.hpp"
#include "trie/trie.hpp"

using namespace std;

int main (int argv,char** argc){

	static const int SZ_DATA = 20;
	key* keys[SZ_DATA];
	value* values[SZ_DATA];

	for(int i = 0; i<SZ_DATA; i++){
		keys[i]=key::alloc(to_string(i%10)+to_string(i%10)+to_string(i%10)+to_string(i)+"_key");
		values[i]=value::alloc(to_string(i%10)+to_string(i%10)+to_string(i%10)+to_string(i)+"_val");
	}
//	hash_trie* tree = new hash_trie();
//	chromatic_tree* tree = new chromatic_tree();
	trie* tree = new trie();
	value* ans;

	for(int i = 0; i<SZ_DATA; i++){
		cout<<"put'ing : "<<to_string(keys[i])<<endl;
		cout<<"(hex) : "<<to_hex(keys[i])<<endl;
		ans = tree->put(keys[i],values[i]);
		cout<<"put'd : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl<<endl;
	}

	for(int i = 0; i<SZ_DATA; i++){
		cout<<"replace'ing : "<<to_string(keys[i])<<endl;
		cout<<"(hex) : "<<to_hex(keys[i])<<endl;
		ans = tree->replace(keys[i],values[i]);
		cout<<"replace'd : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl<<endl;
	}

	for(int i = 0; i<SZ_DATA/2; i++){
		cout<<"remove'ing : "<<to_string(keys[i])<<endl;
		cout<<"(hex) : "<<to_hex(keys[i])<<endl;
		ans = tree->remove(keys[i]);
		cout<<"remove'd : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl<<endl;
	}

	for(int i = 0; i<SZ_DATA; i++){
		cout<<"get'ing : "<<to_string(keys[i])<<endl;
		cout<<"(hex) : "<<to_hex(keys[i])<<endl;
		ans = tree->get(keys[i]);
		cout<<"get'd : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl<<endl;
	}

	for(int i = 0; i<SZ_DATA; i++){
		cout<<"put'ing : "<<to_string(keys[i])<<endl;
		cout<<"(hex) : "<<to_hex(keys[i])<<endl;
		ans = tree->put(keys[i],values[i]);
		cout<<"put'd : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl<<endl;
	}

	for(int i = 0; i<SZ_DATA; i++){
		cout<<"get'ing : "<<to_string(keys[i])<<endl;
		cout<<"(hex) : "<<to_hex(keys[i])<<endl;
		ans = tree->get(keys[i]);
		cout<<"get'd : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl<<endl;
	}

	return 1;
}






















