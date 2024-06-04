
#include "mtp_struct.h"

// ============================================ function for control_port ============================================ //
struct control_port* add_to_control_port_table(struct control_port* cp_head, char* new_port_name){
    if(cp_head == NULL){
        cp_head = build_control_port(new_port_name);
    }else{
        struct control_port* cp_temp = cp_head;

        if(!strcmp(cp_head->port_name,new_port_name)){
            return cp_head;
        }

        while(cp_temp->next){
            if(!strcmp(cp_temp->next->port_name,new_port_name)){
                return cp_head;
            }
            cp_temp = cp_temp->next;
        }
        cp_temp->next = build_control_port(new_port_name);
    }
    return cp_head;
}

struct control_port* build_control_port(char* new_port_name){
    struct control_port* new_node = (struct control_port*) malloc(sizeof(struct control_port));
    strcpy(new_node->port_name, new_port_name);
    new_node->isUP = 0;
    new_node->start = 0;
    new_node->fail_type = 0;
    new_node->last_received_time = 0LL;
    new_node->last_sent_time = 0LL;
    new_node->ntf_head = NULL;
    new_node->next = NULL;
    new_node->continue_count = 3;
    return new_node;
}


struct control_port* find_control_port_by_name(struct control_port* cp_head, char* port_name){
    struct control_port* cp_temp = cp_head;
    while(cp_temp){
        if(!strcmp(cp_temp->port_name, port_name)){
            return cp_temp;
        }
        cp_temp = cp_temp->next;
    }
    return NULL;
}

struct control_port* remove_control_port_by_name(struct control_port* cp_head, char* port_name){
    struct control_port* cp_temp = cp_head;
    if(!strcmp(cp_head->port_name, port_name)){
        cp_head = cp_head->next;
        free(cp_temp);
    }else{
        while(cp_temp->next){
            if(!strcmp(cp_temp->next->port_name,port_name)){
                struct control_port* dummy = cp_temp->next;
                cp_temp->next = dummy->next;
                free(dummy);
                break;
            }
            cp_temp = cp_temp->next;
        }
    }
    return cp_head;
}

struct control_port* clear_control_port(struct control_port* node){
    if(node == NULL) return NULL;
    clear_control_port(node->next);
    free(node);
    return NULL;
}

int is_all_down(struct control_port* cp_head){
    for(struct control_port* cp_temp = cp_head;cp_temp;cp_temp = cp_temp->next){
        if(cp_temp->isUP) return 0;
    }
    return 1;
}


void print_control_port_table(struct control_port* cp_head){
    struct control_port* cp_temp = cp_head;
    printf("--- Printing control port table ---\n");
    while(cp_temp){
        printf("%s\n",cp_temp->port_name);
        cp_temp = cp_temp->next;
    }
}

// ============================================ function for compute interfaces ============================================ //
compute_interface* addComputeInteface(compute_interface* ci_head, char* new_port_name)
{
    // If there are no compute interfaces added, add it as the first one (head).
    if(ci_head == NULL)
    {
        ci_head = buildComputeInterface(new_port_name);
    }

    else
    {
        compute_interface* ci_temp = ci_head;

        // If the new interface name is the same as the head interface name, nothing to do.
        if(!strcmp(ci_head->port_name, new_port_name))
        {
            return ci_head;
        }

        // If the new interface is the same as a non-head interface name, nothing to do. Loop through the names to check.
        while(ci_temp->next)
        {
            if(!strcmp(ci_temp->next->port_name, new_port_name))
            {
                return ci_head;
            }

            ci_temp = ci_temp->next;
        }

        ci_temp->next = buildComputeInterface(new_port_name);
    }

    return ci_head;
}

compute_interface* buildComputeInterface(char* new_port_name)
{
    compute_interface *new_node = malloc(sizeof(compute_interface));

    strcpy(new_node->port_name, new_port_name);
    new_node->next = NULL;

    return new_node;
}

void printComputeInterfaces(compute_interface *head)
{
    compute_interface *ci_temp = head;

    printf("--- Compute Interfaces ---\n");
    
    while(ci_temp)
    {
        printf("%s\n",ci_temp->port_name);
        ci_temp = ci_temp->next;
    }
}

