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

/*
polynodes.hpp

This header file defines several node types that are supported in the polytree.
The critical abstract types are:

key : the key type used in the polytree.  Supports arbitrary length keys.

polynode : generic abstract node type.  All nodes in a polytree are a subclass
of polynode.

polydata : generic abstract data node type.  These polydata objects cannot be 
interior nodes, they must be leaf nodes.  The existance of polydata objects 
enables certain in-place-update optimizations which cannot be used on 
interior nodes in general.

polyinterior : generic abstract non-leaf type.  These polyinterior objects 
can have an arbitrary number of children and will have a single parent.  A 
polyinterior object CAN store data, however, such data cannot be updated 
using a fast in-place-update operation, but must be changed using a slower 
substitution operation.  

A polyinterior is abstracted as a set number of slots in an array, each with 
a pointer to a child subtree.  The node should know which, if any, child 
subtree a given key is in, and the pointer to said subtree should be in a 
fixed location (this location is the returned value of the slot(key k) 
method).  If the node wants to change which slot ANY key will map to, the 
polyinterior MUST be substituted out and replaced with a new copy.

The remainder of the classes in this header file describe various subclasses of
the above abstract types.  Briefly:

kv_node : polydata which stores both a key and value 
val_node : polydata which only stores the value (assumes the tree structure 
contains the key) 
append_node : polydata which attaches appended changes to another polydata

*/

#ifndef POLYNODES_HPP
#define POLYNODES_HPP

#include <string>
#include <string.h>
#include "fptr.hpp"

class polynode{
public:
	polynode* prev = NULL;
	virtual ~polynode() = 0;

	virtual std::string to_string() const = 0;
};

class polydata : public polynode {
public:
	virtual std::string to_string() const{
		return std::string("polydata");
	}
	virtual ~polydata() = 0;
};

class key_or_value {
private:
	size_t length;
	/* always followed by: */
	/*
	char[] contents;
	*/

	key_or_value(size_t length) : length(length){}

	inline char* _contents(){
		char* ptr = (char*)this;
		char* contents_ptr = ptr+sizeof(key_or_value);
		return contents_ptr;
	}

public:
	static inline key_or_value* alloc(const std::string s){
		size_t len = s.length();
		void* ptr = malloc(sizeof(key_or_value)+len);
		key_or_value *o = new (ptr) key_or_value(len); 
		s.copy(o->_contents(),len,0);
		return o;
	}
	inline void copy(key_or_value* to) const{
		memcpy(to,this,this->size_of());
	}

	inline bool equals(const key_or_value* x) const{
		if(x->size_of()!=size_of()){return false;}
		return memcmp((char*)contents(),(char*)x->contents(),len())==0;
	}

	static inline key_or_value* alloc(char* str, int n){
		size_t len = n;
		void* ptr = malloc(sizeof(key_or_value)+len);
		key_or_value *o = new (ptr) key_or_value(len); 
		memcpy(o->_contents(),str,len);
		return o;
	}

	static inline key_or_value* init_at(void* buf, size_t buf_sz, char* str, int str_sz){
		size_t len = str_sz;
		if(sizeof(key_or_value)+len<buf_sz){return NULL;}
		void* ptr = buf;
		key_or_value *o = new (ptr) key_or_value(len); 
		memcpy(o->_contents(),str,len);
		return o;
	}

	inline size_t size_of() const {return sizeof(key_or_value)+length;}
	inline size_t len() const {return length;}
	inline const char* contents() const {
		char* ptr = (char*)this;
		char* contents_ptr = ptr+sizeof(key_or_value);
		return contents_ptr;
	}

};

typedef key_or_value _key;
typedef key_or_value _value;

typedef const key_or_value key;
typedef const key_or_value value;

std::string to_string(const key_or_value* k);
std::string to_hex(const key_or_value* k);


class kv_node : public polydata {

	/* always followed by: */
	/* 
	_key k;
	_value v;
	*/

private:
	kv_node(){}

	inline _key* _k(){
		char* ptr = (char*)this;
		return (_key*)(ptr+sizeof(kv_node));
	}

	inline _value* _v(){
		char* ptr = (char*)this;
		return (_value*)(ptr+sizeof(kv_node)+_k()->size_of());
	}

public:
	static inline kv_node* alloc(const key* k, const value* v){
		void* ptr = malloc(sizeof(kv_node)+k->size_of()+v->size_of());
		kv_node* node = new (ptr) kv_node(); 
		k->copy(node->_k());
		v->copy(node->_v());
		return node;
	}
	inline key* k() const {
		char* ptr = (char*)this;
		return (key*)(ptr+sizeof(kv_node));
	}
	inline value* v() const {
		char* ptr = (char*)this;
		return (value*)(ptr+sizeof(kv_node)+k()->size_of());
	}
	virtual std::string to_string() const{
		return std::string("kv_node <")+::to_string(k())+" : "+::to_string(v())+">";
	}

};

class removed_node : public polydata {
	/* always followed by: */
	/* 
	key k;
	*/

private:
	removed_node(){}

	inline _key* _k(){
		char* ptr = (char*)this;
		return (_key*)(ptr+sizeof(removed_node));
	}

public:
	static inline removed_node* alloc(key* k){
		void* ptr = malloc(sizeof(removed_node)+k->size_of());
		removed_node* node = new (ptr) removed_node(); 
		k->copy(node->_k());
		return node;
	}
	inline key* k() const{
		char* ptr = (char*)this;
		return (key*)(ptr+sizeof(kv_node));
	}

	virtual std::string to_string() const{
		return std::string("removed_node <")+::to_string(k())+">";
	}
};

class val_node : public polydata {
public:
	size_t len;
	char vcontents[];
};

class append_node : public polydata {
public:
	size_t alen;
	char acontents[];
};

#define DNE ((void*)0x1)

class polyinterior : public polynode{
public:
	virtual fptr<polynode>* slot(key* k) = 0;

	void inline local_insert(key* k, polynode* node){
		fptr<polynode>* here = slot(k);
		assert(here!=NULL);
		assert(here!=DNE);
		here->init(node,FP_VALID);
	}

	virtual std::string to_string() const{
		return std::string("polyinterior");
	}
};


#endif
















