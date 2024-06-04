/*
 * File: mtp_build.c 
 *
 * This file includes functions that build that are used in MTP operation.
 * 
 * Nicolas Suarez 6/1/2021
 * 
*/

#include "mtp_build.h"

unsigned char payload[MAX_BUFFER_SIZE];

/* -----------------------------------------------------------------------------------------
 *                              End of build Functions
 * -----------------------------------------------------------------------------------------
 */ 

size_t build_VID_data_payload(unsigned char *payload, char** VID_array, uint16_t VID_array_size){
    size_t size_count = 0;
    for(uint16_t i = 0; i < VID_array_size;i++){
        uint8_t vid_len = (uint8_t)strlen(VID_array[i]);
        payload[size_count] = vid_len;
        size_count++;
        memcpy(payload + size_count,VID_array[i],vid_len);
        size_count += vid_len;
    }
    return size_count;
}


/*
 *
 * Sends and MTP message of type 2 (with no response)
 * This is a hello message that includes the senders VIDs
 * 
 */
void send_hello_NR(char *current_port_name, char **VID_array, uint16_t VID_array_size){ // VID should be a VID table pointer
    
    printf("\nSending HelloNR message on port %s\n",current_port_name);

    payload[0] = MTP_TYPE_HELLONR_MSG;
    memcpy(payload + 1,&VID_array_size,2);

    int payload_len = 3 + build_VID_data_payload(payload + 3, VID_array, VID_array_size);

    if(payload_len){
        dataSend(current_port_name, payload_len, payload);
    }
}

/*
 *
 * Sends an MTP message of Join Request (3)
 * 
 */
void send_join_req( char *current_port_name, char **VID_array, uint16_t VID_array_size ){ // Vincent double pointer as I may ask for more than one VID 

    printf("\nSending JoinReq on port %s...\n", current_port_name);
    
    payload[0] = MTP_TYPE_JOIN_REQ;
    memcpy(payload + 1,&VID_array_size,2);

    int payload_len = 3 + build_VID_data_payload(payload + 3, VID_array, VID_array_size);
    
 
    if(payload_len){
        dataSend(current_port_name, payload_len, payload);
    }
}


/*
 *
 * Sends an MTP message of Join Response (4)
 * 
 */
void send_join_res( char *current_port_name, char **VID_array, uint16_t VID_array_size ) {

    printf("\nSending JoinRes out %s...\n", current_port_name);

    payload[0] = MTP_TYPE_JOIN_RES;
    memcpy(payload + 1,&VID_array_size,2);

    int payload_len = 3 + build_VID_data_payload(payload + 3, VID_array, VID_array_size);

    if(payload_len){
        dataSend(current_port_name, payload_len, payload);
    }
}


void send_join_ack(char *current_port_name,char **VID_array, uint16_t VID_array_size){
    printf("\nSending Join ACK out on %s...\n",current_port_name);

    payload[0] = MTP_TYPE_JOIN_ACK;
    memcpy(payload + 1,&VID_array_size,2);

    int payload_len = 3 + build_VID_data_payload(payload + 3, VID_array, VID_array_size);


    if(payload_len){
        dataSend(current_port_name,payload_len,payload);
    }
}

void send_start_hello(char *current_port_name){
    // printf("\nSending start hello out on %s...\n",current_port_name);
    payload[0] = MTP_TYPE_START_HELLO;
    dataSend(current_port_name,1,payload);
}



/*
 * Sends an MTP message of type 9 (Data)
 *
 */
void route_data_from_tor_to_spine(char *current_port_name, uint16_t src_VID, uint16_t dest_VID,unsigned char *ip_header_with_payload,  unsigned int ip_header_and_payload_len){
    printf("Sending Data from tor to spine on %s...\n", current_port_name);

    payload[0] = MTP_TYPE_DATA_MSG;
    memcpy(payload + 1, &src_VID, 2);
    memcpy(payload + 3, &dest_VID, 2);
    memcpy(payload + 5,ip_header_with_payload,ip_header_and_payload_len);
    int payload_len = 5 + ip_header_and_payload_len;

    if(payload_len){
        dataSend(current_port_name, payload_len, payload); 
    }
}

void route_data_from_spine(char *current_port_name,unsigned char *payload, unsigned int payload_len){
    printf("Sending data message from spine on %s \n",current_port_name);
    if(payload_len){
        dataSend(current_port_name, payload_len, payload);
    }
}

void route_data_to_server(char *computePortName, unsigned char *payload, unsigned int payload_len)
{
    printf("Sending data message from tor to server on %s \n",computePortName);

    if(payload_len)
    {
        computeSend(computePortName, payload_len, payload);
    }
}

int send_keep_alive(char *current_port_name)
{
    payload[0] = MTP_TYPE_KEEP_ALIVE;
    return dataSend(current_port_name,1,payload);
}


int send_failure_update(char *current_port_name, char **VID_array, uint16_t VID_array_size, uint8_t update_option){
    printf("Sending failure update message out from port %s\n",current_port_name);

    payload[0] = MTP_TYPE_FAILURE_UPDATE;
    payload[1] = update_option;
    memcpy(payload + 2,&VID_array_size,2);
    int payload_len = 4 + build_VID_data_payload(payload + 4, VID_array, VID_array_size);

    if(payload_len){
        dataSend(current_port_name, payload_len, payload);
    }
}

int send_recover_update(char *current_port_name, char** VID_array, uint16_t VID_array_size, uint8_t update_option){
    printf("Sending recover update message out from port %s\n",current_port_name);
    payload[0] = MTP_TYPE_RECOVER_UPDATE;
    payload[1] = update_option;
    memcpy(payload + 2,&VID_array_size,2);
    int payload_len = 4 + build_VID_data_payload(payload + 4, VID_array, VID_array_size);
    
    if(payload_len){
        dataSend(current_port_name, payload_len, payload);
    }
}