compute_interface* freeComputeInterfaces(compute_interface *interface)
{
    if(interface == NULL)
    {
        return NULL;
    } 

    freeComputeInterfaces(interface->next);
    free(interface);

    return NULL;
}

// ============================================ function for VID ============================================ //
struct VID* add_to_VID_table(struct VID* VID_head, char* VID_name){
    if(VID_head == NULL){
        VID_head = build_VID(VID_name);
    }else{
        struct VID* VID_temp = VID_head;
        if(!strcmp(VID_head->VID_name,VID_name)){
            return VID_head;
        }

        while(VID_temp->next){
            if(!strcmp(VID_temp->next->VID_name,VID_name)){
                return VID_head;
            }
            VID_temp = VID_temp->next;
        }
        VID_temp->next = build_VID(VID_name);
    }
    return VID_head;
}


struct VID* build_VID(char* VID_name){
    struct VID *new_node = (struct VID*)malloc(sizeof(struct VID));
    strcpy(new_node->VID_name, VID_name);
    new_node->next = NULL;
    return new_node;
}


struct VID* find_VID_by_name(struct VID* VID_head, char* VID_name){
    struct VID* VID_temp = VID_head;
    while(VID_temp){
        if(!strcmp(VID_temp->VID_name, VID_name)){
            return VID_temp;
        }
        VID_temp = VID_temp->next;
    }
    return NULL;
}

struct VID* remove_VID_by_name(struct VID* VID_head,char* VID_name){
    if(VID_head == NULL) return NULL;
    struct VID* VID_temp = VID_head;
    if(!strcmp(VID_head->VID_name, VID_name)){
        VID_head = VID_head->next;
        free(VID_temp);
    }else{
        while(VID_temp->next){
            if(!strcmp(VID_temp->next->VID_name, VID_name)){
                struct VID* dummy = VID_temp->next;
                VID_temp->next = dummy->next;
                free(dummy);
                break;
            }
            VID_temp = VID_temp->next;
        }
    }
    return VID_head;
}

struct VID* clear_VID_table(struct VID* node){
    if(node == NULL) return NULL;
    clear_VID_table(node->next);
    free(node);
    return NULL;
}

size_t get_all_VIDs(struct VID* VID_head, char** store_array){
    struct VID* VID_temp = VID_head;
    size_t counter = 0;
    while(VID_temp){
        if(store_array != NULL){
            copy_VID_prefix(store_array[counter],VID_temp->VID_name);
        }
        counter++;
        VID_temp = VID_temp->next;
    }
    return counter;
}

struct VID* convert_VID_array_to_VID_table( char** VID_array, uint16_t VID_array_size){
    struct VID* VID_head = NULL;
    for(uint16_t k = 0;k < VID_array_size;k++){
        VID_head = add_to_VID_table(VID_head, VID_array[k]);
    }
    return VID_head;
}

struct VID* copy_VID_table(struct VID* VID_head){
    struct VID* VID_new_head = NULL;
    for(struct VID* VID_temp = VID_head;VID_temp;VID_temp = VID_temp->next){
        VID_new_head = add_to_VID_table(VID_new_head,VID_temp->VID_name);
    }
    return VID_new_head;
}

void copy_VID_prefix(char *dest, char* src){
    size_t len = 0;
    while(src[len] != '.' && src[len] != '\0') len++;
    strncpy(dest,src,len);
    dest[len] = '\0';
}

void print_VID_table(struct VID* VID_head){
    struct VID* VID_temp = VID_head;
    while(VID_temp){
        printf("%s\n",VID_temp->VID_name);
        VID_temp = VID_temp->next;
    }
}


// ============================================ function for unreachable ============================================ //
struct unreachable_table* add_to_unreachable_table(struct unreachable_table* ut, char* new_VID_name){
    if(ut == NULL){
        ut = build_unreachable_table();
        ut->VID_head = build_VID(new_VID_name);
    }else{
        ut->VID_head = add_to_VID_table(ut->VID_head,new_VID_name);
    }
    return ut;
}


struct unreachable_table* build_unreachable_table(){
    struct unreachable_table* new_node = (struct unreachable_table*) malloc(sizeof(struct unreachable_table));
    new_node->VID_head = NULL;
    return new_node;
}


struct VID* find_unreachable_VID_by_name(struct unreachable_table* ut, char* VID_name){
    return find_VID_by_name(ut->VID_head, VID_name);
}


