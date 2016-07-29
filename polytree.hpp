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

#ifndef POLYTREE_HPP
#define POLYTREE_HPP

#include <assert.h>

#include "fptr.hpp"
#include "polynodes.hpp"
#include "polysub.hpp"
#include "polyoptions.hpp"
#include "simplevector.hpp"

class polytree{
protected:
	fptr<polynode> root;

	polytree(){root.init(0,FP_VALID);}

private:

	// major algorithm methods =======================================

	/*
	Simple search for a key. Never triggers a substitution.
	key k : the key to find
	path : an empty list; 
		-To be populated with the path, 
		sorted from top (front) to bottom (back).
		-The dummy root node is always the front most object
		-The data node (if present) is the back most object
	Returns success (TRUE) or failure (FALSE) wrt finding the data
	*/
	bool search(key* k, simple_vector<fptr_val<polynode>>& path, 
	 polyoptions opts);


	/*
	Preapproves a path for an operation's future use,
	cleaning nodes as necessary / possible.
	path : the path from 'search' which was used to find
		the operation entry point (includes old value of pending_ptr).
	opts : assorted options
	Returns true if the operation could succeed with the current path.
	*/
	bool preapprove(simple_vector<fptr_val<polynode>>& path, 
	 polyoptions opts);

	/*
	Validates a pending operation
	pending_ptr : the pending fptr to validate (points 
		to pending operation / node)
	pending_val : the value of the pending fptr
	path : the path from 'search' which was used to find
		the operation entry point (includes old value of pending_ptr).
	opts : assorted options
	1) examines the path to determine what needs to be validated
			(last N pointers, where N is max substitution size).
	2) snapshots these values
			(verifies they either haven't changed, or ongoing substitution
			is still pending)
	3) sets the pending operation to VALID
	Returns true if I did the actual validation, false otherwise
	*/
	bool validate_common(fptr<polynode>* pending_ptr, polynode* pending_val,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts ,bool helping,
	 bool is_read);


	bool construct_substitution(polysub* sub,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts);

	bool insert_substitution_at(fptr<polynode>* here, polysub* sub, 
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts);

	bool insert_update_in_place_at(fptr<polynode>* here, polynode* new_node,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts);

	// utility inline methods =========================

	void inline help_pending(fptr<polynode>* pending_ptr, polynode* pending_val,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

		assert(pending_val!=NULL);
		if(polydata* pd = dynamic_cast<polydata*>(pending_val)){
			validate_others_op(pending_ptr,pd,path,opts);
		}
		else if(polyinterior* pi = dynamic_cast<polyinterior*>(pending_val)){
			validate_others_op(pending_ptr,pi,path,opts);
		}
		else if(polysub* ps = dynamic_cast<polysub*>(pending_val)){
			help_substitution(ps,path,opts);
		}
		/*
		else if(polyrange* pr = dynamic_cast<polyrange*>(pending_val)){
			pr->help();
		}
		*/
		else{
			assert(false);
		}
	}


	/*
	Helps a slot resolve from possible PENDING or ABORTED
	to a VALID value.  
	here : the possibly pending/aborted pointer
	here_cpy : a local copy of here
	path : the path taken to get here
	opts : if any validation taken should check range queries
	Returns true iff here_cpy->flag()==FP_VALID; a false
		return value indicates a need to reread the 'here' pointer.
	*/
	bool inline help_to_valid(fptr<polynode>* here, fptr_local<polynode> here_cpy,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

		/* This method enforces an important variant:
			ABORTED nodes must be removed before any new
			PENDING nodes are added.  PENDING nodes
			must be resolved before additional PENDING
			nodes are added (especially since the state
			is in the pointer :)
		*/

		/* Resolve a previous pending value */
		if(here_cpy.flag() == FP_PENDING){
			help_pending(here,here_cpy,path,opts);
			return false;
		}
		/* Resolve a previous aborted value */
		if(here_cpy.flag() == FP_ABORTED){
			clean_aborted(here,here_cpy.ptr());
			return false;
		}

		return true;
	}


