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

#include "hashtrienodes.hpp"
#include <bitset>

using namespace std;


polynode* resize_sub::build(){

	hash_trie_node* top = dynamic_cast<hash_trie_node*>(prev);
	if(top == NULL){assert(false);}

	// create replacement node
	hash_trie_node* new_peak = 
	 hash_trie_node::alloc(top->bit_offset,top->num_bits,
	 top->num_slots+1);

	// copy indexing information
	memcpy(new_peak->_marks(),top->_marks(),new_peak->sz_marks_array);

	// add new node (and mark appropriately)
	new_peak->init_with(k,kv_node::alloc(k,v));

	// note which index we marked
	size_t new_idx = new_peak->slot_index_of(k);

	// copy old node and abort as necessary
	for(size_t i = 0; i<top->num_slots; i++){
		polynode* slot_cpy = abort_to_valid(&top->_slots()[i]);
		if(slot_cpy==NULL){}
		else if(dynamic_cast<removed_node*>(slot_cpy)){}
		else{
			// need to add the node in the correct spot, 
			// but we don't have the key for interior nodes...
			uint8_t gt;
			(i<new_idx)?gt=0:gt=1;
			new_peak->_slots()[i+gt].init(slot_cpy,FP_VALID);
		}
	}
	
	//cout<<"subbing"<<endl;

	return new_peak;
}