struct unreachable_table* remove_unreachable_VID_by_name(struct unreachable_table* ut, char* VID_name){
    ut->VID_head = remove_VID_by_name(ut->VID_head, VID_name);
    return ut;
}


void print_unreachable_table(struct unreachable_table* ut){
    print_VID_table(ut->VID_head);
}


// ============================================ function for reachable ============================================ //
struct reachable_table* add_to_reachable_table(struct reachable_table* rt, char* new_VID_name){
    if(rt == NULL){
        rt = build_reachable_table();
        rt->VID_head = build_VID(new_VID_name);
    }else{
        rt->VID_head = add_to_VID_table(rt->VID_head,new_VID_name);
    }
    return rt;
}

struct reachable_table* build_reachable_table(){
    struct reachable_table* new_node = (struct reachable_table*) malloc(sizeof(struct reachable_table));
    new_node->VID_head = NULL;
    return new_node;
}


struct VID* find_reachable_VID_by_name(struct reachable_table* rt, char* VID_name){
    return find_VID_by_name(rt->VID_head, VID_name);
}


struct reachable_table* remove_reachable_VID_by_name(struct reachable_table* rt, char* VID_name){
    rt->VID_head = remove_VID_by_name(rt->VID_head, VID_name);
    return rt;
}


void print_reachable_table(struct reachable_table* rt){
    print_VID_table(rt->VID_head);
}


// ============================================ function for VID_offered_table ============================================ //
struct VID_offered_port* add_to_offered_table(struct VID_offered_port* vop_head, char* new_port_name, char* VID_name){
    if(vop_head == NULL){ // init first node
        vop_head = build_VID_offered_port(new_port_name);
        vop_head->VID_head = add_to_VID_table(vop_head->VID_head, VID_name);
    }else{
        int flag = 0;
        struct VID_offered_port* vop_temp = vop_head;
        while(1) { // check existence of port name
            if(!strcmp(vop_temp->port_name,new_port_name)){
                flag = 1;
                break;
            }
            if(vop_temp->next == NULL){
                break;
            }
            vop_temp = vop_temp->next;
        }

        if(!flag){ // doesn't exist, append new node to the end
            vop_temp->next = build_VID_offered_port(new_port_name);
            vop_temp = vop_temp->next;
            vop_temp->VID_head = add_to_VID_table(vop_temp->VID_head, VID_name);
        }else{ // does exist, add the vid to the target port
            vop_temp->VID_head = add_to_VID_table(vop_temp->VID_head, VID_name);
        }
    }
    return vop_head;
}

struct VID_offered_port* build_VID_offered_port(char* new_port_name){
    struct VID_offered_port* new_node = (struct VID_offered_port*)malloc(sizeof(struct VID_offered_port));
    strcpy(new_node->port_name, new_port_name);
    new_node->next = NULL;
    new_node->VID_head = NULL;
    new_node->cp = NULL;
    new_node->ut = build_unreachable_table();
    new_node->rt = build_reachable_table();
    return new_node;
}

struct VID_offered_port* find_offered_port_by_name(struct VID_offered_port* vop_head, char *port_name){
    struct VID_offered_port* vop_temp = vop_head;
    while(vop_temp){
        if(!strcmp(vop_temp->port_name, port_name)){
            return vop_temp;
        }
        vop_temp = vop_temp->next;
    }
    return NULL;
}

// struct VID_offered_port* find_offered_port_by_VID(char *VID_name){

// }

size_t count_available_offered_port(struct VID_offered_port* vop_head, char** store_array, char* dest_VID){
    struct VID_offered_port* vop_temp = vop_head;
    size_t counter = 0;
    for(;vop_temp;vop_temp = vop_temp->next){
        // the eth port must be up, and the destination VID doesn't exist in the unreachable table of this port
        if(vop_temp->cp->isUP){
            int check = 0;
            if(vop_temp->rt->VID_head != NULL){
                if(find_reachable_VID_by_name(vop_temp->rt, dest_VID)) {
                    check = 1;
                }
            }else if(vop_temp->ut->VID_head != NULL){
                if(find_unreachable_VID_by_name(vop_temp->ut, dest_VID) == NULL){
                    check = 1;
                }
            }else{
                check = 1;
            }

            if(check){
                if(store_array != NULL){
                    strcpy(store_array[counter],vop_temp->port_name);
                }
                counter++;
            }
        }  
    }
    return counter;
}


