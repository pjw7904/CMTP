#ifndef MTP_UTILS_H
#define MTP_UTILS_H

/*
 * Standard library imports.
 */
#include <stdint.h>
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
#include <netinet/ether.h>
#include <linux/if_packet.h>

/*
 * Custom MTP imports.
 */
// None

/*****************************************
 * CONSTANTS 
 *****************************************/
#define MTP_TYPE_HELLONR_MSG 1 // Hello message with no repsonse
#define MTP_TYPE_HELLOWR_MSG 2 // Hello message with repsonse
#define MTP_TYPE_JOIN_REQ 3 // Join Request 
#define MTP_TYPE_JOIN_RES 4 // Join Response
#define MTP_TYPE_JOIN_ACK 5 // Join ACK
#define MTP_TYPE_KEEP_ALIVE 6
#define MTP_TYPE_DATA_MSG 7
#define MTP_TYPE_FAILURE_UPDATE 8
#define MTP_TYPE_RECOVER_UPDATE 9
#define MTP_TYPE_START_HELLO 10

#define MAX_BUFFER_SIZE 2048

#define ETH_LEN 25 // Ethernet interface naming length (ex: eth1).
#define VID_LEN 64

#define HELLO_TIMER 100 // prior 10 in GENI.
#define DEAD_TIMER 250 // prior 25 in GENI.

#define DETECT_FAIL 1
#define MISS_FAIL 2

#define ETH_MTP_CTRL 0x8850 // MTP Ethertype (custom, it is a currently unregistered value).
#define ETH_IP_CTRL 0x0800  // IPv4 Ethertype (offical registered value).

#define UNREACHABLE_OPTION 1
#define REACHABLE_OPTION 0

#define ADD_OPERATION 1
#define REMOVE_OPERATION 0

/*****************************************
 * STRUCTURES 
 *****************************************/
// None

/*****************************************
 * FUNCTION PROTOTYPES 
 *****************************************/
/**
 * @brief Get the all ethernet interface2 object
 * 
 * @param dest destination array to store ethernet interface name
 * @return uint8_t total working ethernet interface
 */
uint8_t get_all_ethernet_interface2(char** dest);

/**
 * @brief Get the VID by ethernet interface object
 * 
 * @param dest store VID in this pointer
 * @param ethernet_interface_name which interface
 * @param octet get which byte of IP address as VID, 3rd byte is default
 */
void getRootVID(char *dest, char *ethernet_interface_name, uint8_t octet);

/**
 * @brief check whether the provided ethernet interface name is alive
 * 
 * @param port_array ethernet interface list
 * @param port_array_size ethernet interface size
 * @param port_name provided interface name
 * @return int 0 for false, 1 for true
 */
int check_port_is_alive(char** port_array, uint8_t port_array_size,char* port_name);

/**
 * @brief Get the tier from hello message payload
 * 
 * @param payload_with_VID_data get tier 
 * @return uint8_t 
 */
uint8_t get_tier_from_hello_message(char *payload_with_VID_data);

/**
 * @brief append port number after VID
 * 
 * @param port_name which port want to choose
 * @param dest append after this pointer
 */
void append_port_number_after_VID(char *port_name, char *dest);

/**
 * @brief extract VIDs from payload
 * 
 * @param VID_array store VIDs in here
 * @param recvBuff_start_ptr payload
 * @param with_debug 
 * @return uint16_t 
 */
uint16_t extract_VID_from_receive_buff(char **VID_array, char *recvBuff_start_ptr,int with_debug);

/**
 * @brief hashing algorithm for load balancing, for more https://en.wikipedia.org/wiki/Jenkins_hash_function
 * 
 * @param key key to be hashed
 * @param len length of key
 * @return uint32_t a hashcode
 */
uint32_t jenkins_one_at_a_time_hash(uint8_t *key, size_t len);

/**
 * @brief convert integer to string
 * 
 * @param dest_storage store string in here
 * @param number number to be converted
 * @return size_t 
 */
size_t int_to_str(char *dest_storage, unsigned int number);

/**
 * @brief Get current time in milli second
 * 
 * @param current_time 
 * @return long long milli second
 */
long long get_milli_sec(struct timeval* current_time);

#endif