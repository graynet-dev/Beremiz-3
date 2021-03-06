/* File generated by Beremiz (PlugGenerate_C method of modbus Plugin instance) */

/*
 * Copyright (c) 2016 Mario de Sousa (msousa@fe.up.pt)
 *
 * This file is part of the Modbus library for Beremiz and matiec.
 *
 * This Modbus library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this Modbus library.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This code is made available on the understanding that it will not be
 * used in safety-critical situations without a full and competent review.
 */

#define DEF_REQ_SEND_RETRIES 0
#define DEF_CLOSE_ON_SILENCE 1
#include "plc_abi.h"
#if defined(PLC_IS_APP) && !defined(PLC_WIN)
#else
#include <iec_types_all.h>
#include "mb_tcp_private.h"
#include "mb_master_private.h"
#ifdef ARDUINO
#include <STM32FreeRTOS.h>
#include "semphr.h"
#ifdef LWIP
#include "lwip/inet.h"
#endif
#endif
#endif
%(buffer)s

#ifndef INADDR_ANY
#define INADDR_ANY 0x00000000UL
#endif

static memarea server_mem_robits_t[] =
	{
		%(server_mem_robits_t)s
	0};
static memarea server_mem_rwbits_t[] =
	{
		%(server_mem_rwbits_t)s
		0};
static memarea server_mem_rowords_t[] =
	{
		%(server_mem_rowords_t)s
		0};
static memarea server_mem_words_t[] =
	{
		%(server_mem_words_t)s
		0

};


/* The total number of nodes, needed to support _all_ instances of the modbus plugin */
#define TOTAL_TCPNODE_COUNT %(total_tcpnode_count)s
#define TOTAL_RTUNODE_COUNT %(total_rtunode_count)s
#define TOTAL_ASCNODE_COUNT %(total_ascnode_count)s

/* Values for instance %(locstr)s of the modbus plugin */
#define MAX_NUMBER_OF_TCPCLIENTS %(max_remote_tcpclient)s

#define NUMBER_OF_TCPSERVER_NODES %(tcpserver_node_count)s
#define NUMBER_OF_TCPCLIENT_NODES %(tcpclient_node_count)s
#define NUMBER_OF_TCPCLIENT_REQTS %(tcpclient_reqs_count)s

#define NUMBER_OF_RTUSERVER_NODES %(rtuserver_node_count)s
#define NUMBER_OF_RTUCLIENT_NODES %(rtuclient_node_count)s
#define NUMBER_OF_RTUCLIENT_REQTS %(rtuclient_reqs_count)s

#define NUMBER_OF_ASCIISERVER_NODES %(ascserver_node_count)s
#define NUMBER_OF_ASCIICLIENT_NODES %(ascclient_node_count)s
#define NUMBER_OF_ASCIICLIENT_REQTS %(ascclient_reqs_count)s

#define NUMBER_OF_SERVER_NODES (NUMBER_OF_TCPSERVER_NODES + \
								NUMBER_OF_RTUSERVER_NODES + \
								NUMBER_OF_ASCIISERVER_NODES)

#define NUMBER_OF_CLIENT_NODES (NUMBER_OF_TCPCLIENT_NODES + \
								NUMBER_OF_RTUCLIENT_NODES + \
								NUMBER_OF_ASCIICLIENT_NODES)

#define NUMBER_OF_CLIENT_REQTS (NUMBER_OF_TCPCLIENT_REQTS + \
								NUMBER_OF_RTUCLIENT_REQTS + \
								NUMBER_OF_ASCIICLIENT_REQTS)

/*initialization following all parameters given by user in application*/


%(client_request_buffer)s

static client_request_t client_requests[NUMBER_OF_CLIENT_REQTS] = {
	%(client_req_params)s};

static client_node_t client_nodes[NUMBER_OF_CLIENT_NODES] = {
	%(client_nodes_params)s};

static server_node_t server_nodes[NUMBER_OF_SERVER_NODES] = {
	%(server_nodes_params)s};

/*******************/
/*located variables*/
/*******************/

%(loc_vars)s
