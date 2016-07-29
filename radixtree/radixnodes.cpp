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

#include "radixnodes.hpp"


using namespace std;


bool merge_sub::scout_location(polynode* location, int max_fill){
/*
	if(small_prefix_node* top = dynamic_cast<small_prefix_node*>(location)){
		bool can_merge = true;
		int total_size = 0;
		for(int j = 0; j<257; j++){
			polynode* ptr = top->slots[j].ptr();
			if(small_prefix_node* n = 
			 dynamic_cast<small_prefix_node*>(ptr)){
				total_size+=n->size;
			}
			else if(dynamic_cast<polydata*>(ptr)){total_size++;}
			else if(ptr==NULL){}
			else{
				// unsupported node
				can_merge = false; break;
			}
		}
		if(total_size < max_fill){can_merge = false;}
		return can_merge;
	}*/
	return false;
}


polynode* merge_sub::build(){
/*
	small_prefix_node* top = dynamic_cast<small_prefix_node*>(prev);
	if(top == NULL){return NULL;}

	mega_prefix_node* new_peak = new mega_prefix_node(top->depth);

	for(int i = 0; i<257; i++){
		polynode* slot_cpy = abort_to_valid(&top->slots[i]);
		if(small_prefix_node* n = 
		 dynamic_cast<small_prefix_node*>(slot_cpy)){
			for(int j = 0; j<257; j++){
				new_peak->slots[i*256+j].init_relaxed(abort_to_valid(&n->slots[j]));
			}
		}
		else if(kv_node* pd = dynamic_cast<kv_node*>(slot_cpy)){
			new_peak->local_insert(pd->k(),pd);
		}
		else if(slot_cpy==NULL){}
		else if(dynamic_cast<removed_node*>(slot_cpy)){}
		else{
			// unsupported node
			// free new_peak
			return NULL;
		}
	}
	
	return new_peak; */return NULL;
}
