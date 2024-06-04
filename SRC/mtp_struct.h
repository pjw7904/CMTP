#ifndef MTP_STRUCT_H
#define MTP_STRUCT_H

/*
 * Standard library imports.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Custom MTP imports.
 */
#include "mtp_utils.h" // Access MTP constants.

/*****************************************
 * CONSTANTS 
 *****************************************/
// None

/*****************************************
 * STRUCTURES 
 *****************************************/
struct control_port {
    char port_name[ETH_LEN];  // Control Port Name
    int isUP;                     // true [1] (if UP) or false [2] (if DOWN)
    int start;
    int fail_type;
    long long last_received_time;
    long long last_sent_time;
    int continue_count;
    struct notification* ntf_head;
    uint8_t frame[MAX_BUFFER_SIZE];
    struct sockaddr_ll* socket_address;
    struct control_port *next;
};

typedef struct compute_interface {
    char port_name[ETH_LEN];        // Interface name
    uint8_t frame[MAX_BUFFER_SIZE]; // Starting frame
    struct sockaddr_ll* socket_address;
    struct compute_interface *next;
} compute_interface;


// 8_1_22 this link list will store the list of VIDs offered on a single port
struct VID{
   char VID_name[VID_LEN];
   struct VID *next;
};

struct unreachable_table{
    struct VID* VID_head;
};

struct reachable_table{
    struct VID* VID_head;
};

//8_1_2022 when I offer VIDs to my upper tier spines, I store them in this linked list and refer using the port number
// A node can have multiple VIDs. It can thus offer multiple VIDs over a single port
// I will store the multiple ports on which VIDs were offered by this node
// struct VIDs_Offered_OnAPort stores the offered VIDs up link port
struct VID_offered_port{
    char port_name[ETH_LEN]; 
    struct VID *VID_head;
    struct unreachable_table* ut;
    struct reachable_table* rt;
    struct control_port* cp;
    struct VID_offered_port *next;
};

// struct VID_Accepted_ports stores the accepted VIDs up link port
struct VID_accepted_port{
    char port_name[ETH_LEN]; 
    struct VID *VID_head;
    struct unreachable_table* ut;
    struct reachable_table* rt;
    struct control_port* cp;
    struct VID_accepted_port *next;
};

struct notification{
    char from_port_name[ETH_LEN];
    int table_option;
    int operation_option;
    struct notification* next;
};

/*****************************************
 * FUNCTION PROTOTYPES 
 *****************************************/
// ====================== function for control_port ====================== //
struct control_port* add_to_control_port_table(struct control_port* cp_head, char* new_port_name);
struct control_port* build_control_port(char* new_port_name);
struct control_port* find_control_port_by_name(struct control_port* cp_head, char* port_name);
struct control_port* remove_control_port_by_name(struct control_port* cp_head, char* port_name);
struct control_port* clear_control_port(struct control_port* node);
struct control_port* setControlInterfaces(struct ifaddrs *ifaddr, char *computeSubnetIntfName, bool isLeaf);
int is_all_down(struct control_port* cp_head);
void print_control_port_table(struct control_port* cp_head);
void initalizeControlSocketResources(int* socketfd, struct control_port* cp_head);

// ====================== function for compute interfaces ====================== //
compute_interface *addComputeInteface(compute_interface *ci_head, char *new_port_name);
compute_interface *buildComputeInterface(char *new_port_name);
compute_interface *setComputeInterfaces(struct ifaddrs *ifaddr, char *computeSubnetIntfName, bool isLeaf);
compute_interface *freeComputeInterfaces(compute_interface *interface);
void initalizeComputeSocketResources(int *socketfd, compute_interface *ci_head);
void printComputeInterfaces(compute_interface *head);