size_t get_offered_VIDs_by_port_name(struct VID_offered_port* vop_head, char* port_name, char **store_array){
    struct VID* VID_temp = find_offered_port_by_name(vop_head, port_name)->VID_head;
    return get_all_VIDs(VID_temp, store_array);
}


size_t get_reachable_VIDs_from_offered_ports(struct VID_offered_port* vop_head, char** store_array){
    struct VID_offered_port* vop_temp = vop_head;
    size_t counter = 0;
    for(;vop_temp;vop_temp = vop_temp->next){
        if(vop_temp->cp->isUP && vop_temp->rt != NULL){
            counter += get_all_VIDs(vop_temp->rt->VID_head, store_array + counter);
        }
    }
    return counter;
}


size_t get_unreachable_VIDs_from_offered_ports(struct VID_offered_port* vop_head, char** store_array){
    struct VID_offered_port* vop_temp = vop_head;
    size_t counter = 0;
    for(;vop_temp;vop_temp = vop_temp->next){
        if(vop_temp->cp->isUP && vop_temp->ut != NULL){
            counter += get_all_VIDs(vop_temp->ut->VID_head, store_array + counter);
        }
    }
    return counter;
}


int is_unreachable_and_reachable_empty(struct VID_offered_port* vop_head){
    struct VID_offered_port* vop_temp = vop_head;
    for(;vop_temp;vop_temp = vop_temp->next){
        if(vop_temp->ut->VID_head == NULL && vop_temp->rt->VID_head == NULL) return 1;
    }
    return 0;
}

int is_all_offered_ports_down(struct VID_offered_port* vop_head){
    struct VID_offered_port* vop_temp = vop_head;
    for(;vop_temp;vop_temp = vop_temp->next){
        if(vop_temp->cp->isUP) return 0;
    }
    return 1;
}


void print_offered_table(struct VID_offered_port* vop_head){
    struct VID_offered_port* vop_temp = vop_head;
    printf("--- Printing VID offered table ---\n");
    while(vop_temp){
        printf("Port %s has VIDs => \n",vop_temp->port_name);
        print_VID_table(vop_temp->VID_head);
        vop_temp = vop_temp->next;
    }
}


// ============================================ function for VID_accepted_table ============================================ //
struct VID_accepted_port* add_to_accepted_table(struct VID_accepted_port* vap_head, char* new_port_name, char* VID_name){
    if(vap_head == NULL){ // init first node
        vap_head = build_VID_accepted_port(new_port_name);
        vap_head->VID_head = add_to_VID_table(vap_head->VID_head, VID_name);
    }else{
        int flag = 0;
        struct VID_accepted_port* vap_temp = vap_head;
        while(1) { // check existence of port name
            if(!strcmp(vap_temp->port_name,new_port_name)){
                flag = 1;
                break;
            }
            if(vap_temp->next == NULL){
                break;
            }
            vap_temp = vap_temp->next;
        }

        if(!flag){ // doesn't exist, append new node to the end
            vap_temp->next = build_VID_accepted_port(new_port_name);
            vap_temp = vap_temp->next;
            vap_temp->VID_head = add_to_VID_table(vap_temp->VID_head, VID_name);
        }else{ // does exist, add the vid to the target port
            vap_temp->VID_head = add_to_VID_table(vap_temp->VID_head, VID_name);
        }
    }
    return vap_head;
}


struct VID_accepted_port* build_VID_accepted_port(char* new_port_name){
    struct VID_accepted_port* new_node = (struct VID_accepted_port*)malloc(sizeof(struct VID_accepted_port));
    strcpy(new_node->port_name, new_port_name);
    new_node->next = NULL;
    new_node->VID_head = NULL;
    new_node->cp = NULL;
    new_node->ut = build_unreachable_table();
    new_node->rt = build_reachable_table();
    return new_node;
}


struct VID_accepted_port* find_accepted_port_by_name(struct VID_accepted_port* vap_head, char *port_name){
    struct VID_accepted_port* vap_temp = vap_head;
    while(vap_temp){
        if(!strcmp(vap_temp->port_name, port_name)){
            return vap_temp;
        }
        vap_temp = vap_temp->next;
    }
    return NULL;
}

