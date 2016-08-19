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

#ifndef CHROMATICNODES_HPP
#define CHROMATICNODES_HPP

#include <string>
#include <atomic>
#include <string.h>
#include "fptr.hpp"
#include "polynodes.hpp"
#include "polysub.hpp"


class chromatic_node{
public:
	chromatic_node(){}
	virtual int64_t weight() = 0;
	virtual polynode* duplicate(int64_t new_weight) = 0;
};


class chromatic_interior_node : public polyinterior, 
 public chromatic_node{

private:
	int64_t incoming_weight;

public:
	fptr<polynode> slots[2];
	// always followed by:
	// key router;

private:
	chromatic_interior_node(key* router, int64_t incoming_weight){
		if(incoming_weight<0){incoming_weight=0;}
		this->incoming_weight = incoming_weight;
		router->copy(_router());
	}

	inline key_or_value* _router(){
		char* ptr = (char*)this;
		key_or_value* retval = (key_or_value*)(ptr+sizeof(chromatic_interior_node));
		return retval;
	}

public:

	inline key* router() const{
		char* ptr = (char*)this;
		key_or_value* retval = (key_or_value*)(ptr+sizeof(chromatic_interior_node));
		return retval;
	}

	static inline size_t size_of(key* router){
		return sizeof(chromatic_interior_node)+router->size_of();
	}

	inline size_t size_of() const {
		return chromatic_interior_node::size_of(router());
	}

	static inline chromatic_interior_node* alloc(key* router,
	 int64_t incoming_weight){
		size_t sz = chromatic_interior_node::size_of(router);
		void* ptr = malloc(sz);
		memset(ptr,0,sizeof(chromatic_interior_node));
		chromatic_interior_node *o = 
		 new (ptr) chromatic_interior_node(router,incoming_weight);
		return o;
	}

	std::string to_string() const{
		return std::string("chromatic_interior_node < ")+
		 "incoming_weight="+std::to_string(incoming_weight)+" "
		 +"router="+::to_string(router());
	}

	fptr<polynode>* slot(key* k){
		int32_t idx;
		idx = k->cmp(_router());
		if(idx<=0){return &slots[0];}
		else{return &slots[1];}
	}

	polynode* duplicate(int64_t new_weight){
		return chromatic_interior_node::alloc(router(),new_weight);
	}

	int64_t weight(){
		return incoming_weight;
	}

};




class chromatic_kv_node : public polydata, public chromatic_node {

	/* always followed by: */
	/* 
	_key k;
	_value v;
	*/

private:	
	int64_t incoming_weight;

	chromatic_kv_node(int64_t incoming_weight){
		if(incoming_weight<0){incoming_weight=0;}
		this->incoming_weight = incoming_weight;
	}

	inline _key* _k(){
		char* ptr = (char*)this;
		return (_key*)(ptr+sizeof(chromatic_kv_node));
	}

	inline _value* _v(){
		char* ptr = (char*)this;
		return (_value*)(ptr+sizeof(chromatic_kv_node)+_k()->size_of());
	}

public:
	static inline chromatic_kv_node* alloc(const key* k, const value* v, 
	 int64_t incoming_weight){
		void* ptr = malloc(sizeof(chromatic_kv_node)+k->size_of()+v->size_of());
		chromatic_kv_node* node = new (ptr) chromatic_kv_node(incoming_weight); 
		k->copy(node->_k());
		v->copy(node->_v());
		return node;
	}
	inline key* k() const {
		char* ptr = (char*)this;
		return (key*)(ptr+sizeof(chromatic_kv_node));
	}
	inline value* v() const {
		char* ptr = (char*)this;
		return (value*)(ptr+sizeof(chromatic_kv_node)+k()->size_of());
	}
	virtual std::string to_string() const{
		return std::string("chromatic_kv_node <")+::to_string(k())+" : "
		 +::to_string(v())+">";
	}

	inline int64_t weight(){
		return incoming_weight;
	}

	polynode* duplicate(int64_t new_weight){
		return chromatic_kv_node::alloc(k(),v(),new_weight);
	}

};

class delete_sub : public polysub{
public:
	key* k;
	chromatic_kv_node* victim;
	delete_sub(key* k, chromatic_kv_node* victim):k(k),victim(victim){}

	polynode* build();

};



// Rotations come from the original chromatic 
// tree paper: Chromatic binary search trees by Nurmi 
// and Soisalon-Soininen, Figure 2
// http://rd.springer.com/article/10.1007/s002360050057
class rotation_a_d : public polysub{
public:
	const int too_heavy;
	rotation_a_d(const int too_heavy):too_heavy(too_heavy){}
	polynode* build();
};

class rotation_b_e : public polysub{
public:
	bool right_long;
	const int too_heavy;
	rotation_b_e(bool right_long, const int too_heavy):
	 right_long(right_long),too_heavy(too_heavy){}
	polynode* build();
};

class rotation_c : public polysub{
public:
	bool right_long;
	const int too_heavy;
	rotation_c(bool right_long, const int too_heavy):
	 right_long(right_long),too_heavy(too_heavy){}
	polynode* build();
};

#endif