	/*
	Help complete an encountered polysub.
	*/
	void inline help_substitution(polysub* sub,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){

		/* Construct the substitution by creating the
		new subtree and replacing the polysub
		with it.*/
		construct_substitution(sub,path,opts);
		/* Then validate the original path to get here. */
		validate_others_op(sub->to_here, 
		 sub->new_peak.load(std::memory_order::memory_order_acquire),
		 path, opts);
	}


	/*
	Removes an aborted node from the head of a slot.
	aborted_ptr : the slot pointer.
	aborted_node : the node that was aborted.
		-At the end of this method, aborted_node is disconnected
		from the polytree.
	Returns true if node was actually removed by this thread; false otherwise.
	The thread owning the aborted operation is responsible for garbage
	collecting the removed and aborted node.
	*/
	bool inline clean_aborted(fptr<polynode>* aborted_ptr, polynode* aborted_node){
		assert(aborted_node!=NULL);
		polynode* prev = aborted_node->prev;
		return aborted_ptr->CAS(aborted_node, FP_ABORTED, prev, FP_VALID);
		/* aborted thread is responsible for garbage collection */
	}

	
	bool inline check_for_completion(fptr<polynode>* here, 
	 polynode* new_node, polynode* prev, polyoptions opts){
		fptr_local<polynode> here_cpy;
		here_cpy.init(here->all());
		polynode* current;
		current = here_cpy.ptr();
		/* Remove, if necessary, an ABORTED node from the top
			of the slot. Everything else must be VALID. */
		if(here_cpy.flag()==FP_ABORTED){
			if(opts.COMPLETION_HELP_CLEAN_ABORTS){clean_aborted(here,current);}
			if(current==new_node){return false;}
			else{current = current->prev;};
		}

		while(true){
			/* should never happen because this 
			 means prev was disconnected, but prev had to be valid */
			// TODO: remove this assert once we have garbage reclamation....
			assert(current!=NULL); 

			if(current==prev){return false;}
			if(current==new_node){return true;}
			current = current->prev;
		}
	}


	bool inline attach_new_peak(polysub* sub, polynode* my_new_peak){
		polynode* x = NULL;
		return sub->new_peak.compare_exchange_strong(
			x,my_new_peak,std::memory_order::memory_order_acq_rel);
	}


	bool inline validate_my_op(fptr<polynode>* pending_ptr, 
	 polynode* pending_val, polynode* prev,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){
		/* Validate pending value */
		if(validate_common(pending_ptr, pending_val, path, opts,false,false)){
			return true;
		}
		/* we didn't personally validate our operation, 
			so it could have failed or succeeded */
		if(check_for_completion(pending_ptr, pending_val, prev, opts)){
			return true;
		}
		/* our operation was never validated*/
		return false;
	}

	bool inline validate_others_op(fptr<polynode>* pending_ptr, 
	 polynode* pending_val,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){
		return validate_common(pending_ptr, pending_val, path, 
		 opts,true,false);
	}


	bool inline validate_my_read(fptr<polynode>* pending_ptr, 
	 polynode* pending_val,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts){
		return validate_common(pending_ptr, pending_val, 
		 path, opts, false, true);
	}



protected:

	// utility methods ============================

	inline polynode* get_terminal_node(simple_vector<fptr_val<polynode>>& path){
		fptr<polynode>* here;
		fptr_local<polynode> here_cpy;
		fptr_val<polynode> fv;
		uint64_t flag;
		polynode* current;

		if(path.size()>0){
			fv = path.back();
			here = fv.fp();
			here_cpy = fv.val();
			current = here_cpy.ptr();
			return current;
		}
		return NULL;
	}

	inline polynode* get_penultimate_node(simple_vector<fptr_val<polynode>>& path){
		fptr<polynode>* here;
		fptr_local<polynode> here_cpy;
		fptr_val<polynode> fv;
		uint64_t flag;
		polynode* parent;

		if(path.size()>1){
			fv = path.get(path.size()-2);
			here = fv.fp();
			here_cpy = fv.val();
			parent = here_cpy.ptr();
			return parent;
		}
		return NULL;
	}


