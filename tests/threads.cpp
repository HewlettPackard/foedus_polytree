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

static const int SZ_DATA = 500000;
static const int NUM_OPS = 1000000;
static const bool DO_GETS  = true;
static const int NUM_THREADS = 4;
key** keys;
value** values;
//radix_tree* tree;
//hash_trie* tree;
chromatic_tree* tree;
pthread_barrier_t barrier;
time_point<Clock> start;
time_point<Clock> finish;


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
	for (int i = 0; i < len; i++) {
		buf[i] = alphanum[rand()%(sizeof(alphanum)-1)];
	}
}

void gen_random(char* buf, const int len) {
	buf[len] = 0;
	for (int i = 0; i < len; i++) {
		buf[i] = rand() % 256;
	}
}

void* thread_main(void* ptr){
	value* v;
	char buf[200];
	unsigned long r = rand(); 
	pthread_barrier_wait(&barrier); // to link up
	if(ptr==NULL){start = Clock::now();}
	pthread_barrier_wait(&barrier); // wait for master thread to start timer

	for(int j = 0; j<NUM_OPS; j++){
			r = next_rand(r);
			if(DO_GETS){
				v = tree->get(keys[r%SZ_DATA]);
				if(v!=NULL){memcpy(buf,v,v->len());}
			}
			else{
				v = tree->put(keys[r%SZ_DATA],values[r%SZ_DATA]);
			}
	}

	pthread_barrier_wait(&barrier); // to link up
	if(ptr==NULL){finish = Clock::now();}

	return NULL;
}


int main (int argv,char** argc){

	keys = new key*[SZ_DATA];
	values = new value*[SZ_DATA];
	pthread_barrier_init(&barrier,NULL,NUM_THREADS);

	char buf[200];
	buf[190]=0;

	unsigned long r = 1;
	for(int i = 0; i<SZ_DATA; i++){
		r = next_rand(r);
		gen_random(buf,8);
		keys[i]=key::alloc(buf,8);
		values[i]=value::alloc(buf,8);
	}
//	tree = new radix_tree();
//	tree = new hash_trie();
		tree = new chromatic_tree();
	value* ans;
	for(int i = 0; i<SZ_DATA; i++){
		tree->put(keys[i],values[i]);
	}
	std::string op;
	DO_GETS? op = "GETS" : op="PUTS";
	cout<<"inited "<<SZ_DATA<<" with: "<<NUM_THREADS<<" threads for "<<op<<endl;

	pthread_t threads[NUM_THREADS-1];
	int rc;
	long t;
	for(t=0;t<NUM_THREADS-1;t++){
		pthread_create(&threads[t], NULL, thread_main, (void*)(t+1));
	}
	thread_main(NULL);

	for(t=0;t<NUM_THREADS-1;t++){
		pthread_join(threads[t], NULL);
	}


	for(int i = 0; i<SZ_DATA && i<3; i++){
		ans = tree->get(keys[i]);
		cout<<"get : "<<to_hex(keys[i])<<" : "<<to_hex(ans)<<endl;
	}

	milliseconds diff = duration_cast<milliseconds>(finish - start);
	std::cout << diff.count() << "ms" << std::endl;
	std::cout << (NUM_OPS*NUM_THREADS/1000000.0)/(diff.count()/1000.0) <<" Mops/sec"<< std::endl;
	std::cout << (NUM_OPS/1000000.0)/(diff.count()/1000.0) <<" Mops/sec/core"<< std::endl;


	return 1;
}


/*

	time_point<Clock> start = Clock::now();


	time_point<Clock> end = Clock::now();

	for(int i = 0; i<SZ_DATA && i<3; i++){
		ans = tree->get(keys[i]);
		cout<<"get : "<<to_string(keys[i])<<" : "<<to_string(ans)<<endl;
	}

	milliseconds diff = duration_cast<milliseconds>(end - start);
	std::cout << diff.count() << "ms" << std::endl;
	std::cout << (NUM_OPS/1000000)/(diff.count()/(double)1000) <<" Mops/sec"<< std::endl;

*/



















