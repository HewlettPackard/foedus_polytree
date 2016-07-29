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

#include "radixtree.hpp"
#include "radixnodes.hpp"
#include <iostream>

using namespace std;

radix_tree::radix_tree(){

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

int inline calculate_depth(simple_vector<fptr_val<polynode>>& path){
	int depth = 0;
	for(int i = 0; i<path.size(); i++){
		fptr_val<polynode> fv = path.get(i);
		polynode* current = fv.val().ptr();
		if(prefix_node* pn = dynamic_cast<prefix_node*>(current)){
			depth+=pn->num_bits;
		}
		else if(dynamic_cast<polydata*>(current)){}
		else if(current==NULL){}
		else{assert(false);}
	}
	return depth;
}




polynode* radix_tree::get_update_node(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts,
	 value*& ans_value_out, bool& callback_out, void* params){
	
	assert(opts.OP==OP_UPDATE);

	polynode* current = get_terminal_node(path);
	polynode* n;

	callback_out = true;

	/* leaf slot isn't occupied */
	if(current == NULL){
		return get_update_node_at_NULL(k, v, opts, ans_value_out);
	}
	/* found a removed node */
	else if(removed_node* pd = dynamic_cast<removed_node*>(current)){
		return get_update_node_at_removed(k, v, pd, opts, ans_value_out);
	}
	/* leaf slot is already occupied*/
	else if(kv_node* pd = dynamic_cast<kv_node*>(current)){
		if(pd->k()->equals(k)){
			/* already existing value for this key */
			return get_update_node_at_equals(k, v, pd, opts, ans_value_out);
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
				int64_t depth = calculate_depth(path);
				assert(depth>=0);
				prefix_node* top = prefix_node::alloc(depth,8);
				prefix_node* node = top;
				while(node->slot(k)==node->slot(pd->k())){	
					/* add nodes until keys differ */
					depth+=node->num_bits;
					prefix_node* down = prefix_node::alloc(depth,8);
					node->local_insert(k,down);
					node->num_children++;
					node=down;
				}
				node->local_insert(k,kv_node::alloc(k, v));
				node->local_insert(pd->k(),kv_node::alloc(pd->k(), pd->v()));
				node->num_children+=2;
				return top;
			}
		}
	}
	else if(polyinterior* pi = dynamic_cast<polyinterior*>(current)){
		/* slot doesn't exist, we need to resize the node*/
		assert(false);
	}
	assert(false);return NULL;
}

static thread_local unsigned long count = 0; 

void radix_tree::on_update_success(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polynode* new_node,
	 polynode* old_node, polyoptions opts){

	//print_UIP(k,new_node,path);

	prefix_node* parent;

	// update parent node size
	parent = (prefix_node*)get_penultimate_node(path);
	if(parent!=NULL){
		kv_node* n = dynamic_cast<kv_node*>(new_node);
		kv_node* o = dynamic_cast<kv_node*>(old_node);
		if(n != NULL && o != NULL && !n->k()->equals(o->k()) ){
			parent->num_children++; 
		}
		else if(old_node == NULL){
			parent->num_children++; 
		}
	}

	count++;

	//if(count%10 != 0){return;}
	return;
/*
	// check for opportunity for merge
	for(int i = 0; i<path.size(); i++){
		fv = path.get(i);
		here = fv.fp();
		here_cpy = fv.val();
		current = here_cpy.ptr();

		// preapprove a location
		if(merge_sub::scout_location(current,1000)){
			puts("merging....");
			polysub* merge = new merge_sub();
			substitute_at(merge,here,path,opts);
		}
		
	}
*/

}

value* radix_tree::get_point_read_value(key* k, 
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
		return read_data_node(k,(polydata*)current);
	}
}

value* radix_tree::get(key* k){
	value* v = point_read(k,get_options);
	return v;
}

value* radix_tree::put(key* k, value* v){
	value* ans;
	update(k,v,put_options,ans,NULL);
	return v;
}

bool radix_tree::insert(key* k, value* v){
	value* ans;
	return update(k,v,insert_options,ans,NULL);
}

value* radix_tree::replace(key* k, value* v){
	value* ans;
	update(k,v,replace_options,ans,NULL);
	return ans;
}

value* radix_tree::remove(key* k){
	value* ans;
	update(k,NULL,remove_options,ans,NULL);
	return ans;
}


