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

#include "chromaticnodes.hpp"
#include <iostream>

using namespace std;


polynode* delete_sub::build(){

	chromatic_interior_node* top = dynamic_cast<chromatic_interior_node*>(prev);
	if(top == NULL){assert(false);}

	// copy old
	chromatic_node* left = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[0])).ptr());
	chromatic_node* right = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[1])).ptr());

	// compute new weights
	int64_t new_weight;
	bool delete_left;
	if(left==victim){ delete_left = true;}
	else if(right==victim){ delete_left = false;}
	// TODO correct casting to get this check to work...
	//else if(left!=NULL && left->k()->equals(k)){ delete_left = true;}
	//else if(right!=NULL && right->k()->equals(k)){ delete_left = false;}
	else{return NULL;}

	polynode* new_peak;
	if(delete_left){
		new_weight = right->weight()+top->weight();
		new_peak = right->duplicate(new_weight);
	}
	else{
		new_weight = left->weight()+top->weight();
		new_peak = left->duplicate(new_weight);
	}
	
	return new_peak;
}



polynode* rotation_a_d::build(){

	chromatic_interior_node* top = dynamic_cast<chromatic_interior_node*>(prev);
	if(top == NULL){assert(false);}

	// copy old
	chromatic_node* left = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[0])).ptr());
	chromatic_node* right = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[1])).ptr());

	// verify children exist and rotation is possible
	if(left == NULL || right == NULL){return NULL;}

	// need grandchildren copied also
	chromatic_node *ll=NULL,*lr=NULL,*rl=NULL,*rr=NULL;
	chromatic_interior_node *lint=NULL, *rint=NULL;
	if((lint = dynamic_cast<chromatic_interior_node*>(left)) ){
		ll=dynamic_cast<chromatic_node*>(abort_to_valid(&(lint->slots[0])).ptr());
		lr=dynamic_cast<chromatic_node*>(abort_to_valid(&(lint->slots[1])).ptr());
	}
	if((rint = dynamic_cast<chromatic_interior_node*>(right)) ){
		rl=dynamic_cast<chromatic_node*>(abort_to_valid(&(rint->slots[0])).ptr());
		rr=dynamic_cast<chromatic_node*>(abort_to_valid(&(rint->slots[1])).ptr());
	}

	// verify weights justify a rotation 
	// and create the new nodes
	int tw = top->weight();
	int lw = left->weight();
	int rw = right->weight();
	chromatic_interior_node* new_peak;
	polynode *new_left, *new_right;
	if(tw>0 && lw==0 && rw==0){
		// chose weights for rotation a
		new_peak = (chromatic_interior_node*)top->duplicate(tw-1);
		new_left = left->duplicate(1);
		new_right = right->duplicate(1);
	}
	else if((lw>too_heavy && rw>0) || (lw>0 && rw>too_heavy)){
		// chose weights for rotation d
		new_peak = (chromatic_interior_node*)top->duplicate(tw+1);
		new_left = left->duplicate(lw-1);
		new_right = right->duplicate(rw-1);
	}
	else{
		// no rotation applicable
		return NULL;
	}

	// build the sub tree
	new_peak->local_insert_at(&new_peak->slots[0],new_left);
	new_peak->local_insert_at(&new_peak->slots[1],new_right);
	if(lint!=NULL){
		chromatic_interior_node* nlint = (chromatic_interior_node*)new_left;
		nlint->local_insert_at(&nlint->slots[0],dynamic_cast<polynode*>(ll));
		nlint->local_insert_at(&nlint->slots[1],dynamic_cast<polynode*>(lr));
	}
	if(rint!=NULL){
		chromatic_interior_node* nrint = (chromatic_interior_node*)new_right;
		nrint->local_insert_at(&nrint->slots[0],dynamic_cast<polynode*>(rl));
		nrint->local_insert_at(&nrint->slots[1],dynamic_cast<polynode*>(rr));
	}
	return new_peak;

}