struct VID_accepted_port* find_accepted_port_by_VID(struct VID_accepted_port* vap_head, char *VID_name){

    size_t dest_VID_len = strlen(VID_name);

    struct VID_accepted_port* vap_temp = vap_head;
    while(vap_temp){ // iterate VID_Accepted_Table
        struct VID* VID_temp = vap_temp->VID_head;
        while(VID_temp){ // iterate VID table
            int i = 0;
            while( 1 ){
                if(VID_temp->VID_name[i] != VID_name[i]) break;
                else i++;
                if(i == dest_VID_len && VID_temp->VID_name[i] == '.') return vap_temp;
                if(i == dest_VID_len || VID_temp->VID_name[i] == '.') break;
            }
            VID_temp = VID_temp->next;
        }
        
        vap_temp = vap_temp->next;
    }

    return NULL; // return NULL if VID doesn't exist
}


size_t get_accepted_VIDs_by_port_name(struct VID_accepted_port* vap_head, char* port_name, char **store_array){
    struct VID* VID_temp = find_accepted_port_by_name(vap_head, port_name)->VID_head;
    return get_all_VIDs(VID_temp, store_array);
}

size_t get_all_accepted_VIDs(struct VID_accepted_port* vap_head, char **store_array){
    size_t counter = 0;
    for(struct VID_accepted_port* vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){
        if(vap_temp->cp->isUP){
            counter += get_all_VIDs(vap_temp->VID_head, store_array + counter);
        }
    }
    return counter;
}


void print_accepted_table(struct VID_accepted_port* vap_head){
    struct VID_accepted_port* vap_temp = vap_head;
    printf("--- Printing VID accepted table ---\n");
    while(vap_temp){
        printf("Port %s has VIDs =>\n",vap_temp->port_name);
        print_VID_table(vap_temp->VID_head);
        vap_temp = vap_temp->next;
    }
}


// ============================================ function for port_recover_notification ============================================ //
struct notification* add_to_notification_table(struct notification* ntf_head, char* new_from_port_name, int table_option, int operation_option){
    if(ntf_head == NULL){
        ntf_head = build_notification(new_from_port_name, table_option, operation_option);
    }else{
        struct notification* ntf_temp = ntf_head;
        while(ntf_temp->next){
            ntf_temp = ntf_temp->next;
        }
        ntf_temp->next = build_notification(new_from_port_name, table_option, operation_option);
    }
    return ntf_head;
}


struct notification* build_notification(char* new_from_port_name, int table_option, int operation_option){
    struct notification* new_node = (struct notification*)malloc(sizeof(struct notification));
    strcpy(new_node->from_port_name,new_from_port_name);
    new_node->table_option = table_option;
    new_node->operation_option = operation_option;
    new_node->next = NULL;
    return new_node;
}


struct notification* find_notification_by_from_port_name(struct notification* ntf_head,char* from_port_name){
    struct notification* ntf_temp = ntf_head;
    while(ntf_temp){
        if(!strcmp(ntf_temp->from_port_name, from_port_name)){
            return ntf_temp;
        }
        ntf_temp = ntf_temp->next;
    }
    return NULL;
}


struct notification* remove_notification_by_from_port_name(struct notification* ntf_head, char* from_port_name){
    struct notification* ntf_temp = ntf_head;
    if(!strcmp(ntf_head->from_port_name, from_port_name)){
        ntf_head = ntf_head->next;
        free(ntf_temp);
    }else{
        while(ntf_temp->next){
            if(!strcmp(ntf_temp->next->from_port_name, from_port_name)){
                struct notification* dummy = ntf_temp->next;
                ntf_temp->next = dummy->next;
                free(dummy);
                break;
            }
            ntf_temp = ntf_temp->next;
        }
    }
    return ntf_head;
}


void print_notification_table(struct notification* ntf_head){
    struct notification* ntf_temp = ntf_head;
    while(ntf_temp){
        printf("from port name: %s\n",ntf_temp->from_port_name);
        printf("table option: %d\n",ntf_temp->table_option);
        printf("operation option: %d\n",ntf_temp->operation_option);
        ntf_temp = ntf_temp->next;
    }
}