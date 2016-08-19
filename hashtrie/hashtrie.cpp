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

#include "hashtrie.hpp"
#include "hashtrienodes.hpp"
#include <iostream>

using namespace std;

hash_trie::hash_trie(){

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

static int inline calculate_depth(simple_vector<fptr_val<polynode>>& path){
	int depth = 0;
	for(int i = 0; i<path.size(); i++){
		fptr_val<polynode> fv = path.get(i);
		polynode* current = fv.val().ptr();
		if(hash_trie_node* pn = dynamic_cast<hash_trie_node*>(current)){
			depth+=pn->num_bits;
		}
		else if(dynamic_cast<polydata*>(current)){}
		else if(current==NULL){}
		else{assert(false);}
	}
	return depth;
}

polynode* hash_trie::get_update_node(key* k, value* v,
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
				int64_t num_bits = 9;
				hash_trie_node* top = NULL;
				hash_trie_node* node = top;
				hash_trie_node* down;
				assert(depth>=0);
				/* add nodes until keys differ */
				if(key_to_subbits(k,depth,num_bits) != 
				 key_to_subbits(pd->k(),depth,num_bits)){
					top = hash_trie_node::alloc(depth,num_bits,2);
					node = top;
				}
				else{
					top = hash_trie_node::alloc(depth,num_bits,1);
					node = top;
					depth+=node->num_bits;
					while(true){	
						if(key_to_subbits(k,depth,num_bits) != 
						 key_to_subbits(pd->k(),depth,num_bits)){
							down = hash_trie_node::alloc(depth,num_bits,2);
							node->init_with(k,down);
							node=down;
							break;
						}
						else{
							hash_trie_node* down = hash_trie_node::alloc(depth,num_bits,1);
							node->init_with(k,down);
							node=down;
							depth+=node->num_bits;
						}
					}
				}
				node->init_with(k,kv_node::alloc(k, v), 
				 pd->k(),kv_node::alloc(pd->k(), pd->v()));
				return top;
			}
		}
	}
	else if(polyinterior* pi = dynamic_cast<polyinterior*>(current)){
		/* slot doesn't exist, we need to resize the node*/
		return new resize_sub(k,v);
	}
	assert(false);return NULL;
}

static thread_local unsigned long count = 0; 

void hash_trie::on_update_success(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polynode* new_node,
	 polynode* old_node, polyoptions opts){

	//print_UIP(k,new_node,path);

	hash_trie_node* parent;

	// update parent node size
	parent = (hash_trie_node*)get_penultimate_node(path);
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

	return;
}

value* hash_trie::get_point_read_value(key* k, 
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

	assert(opts.OP==OP_READ);
	polynode* current = get_terminal_node(path);

	if(polyinterior* pi = dynamic_cast<polyinterior*>(current)){
		// got a DNE
		return NULL;
	}
	else if(polysub* pi = dynamic_cast<polysub*>(current)){
		assert(false);
	}
	else{
		return read_data_node(k,(polydata*)current);
	}
}

value* hash_trie::get(key* k){
	value* v = point_read(k,get_options);
	return v;
}

value* hash_trie::put(key* k, value* v){
	value* ans;
	update(k,v,put_options,ans,NULL);
	return v;
}

bool hash_trie::insert(key* k, value* v){
	value* ans;
	return update(k,v,insert_options,ans,NULL);
}

value* hash_trie::replace(key* k, value* v){
	value* ans;
	update(k,v,replace_options,ans,NULL);
	return ans;
}

value* hash_trie::remove(key* k){
	value* ans;
	update(k,NULL,remove_options,ans,NULL);
	return ans;
}