polynode* rotation_b_e::build(){

	chromatic_interior_node* top = dynamic_cast<chromatic_interior_node*>(prev);
	if(top == NULL){assert(false);}

	int longer = right_long;
	int shorter = !right_long;

	// copy old children
	chromatic_node* lng = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[longer])).ptr());
	chromatic_node* shrt = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[shorter])).ptr());

	// verify children exist and rotation is possible
	if(lng == NULL || shrt == NULL){return NULL;}

	// need grandchildren copied also
	chromatic_node *ll=NULL,*ls=NULL;
	chromatic_interior_node *lint=NULL;
	if((lint = dynamic_cast<chromatic_interior_node*>(lng)) ){
		ll=dynamic_cast<chromatic_node*>(abort_to_valid(&(lint->slots[longer])).ptr());
		ls=dynamic_cast<chromatic_node*>(abort_to_valid(&(lint->slots[shorter])).ptr());
	}
	else{
		// long side must have children for rotation to proceed
		return NULL;
	}


	// verify weights justify a rotation 
	// and create the new nodes
	int tw = top->weight();
	int lw = lng->weight();
	int sw = shrt->weight();
	int llw = ll->weight();
	int lsw = ls->weight();
	chromatic_interior_node* new_peak;
	chromatic_interior_node *new_shrt;
	if(lw==0 && llw==0 && lsw>0){
		// chose nodes for rotation b
		new_peak = (chromatic_interior_node*)lng->duplicate(tw);
		new_peak->local_insert_at(&new_peak->slots[longer],dynamic_cast<polynode*>(ll));
		new_shrt = (chromatic_interior_node*) top->duplicate(0);
		new_peak->local_insert_at(&new_peak->slots[shorter],new_shrt);
		new_shrt->local_insert_at(&new_shrt->slots[longer],dynamic_cast<polynode*>(ls));
		new_shrt->local_insert_at(&new_shrt->slots[shorter],dynamic_cast<polynode*>(shrt));
		return new_peak;
	}
	// TODO: verify these rotations are actually EXACTLY the same, then merge 
	// if statements.
	else if(tw>0 && ((lw>too_heavy && sw>0) || (lw>0 && sw>too_heavy))){
		// chose nodes for rotation e
		new_peak = (chromatic_interior_node*)lng->duplicate(tw);
		new_peak->local_insert_at(&new_peak->slots[longer],dynamic_cast<polynode*>(ll));
		new_shrt = (chromatic_interior_node*) top->duplicate(0);
		new_peak->local_insert_at(&new_peak->slots[shorter],new_shrt);
		new_shrt->local_insert_at(&new_shrt->slots[longer],dynamic_cast<polynode*>(ls));
		new_shrt->local_insert_at(&new_shrt->slots[shorter],dynamic_cast<polynode*>(shrt));
		return new_peak;
	}
	// no rotation applicable
	return NULL;
	
}





polynode* rotation_c::build(){

	chromatic_interior_node* top = dynamic_cast<chromatic_interior_node*>(prev);
	if(top == NULL){assert(false);}

	int longer = right_long;
	int shorter = !right_long;

	// copy old
	chromatic_node* lng = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[longer])).ptr());
	chromatic_node* shrt = 
	 dynamic_cast<chromatic_node*>(abort_to_valid(&(top->slots[shorter])).ptr());

	// verify children exist and rotation is possible
	if(lng == NULL || shrt == NULL){return NULL;}

	// need grandchildren copied also
	chromatic_node *ll=NULL,*ls=NULL;
	chromatic_interior_node *lint=NULL;
	if((lint = dynamic_cast<chromatic_interior_node*>(lng)) ){
		ll=dynamic_cast<chromatic_node*>(abort_to_valid(&(lint->slots[longer])).ptr());
		ls=dynamic_cast<chromatic_node*>(abort_to_valid(&(lint->slots[shorter])).ptr());
	}
	else{
		// long side must have children for rotation to proceed
		return NULL;
	}

	// need greatgrandchildren also
	chromatic_interior_node *lsint=NULL;
	chromatic_node *lsl=NULL,*lss=NULL;
	if((lsint = dynamic_cast<chromatic_interior_node*>(ls)) ){
		lsl=dynamic_cast<chromatic_node*>(abort_to_valid(&(lsint->slots[longer])).ptr());
		lss=dynamic_cast<chromatic_node*>(abort_to_valid(&(lsint->slots[shorter])).ptr());
	}
	else{
		// long-short side must have children for rotation to proceed
		return NULL;
	}

	// verify weights justify a rotation 
	// and create the new nodes
	int tw = top->weight();
	int lw = lng->weight();
	int sw = shrt->weight();
	int lsw = ls->weight();
	chromatic_interior_node* new_peak;
	chromatic_interior_node *new_lng, *new_shrt;
	if(lw==0 && lsw==0 && sw>0){
		// chose nodes for rotation c
		new_peak = (chromatic_interior_node*)ls->duplicate(tw);
		new_lng = (chromatic_interior_node*) lng->duplicate(0);
		new_shrt = (chromatic_interior_node*) top->duplicate(0);
		new_peak->local_insert_at(&new_peak->slots[longer],new_lng);
		new_peak->local_insert_at(&new_peak->slots[shorter],new_shrt);

		new_lng->local_insert_at(&new_lng->slots[longer],dynamic_cast<polynode*>(ll));
		new_lng->local_insert_at(&new_lng->slots[shorter],dynamic_cast<polynode*>(lsl));

		new_shrt->local_insert_at(&new_shrt->slots[longer],dynamic_cast<polynode*>(lss));
		new_shrt->local_insert_at(&new_shrt->slots[shorter],dynamic_cast<polynode*>(shrt));
		return new_peak;
	}
	// no rotation applicable
	return NULL;
	
}






