/*****************************************************************************\
 *  node_info.c - get the node records of slurm
 *  see slurm.h for documentation on external functions and data structures
 *****************************************************************************
 *  Copyright (C) 2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Moe Jette <jette1@llnl.gov> et. al.
 *  UCRL-CODE-2002-040.
 *  
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://www.llnl.gov/linux/slurm/>.
 *  
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *  
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with ConMan; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
\*****************************************************************************/

#define DEBUG_SYSTEM 1

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "slurm.h"
#include <src/common/slurm_protocol_api.h>

#if DEBUG_MODULE
/* main is used here for testing purposes only */
int
main (int argc, char *argv[]) 
{
	static time_t last_update_time = (time_t) NULL;
	int error_code, i;
	node_info_msg_t * node_info_msg_ptr = NULL;
	node_table_t * node_ptr;

	error_code = slurm_load_node (last_update_time, &node_info_msg_ptr);
	if (error_code) {
		printf ("slurm_load_node error %d\n", error_code);
		exit (error_code);
	}
	node_ptr = node_info_msg_ptr -> node_array ;

	/* print everything only on smaller clusters */
	if (node_info_msg_ptr-> record_count <= 100)
			slurm_print_node_info_msg ( node_info_msg_ptr ) ;

	else {
		printf("Nodes updated at %d, record count %d\n",
			node_info_msg_ptr ->last_update, node_info_msg_ptr->record_count);

		for (i = 0; i < node_info_msg_ptr-> record_count; i++) 
		{
			/* to limit output we print only the first 10 entries, 
			 * last 1 entry, and every 200th entry */
			if ((i < 10) || (i % 200 == 0) || 
			    ((i + 1)  == node_info_msg_ptr-> record_count)) {
				slurm_print_node_table ( & node_ptr[i] ) ;
			}
			else if ((i==10) || (i % 200 == 1))
				printf ("skipping...\n");
		}
	}

	slurm_free_node_info ( node_info_msg_ptr ) ;
	exit (0);
}
#endif

/* print the entire node_info_msg */
void 
slurm_print_node_info_msg ( node_info_msg_t * node_info_msg_ptr )
{
	int i;
	node_table_t * node_ptr = node_info_msg_ptr -> node_array ;

	printf("Nodes updated at %d, record count %d\n",
		node_info_msg_ptr ->last_update, node_info_msg_ptr->record_count);

	for (i = 0; i < node_info_msg_ptr-> record_count; i++) 
	{
		slurm_print_node_table ( & node_ptr[i] ) ;
	}
}


/* print an individual node_table entry */
void
slurm_print_node_table (node_table_t * node_ptr )
{
	printf ("NodeName=%s CPUs=%u ", 
		node_ptr->name, node_ptr->cpus);
	printf ("RealMemory=%u TmpDisk=%u ", 
		node_ptr->real_memory, node_ptr->tmp_disk);
	printf ("State=%u Weight=%u ", 
		node_ptr->node_state, node_ptr->weight);
	printf ("Features=%s Partition=%s\n\n", 
		node_ptr->features, node_ptr->partition);
}


/* slurm_load_node - load the supplied node information buffer if changed */
int
slurm_load_node (time_t update_time, node_info_msg_t **node_info_msg_pptr)
{
        int msg_size ;
        int rc ;
        slurm_fd sockfd ;
        slurm_msg_t request_msg ;
        slurm_msg_t response_msg ;
        last_update_msg_t last_time_msg ;
	return_code_msg_t * slurm_rc_msg ;

        /* init message connection for message communication with controller */
        if ( ( sockfd = slurm_open_controller_conn ( SLURM_PORT ) ) == SLURM_SOCKET_ERROR )
                return SLURM_SOCKET_ERROR ;


        /* send request message */
        last_time_msg . last_update = update_time ;
        request_msg . msg_type = REQUEST_NODE_INFO ;
        request_msg . data = &last_time_msg ;
        if ( ( rc = slurm_send_controller_msg ( sockfd , & request_msg ) ) == SLURM_SOCKET_ERROR )
                return SLURM_SOCKET_ERROR ;

        /* receive message */
        if ( ( msg_size = slurm_receive_msg ( sockfd , & response_msg ) ) == SLURM_SOCKET_ERROR )
                return SLURM_SOCKET_ERROR ;
        /* shutdown message connection */
        if ( ( rc = slurm_shutdown_msg_conn ( sockfd ) ) == SLURM_SOCKET_ERROR )
                return SLURM_SOCKET_ERROR ;

	switch ( response_msg . msg_type )
	{
		case RESPONSE_NODE_INFO:
        		 *node_info_msg_pptr = ( node_info_msg_t * ) response_msg . data ;
			break ;
		case RESPONSE_SLURM_RC:
			slurm_rc_msg = ( return_code_msg_t * ) response_msg . data ;
			break ;
		default:
			return SLURM_UNEXPECTED_MSG_ERROR ;
			break ;
	}

        return SLURM_SUCCESS ;
}

