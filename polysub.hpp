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

#ifndef POLYSUB_HPP
#define POLYSUB_HPP

#include <assert.h>
#include <list>

#include "fptr.hpp"
#include "polynodes.hpp"
#include "polyoptions.hpp"
#include "simplevector.hpp"

class polytree;


class polysub : public polynode{

public:
	fptr<polynode>* to_here = NULL;

	std::atomic<polynode*> new_peak;

	polysub(){
		new_peak = NULL;
	}

	virtual std::string to_string() const{
		return std::string("polysub");
	}

	/*
	Build a replacement subtree locally.  Returns the root (peak)
	of the new subtree.  Needs to abort/complete any PENDING pointers in the danger
	zone.

	This object should be initialized with sufficient information
	such that any thread can complete build.
	*/
	virtual polynode* build() = 0;

	/*
	Utility method for clearing the danger zone.  Should be called
	on every slot within the danger zone.
	*/
	fptr_local<polynode> inline abort_to_valid(fptr<polynode>* here){

		while(true){
			fptr_local<polynode> here_cpy = here->all();
			/* Resolve a previous pending value by aborting*/
			if(here_cpy.flag() == FP_PENDING){
				here->abort(here_cpy);
				continue;
			}
			/* Ignore previous aborted value */
			if(here_cpy.flag() == FP_ABORTED){
				fptr_local<polynode> retval;
				retval.init(here_cpy->prev,FP_VALID);
				return retval;
			}
			/* Use valid value */
			else{
				return here_cpy;
			}
		}

	}

};


#endif








