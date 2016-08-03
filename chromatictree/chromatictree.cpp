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

#include "chromatictree.hpp"
#include "chromaticnodes.hpp"
#include <iostream>

using namespace std;

chromatic_tree::chromatic_tree(){

	default_options.all_flags = 0;
	default_options.all_flags = default_options.all_flags | 
	 always_help_clean_abort();
	default_options.all_flags = default_options.all_flags | 
	 always_help_resolve_pending();
	default_options.MAX_DEPTH = 1;  // check the parent, but that's it


	get_options.all_flags = default_options.all_flags;
	get_options.FUNC = FUNC_GET;
	get_options.OP = OP_READ;

	put_options.all_flags = default_options.all_flags;
	put_options.FUNC = FUNC_PUT;
	put_options.OP = OP_UPDATE;

	insert_options.all_flags = default_options.all_flags;
	insert_options.FUNC = FUNC_INSERT;
	insert_options.OP = OP_UPDATE;

	replace_options.all_flags = default_options.all_flags;
	replace_options.FUNC = FUNC_REPLACE;
	replace_options.OP = OP_UPDATE;

	remove_options.all_flags = default_options.all_flags;
	remove_options.FUNC = FUNC_REMOVE;
	remove_options.OP = OP_UPDATE;

}


polynode* chromatic_tree::get_update_node(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts,
	 value*& ans_value_out, bool& callback_out, void* params){
	
	assert(opts.OP==OP_UPDATE);

	polynode* current = get_terminal_node(path);
	polynode* n;

	callback_out = true; // TODO: after debugging, set this to false

	/* leaf slot isn't occupied */
	if(current == NULL){
		// only ever happens with root
		ans_value_out = NULL;
		switch(opts.FUNC){
		case FUNC_PUT:
		case FUNC_INSERT:
			return chromatic_kv_node::alloc(k, v,1);
		case FUNC_REPLACE:
		case FUNC_REMOVE:
			return NULL;
		}assert(false);
	}
	/* leaf slot is already occupied*/
	else if(chromatic_kv_node* pd = 
	 dynamic_cast<chromatic_kv_node*>(current)){
		if(pd->k()->equals(k)){
			/* already existing value for this key */
			ans_value_out = pd->v();
			switch(opts.FUNC){
			case FUNC_PUT:
			case FUNC_REPLACE:
				return chromatic_kv_node::alloc(k, v,pd->weight());
			case FUNC_INSERT:
				return NULL;
			case FUNC_REMOVE:
				// do a delete substitution
				delete_sub* sub = new delete_sub(k,pd);
				path.pop_back(); // adjust path so we target victim node's parent
				return sub;
			}assert(false);
		}
		else{
			/* key conflict on this slot */
			ans_value_out = NULL;
			switch(opts.FUNC){
			case FUNC_REPLACE:
			case FUNC_REMOVE:
				return NULL;		
			case FUNC_PUT:
			case FUNC_INSERT:	
				/* sprouting insert */
				int64_t new_weight = pd->weight();
				new_weight = (new_weight==0)?0:new_weight-1;
				key* router;
				int cmp = k->cmp(pd->k());
				chromatic_interior_node* top;
				if(cmp<0){
					router = k;
					top = 
					 chromatic_interior_node::alloc(router,new_weight);
					top->local_insert_at(&top->slots[0],
					 chromatic_kv_node::alloc(k,v,1));
					top->local_insert_at(&top->slots[1],
					 chromatic_kv_node::alloc(pd->k(), pd->v(),1));
				}
				else{
					router = pd->k();
					top = 
					 chromatic_interior_node::alloc(router,new_weight);
					top->local_insert_at(&top->slots[1],
					 chromatic_kv_node::alloc(k,v,1));
					top->local_insert_at(&top->slots[0],
					 chromatic_kv_node::alloc(pd->k(), pd->v(),1));
				}
				assert(top->slots[0].ptr()!=NULL);
				assert(top->slots[1].ptr()!=NULL);
				return top;
			}
		}
	}
	else if(polyinterior* pi = dynamic_cast<polyinterior*>(current)){
		/* should never happen */
		assert(false);
	}
	assert(false);return NULL;
}


void chromatic_tree::on_update_success(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polynode* new_node,
	 polynode* old_node, polyoptions opts){

	//print_UIP(k,new_node,path);
	return;
}

value* chromatic_tree::get_point_read_value(key* k, 
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

	assert(opts.OP==OP_READ);
	polynode* current = get_terminal_node(path);

	if(polyinterior* pi = dynamic_cast<polyinterior*>(current)){
		assert(false);
	}
	else if(polysub* pi = dynamic_cast<polysub*>(current)){
		assert(false);
	}
	else{
		if(chromatic_kv_node* pd = 
		 dynamic_cast<chromatic_kv_node*>(current)){
			if(pd->k()->equals(k)){return pd->v();}
			else{return NULL;}
		}
		else if(dynamic_cast<removed_node*>(current)){
			return NULL;
		}
		else if(current == NULL){return NULL;}
		assert(false);return NULL;
	}
}

value* chromatic_tree::get(key* k){
	value* v = point_read(k,get_options);
	return v;
}

value* chromatic_tree::put(key* k, value* v){
	value* ans;
	update(k,v,put_options,ans,NULL);
	return v;
}

bool chromatic_tree::insert(key* k, value* v){
	value* ans;
	return update(k,v,insert_options,ans,NULL);
}

value* chromatic_tree::replace(key* k, value* v){
	value* ans;
	update(k,v,replace_options,ans,NULL);
	return ans;
}

value* chromatic_tree::remove(key* k){
	value* ans;
	update(k,NULL,remove_options,ans,NULL);
	return ans;
}