// ====================== function for VID ====================== //
struct VID* add_to_VID_table(struct VID* VID_head, char* VID_name);
struct VID* build_VID(char* VID_name);
struct VID* find_VID_by_name(struct VID* VID_head,char* VID_name);
struct VID* remove_VID_by_name(struct VID* VID_head,char* VID_name);
struct VID* clear_VID_table(struct VID* node);
size_t get_all_VIDs(struct VID* VID_head, char** store_array);
struct VID* convert_VID_array_to_VID_table( char** VID_array, uint16_t VID_array_size);
struct VID* copy_VID_table(struct VID* VID_head);
void copy_VID_prefix(char *dest, char* src);
void print_VID_table(struct VID* VID_head);


// ====================== function for unreachable ====================== //
struct unreachable_table* add_to_unreachable_table(struct unreachable_table* ut, char* new_VID_name);
struct unreachable_table* build_unreachable_table();
struct VID* find_unreachable_VID_by_name(struct unreachable_table* ut, char* VID_name);
struct unreachable_table* remove_unreachable_VID_by_name(struct unreachable_table* ut, char* VID_name);
void print_unreachable_table(struct unreachable_table* ut);


// ====================== function for reachable ====================== //
struct reachable_table* add_to_reachable_table(struct reachable_table* rt, char* new_VID_name);
struct reachable_table* build_reachable_table();
struct VID* find_reachable_VID_by_name(struct reachable_table* rt, char* VID_name);
struct reachable_table* remove_reachable_VID_by_name(struct reachable_table* rt, char* VID_name);
void print_reachable_table(struct reachable_table* ut);


// ====================== function for VID_offered_table ====================== //
struct VID_offered_port* add_to_offered_table(struct VID_offered_port* vop_head, char* new_port_name, char* VID_name);
struct VID_offered_port* build_VID_offered_port(char* new_port_name);
struct VID_offered_port* find_offered_port_by_name(struct VID_offered_port* vop_head,char *port_name);
struct VID_offered_port* find_offered_port_by_VID(struct VID_offered_port* vop_head, char *VID_name);

size_t count_available_offered_port(struct VID_offered_port* vop_head, char** store_array, char* dest_VID);
size_t get_offered_VIDs_by_port_name(struct VID_offered_port* vop_head, char* port_name, char **store_array);
size_t get_reachable_VIDs_from_offered_ports(struct VID_offered_port* vop_head, char** store_array);
size_t get_unreachable_VIDs_from_offered_ports(struct VID_offered_port* vop_head, char** store_array);
int is_unreachable_and_reachable_empty(struct VID_offered_port* vop_head);
int is_all_offered_ports_down(struct VID_offered_port* vop_head);

void print_offered_table(struct VID_offered_port* vop_head);


// ====================== function for VID_accepted_table ====================== //
struct VID_accepted_port* add_to_accepted_table(struct VID_accepted_port* vap_head, char* new_port_name, char* VID_name);
struct VID_accepted_port* build_VID_accepted_port(char* new_port_name);
struct VID_accepted_port* find_accepted_port_by_name(struct VID_accepted_port* vap_head, char *port_name);
struct VID_accepted_port* find_accepted_port_by_VID(struct VID_accepted_port* vap_head, char *VID_name);

size_t get_accepted_VIDs_by_port_name(struct VID_accepted_port* vap_head, char* port_name, char **store_array);
size_t get_all_accepted_VIDs(struct VID_accepted_port* vap_head, char **store_array);

void print_accepted_table(struct VID_accepted_port* vap_head);


// ====================== function for port_recover_notification ====================== //
// those function for handling multiple failures occur in a same time, just leaf it alone
struct notification* add_to_notification_table(struct notification* ntf_head, char* new_from_port_name, int table_option, int operation_option);
struct notification* build_notification(char* new_from_port_name, int table_option, int operation_option);
struct notification* find_notification_by_from_port_name(struct notification* ntf_head, char* from_port_name);
struct notification* remove_notification_by_from_port_name(struct notification* ntf_head, char* from_port_name);
void print_notification_table(struct notification* ntf_head);

#endif