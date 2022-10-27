/*
 * File: mtp_build.h
 */

#ifndef MTP_BUILD_H
#define MTP_BUILD_H

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/if_ether.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <signal.h>
#include <ctype.h>
#include <mcheck.h>
#include <stdbool.h>

#include "mtp_send.h"
#include "mtp_utils.h"



// void sendHelloNR( struct control_ports *currentMTPPort, uint8_t VID);  -- Mana
void send_hello_NR( char *current_port_name, char **VID_array, uint16_t numOfVID);
void send_join_req( char *current_port_name, char **VID_array, uint16_t numOfVID);
void send_join_res( char *current_port_name, char **VID_array, uint16_t numOfVID);
void send_join_ack( char *current_port_name, char **VID_array, uint16_t numOfVID);
void send_start_hello(char *current_port_name);


void route_data_from_tor_to_spine(char *current_port_name, uint16_t src_VID, uint16_t dest_VID,unsigned char *ip_header_with_payload, unsigned int ip_header_and_payload_len);
void route_data_from_spine(char *current_port_name,unsigned char *payload, unsigned int payload_len);

void route_data_to_server(char *current_port_name,unsigned char *payload, unsigned int payload_len);

int send_keep_alive(char *current_port_name);

int send_failure_update(char *current_port_name, char** VID_array, uint16_t VID_array_size, uint8_t update_option);

int send_recover_update(char *current_port_name, char** VID_array, uint16_t VID_array_size, uint8_t update_option);


size_t build_VID_data_payload(unsigned char *payload, char** VID_array, uint16_t VID_array_size);

#endif