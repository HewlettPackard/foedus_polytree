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

#include <chrono>
#include <thread>
#include <iostream>

#include "polytree.hpp"
#include "radixtree/radixtree.hpp"
#include "hashtrie/hashtrie.hpp"
#include "chromatictree/chromatictree.hpp"

using namespace std;

using Clock = std::chrono::steady_clock;
using std::chrono::time_point;
using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::milliseconds;

using std::this_thread::sleep_for;

unsigned long next_rand(unsigned long last) {
	unsigned long next = last;
	next = next * 110351524567923 + 12345;
	return((unsigned long)(next/65536) % 1524132763);
}

void gen_random_alphanum(char* buf, const int len) {
	static constexpr char alphanum[] =
	 "0123456789"
	 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	 "abcdefghijklmnopqrstuvwxyz";

	buf[len] = 0;
	for (int i = 0; i < len; ++i) {
		buf[i] = alphanum[rand()%(sizeof(alphanum)-1)];
	}
}

int main (int argv,char** argc){

	static const int SZ_DATA = 500000;
	static const int NUM_OPS = 1000000;
	key** keys;
	keys = new key*[SZ_DATA];
	value** values;
	values = new value*[SZ_DATA];
	
	char buf[200];
	buf[190]=0;

	unsigned long r = 1;

	for(int i = 0; i<SZ_DATA; i++){
		r = next_rand(r);
		gen_random_alphanum(buf,8);
		keys[i]=key::alloc(buf,8);
		values[i]=value::alloc(buf,8);
	}
	chromatic_tree* tree = new chromatic_tree();
//	hash_trie* tree = new hash_trie();
	value* ans;

	for(int i = 0; i<SZ_DATA; i++){
		//cout<<"start: "<<to_string(keys[i])<<endl;
		tree->put(keys[i],values[i]);
		if(i%10000==0){cout<<i<<endl;}
		//cout<<"end: "<<to_string(keys[i])<<endl<<endl;
	}
	cout<<"inited"<<endl;

	time_point<Clock> start = Clock::now();
	for(int j = 0; j<NUM_OPS; j++){
		int i = j%SZ_DATA;
		//tree->put(keys[i],values[i]);
		//if(i%5==0){tree->put(keys[i],values[i]);}
		//else{tree->get(keys[i]);}
		tree->get(keys[i]);
	}

	time_point<Clock> end = Clock::now();

	for(int i = 0; i<SZ_DATA && i<3; i++){
		ans = tree->get(keys[i]);
		cout<<"get : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl;
	}

	milliseconds diff = duration_cast<milliseconds>(end - start);
	std::cout << diff.count() << "ms" << std::endl;
	std::cout << (NUM_OPS/1000000.0)/(diff.count()/(double)1000) <<" Mops/sec"<< std::endl;


	return 1;
}






