	// options methods ============================

	uint64_t always_help_clean_abort(){
		polyoptions opts;
		opts.all_flags = 0;
		opts.SEARCH_HELP_CLEAN_ABORTS = 1;
		opts.PREAPPROVE_HELP_CLEAN_ABORTS = 1;
		opts.VALIDATE_ME_HELP_CLEAN_ABORTS = 1;
		opts.VALIDATE_OTHERS_HELP_CLEAN_ABORTS = 1;
		opts.READ_HELP_CLEAN_ABORTS = 1;
		opts.COMPLETION_HELP_CLEAN_ABORTS = 1;
		return opts.all_flags;
	}

	uint64_t always_help_resolve_pending(){
		polyoptions opts;
		opts.all_flags = 0;
		opts.SEARCH_HELP_RESOLVE_PENDING = 1;
		opts.PREAPPROVE_HELP_RESOLVE_PENDING = 1;
		opts.READ_HELP_RESOLVE_PENDING = 1;
		return opts.all_flags;
	}

	// utility API methods for descendants ===================
	// these methods are for simple update cases
	// on trees where data is generally stored in kv_nodes and 
	// removals use a remove node.

	inline kv_node* get_update_node_at_NULL(key* k, value* v, 
	 polyoptions opts, value*& ans_value_out){
		ans_value_out = NULL;
		switch(opts.FUNC){
		case FUNC_PUT:
		case FUNC_INSERT:
			return kv_node::alloc(k, v);
		case FUNC_REPLACE:
		case FUNC_REMOVE:
			return NULL;
		}
		assert(false);return NULL;
	}

	inline polynode* get_update_node_at_equals(key* k, value* v, 
	  kv_node* old_node, polyoptions opts,value*& ans_value_out){
		ans_value_out = old_node->v();
		switch(opts.FUNC){
		case FUNC_PUT:
		case FUNC_REPLACE:
			return kv_node::alloc(k, v);
		case FUNC_INSERT:
			return NULL;
		case FUNC_REMOVE:
			return removed_node::alloc(k);				
		}
		assert(false);return NULL;
	}

	inline kv_node* get_update_node_at_removed(key* k, value* v, 
	  removed_node* old_node, polyoptions opts,value*& ans_value_out){
		return get_update_node_at_NULL(k,v,opts,ans_value_out);
	}

	inline value* read_data_node(key* k, polydata* current){
		if(kv_node* pd = dynamic_cast<kv_node*>(current)){
			if(pd->k()->equals(k)){return pd->v();}
			else{return NULL;}
		}
		else if(dynamic_cast<removed_node*>(current)){
			return NULL;
		}
		else if(current == NULL){return NULL;}
		assert(false);return NULL;
	}

	// key API methods for descendants ======================
	value* point_read(key* k, polyoptions opts);

	bool update(key* k, value* v, polyoptions opts, 
	 simple_vector<fptr_val<polynode>>& path, polynode*& old_node, 
	 polynode*& new_node, value*& ans_value, void* params);

	bool update(key* k, value* v, polyoptions opts, 
	 value*& ans_value_out, void* params);

	// call back methods ====================================
	virtual polynode* get_update_node(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts,
	 value*& ans_value_out, bool& callback_out, void* params) = 0;
	virtual value* get_point_read_value(key* k, 
	 simple_vector<fptr_val<polynode>>& path, polyoptions opts) = 0;

	virtual void on_update_success(key* k, value* v,
	 simple_vector<fptr_val<polynode>>& path, polynode* new_node,
	 polynode* old_node, polyoptions opts) = 0;

};


// utility functions
unsigned long local_rand();
void print_UIP(key* k, polynode* new_node, 
	simple_vector<fptr_val<polynode>>& path);

#endif





