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

#ifndef POLYOPTIONS_HPP
#define POLYOPTIONS_HPP


enum {FUNC_GET, FUNC_PUT, FUNC_INSERT, 
	FUNC_REMOVE, FUNC_REPLACE, FUNC_RANGE};

enum {OP_READ, OP_UPDATE, OP_RANGE, OP_SUB};

typedef union{
uint64_t all_flags;      /* Allows us to refer to the flags 'en masse' */
 struct{
  uint64_t 
		/* exterior functions called - 
		 used to detect flag configuration violations */
		FUNC : 3,
		/* interior methods used - 
		 used to detect flag configuration violations */
		OP : 2,	

		/* search options -
		 what actions to take on a basic search and
		 how to speculate wrt pending nodes.  */

		// TODO: difference between sub and ipu?
		SEARCH_HELP_CLEAN_ABORTS : 1, /* else will bypass */
		SEARCH_HELP_RESOLVE_PENDING : 1, /* else will bypass via old or pending */
		SEARCH_SPECULATE_VIA_PENDING : 1, /* speculate the pending op succeeds */

		/* preapprove options - 
		 some of these options don't make sense 
		 with search options, be careful.  For instance, if you 
		 speculate via pending, but then resolve pending in preapprove, 
		 you will always fail validation.*/
		SKIP_PREAPPROVE : 1,
		PREAPPROVE_HELP_CLEAN_ABORTS : 1, 
		PREAPPROVE_HELP_RESOLVE_PENDING : 1, 

		/* validation options -
		 two sets, based on whether we are validating ourselves
		 or helping to complete someone else.  Be careful using these
		 options, it may be possible to introduce blocking. */
		VALIDATE_ME_IGNORE_RANGE_QUERY : 1,
		VALIDATE_OTHERS_IGNORE_RANGE_QUERY : 1,
		VALIDATE_ME_HELP_CLEAN_ABORTS : 1,
		VALIDATE_OTHERS_HELP_CLEAN_ABORTS : 1,

		/* completion options */
		COMPLETION_HELP_CLEAN_ABORTS : 1, 

		/* read options */
		READ_HELP_CLEAN_ABORTS : 1, 
		READ_HELP_RESOLVE_PENDING : 1, 

		// 22 flags above here

		MAX_DEPTH : 16,
		CUSTOM_FLAGS : 16,
		
		remainder : 10;
 };
} polyoptions;



#endif