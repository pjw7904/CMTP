/*
 * Standard library imports.
 */
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

/*
 * Custom MTP imports.
 */
#include "mtp_utils.h"
#include "mtp_struct.h"
#include "mtp_build.h"
#include "config.h"

/*
 * Configuration variables
 */ 
Config mtpConfig = {0};
uint8_t VID_octet = 3; // Third octet in IPv4 address, example: octet value C in address A.B.C.D
char my_VID[VID_LEN] = {'\0'};

/*
 * Control Ports (MTP-speaking interfaces)
 */
struct control_port *cp_head = NULL;
struct control_port *cp_temp = NULL;

/*
 * Compute Ports (IPv4-speaking interfaces)
 */
compute_interface *compute_intf_head = NULL;
compute_interface *compute_intf_temp = NULL;

/*
 * Offered Ports (interfaces sending VIDs to upper tier devices).
 */
struct VID_offered_port* vop_head = NULL;
struct VID_offered_port* vop_temp = NULL;

/*
 * Accepted Ports (interfaces receiving VIDs from lower tier devices).
 */
struct VID_accepted_port* vap_head = NULL;
struct VID_accepted_port* vap_temp = NULL;

/*
 * Current clock time.
 */
struct timeval current_time;

/*
 * Temporary storage for VIDs and interface names
*/
char **temp_2d_array;
char **temp_2d_port_array;

extern int socketfd;

/*
 * Function prototypes to handle each type of MTP message that can be received.
*/
void handle_receive_hello_NR(unsigned char* recvBuffer_MTP,char* recvOnEtherPort);
void handle_receive_join_req(unsigned char* recvBuffer_MTP,char* recvOnEtherPort);
void handle_receive_join_res(unsigned char* recvBuffer_MTP,char* recvOnEtherPort);
void handle_receive_join_ack(unsigned char* recvBuffer_MTP,char* recvOnEtherPort);
void handle_receive_start_hello(char* recvOnEtherPort);
void handle_receive_data_msg(unsigned char* recvBuffer_MTP,char* recvOnEtherPort, socklen_t recv_len_MTP);
void handle_receive_keep_alive(char* recvOnEtherPort);
void handle_receive_failure_update(unsigned char* recvBuffer_MTP,char* recvOnEtherPort, socklen_t recv_len_MTP);
void handle_receive_recover_update(unsigned char* recvBuffer_MTP,char* recvOnEtherPort);
void handle_receive_from_server(unsigned char* recvBuffer_IP,char* recvOnEtherPort, socklen_t recv_len_IP);

/*
 * Function prototype to handle SIGINT (SIGINT) and stopping MTP.
*/
void handleSIGINT(int sig);

int main(int argc, char **argv)
{
    // Set up a SIGINT (CTRL + C) handler to gracefully stop running MTP.
    if(signal(SIGINT, handleSIGINT) == SIG_ERR)
    {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    /*
        START-UP STAGE
        -----------------------------------------------------------------------------
        When the protocol starts up, process the inital configuration via the configuration file
        and by defining MTP-speaking interfaces on the device. If the local system is a leaf node,
        find the compute subnet interface IPv4 address and use that to generate its root VID.

        Sockets and other necessary memory will be defined as well.
        -----------------------------------------------------------------------------
    */
    readConfigurationFile(&mtpConfig);

    // Use ifaddrs structure to loop through network interfaces on the system.
    struct ifaddrs *ifaddr;
    if(getifaddrs(&ifaddr) == -1) 
    {
        perror("\nGetting network interfaces failed (getifaddrs).\n");
        exit(EXIT_FAILURE);
    }

    // Find if a compute interface exists on the node and then find the control (MTP) interfaces.
    compute_intf_head = setComputeInterfaces(ifaddr, mtpConfig.computeIntfName, mtpConfig.isLeaf);
    cp_head = setControlInterfaces(ifaddr, mtpConfig.computeIntfName, mtpConfig.isLeaf);
    freeifaddrs(ifaddr); // Free the interface memory.

    printf("===MTP START-UP CONFIG===\ntier = %d\nisTopSpine = %d\nisLeaf = %d\ncomputeIntfName = %s\n", 
            mtpConfig.tier, mtpConfig.isTopSpine, mtpConfig.isLeaf, mtpConfig.computeIntfName);

    // Leaf nodes are the root of the trees, they define the starting (root) VID.
    if(mtpConfig.isLeaf)
    {
        getRootVID(my_VID, mtpConfig.computeIntfName, VID_octet);
        printf("Root VID: %s\n\n", my_VID);
    }

    else
    {
        printf("Root VID: None\n\n"); 
    }

    // TO-DO: Ask Vincent about the magic number 32, and why is the port array being given VID_LEN?
    // Initalize an array to add VIDs to as necessary (this also is used for ports sometimes?).
    temp_2d_array = malloc(32 * sizeof(char*));
    for(int j = 0; j < 10; j++)
    {
        temp_2d_array[j] = malloc(VID_LEN);
        memset(temp_2d_array[j], '\0', VID_LEN);
    }

    // Initalize an array to add interface names to as necessary.
    temp_2d_port_array = malloc(32 * sizeof(char*));
    for(int j = 0; j < 10; j++)
    {
        temp_2d_port_array[j] = malloc(VID_LEN);
        memset(temp_2d_port_array[j], '\0', VID_LEN);
    }

    // Create a socket for MTP messages from MTP-speaking (tier 1+) neighbors.
    int sockMTP = 0;
    if((sockMTP = socket(AF_PACKET, SOCK_RAW, htons (ETH_MTP_CTRL))) < 0)
    {
		perror("Error: MTP socket()");
		exit(1);
	}

    // Create socket for IPv4 packets from compute (tier 0) systems.
    int sockIP  = 0; 
    if((sockIP = socket(AF_PACKET, SOCK_RAW, htons (ETH_IP_CTRL))) < 0)
    {
		perror("Error: IP socket()");
		exit(1);
	}

    // init send socket
    if((socketfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("Socket Error");
        exit(1);
	}

    /*
        Builds the Ethernet II header used for each control interface when sending a message. 
        For each control interface, the header is generated as:
        [FF:FF:FF:FF:FF:FF] | [SMAC] | [0x8850]

        Where the destination MAC is set to FF:FF:FF:FF:FF:FF (the broadcast address),
        SMAC is the MAC address of the interface, and 0x8850 is the ethertype of MTP.
    */
    initalizeControlSocketResources(&socketfd, cp_head);

    // If this node is a spine, nothing to do right now, wait for a lower-tier device to annouce they can communicate via MTP.
    if(!mtpConfig.isLeaf)
    {
        if(mtpConfig.isTopSpine) 
        {
            printf("\nI am a top-tier Spine, waiting for hello message\n"); 
        }
        else 
        {
            printf("\nI am a Spine, waiting for hello message\n"); 
        }
    }

    // If this node is a leaf, start the VID propagation process by informing tier-2 spines it is ready to communicate.
    else
    {
        /*
            Initalize the resources for compute interfaces as well.
            The Ethernet II header results in the following:

            [FF:FF:FF:FF:FF:FF] | [SMAC] | [0x0800]

            Right now, the inital control and compute resource allocation is almost identical, but the
            compute function needs to be updated later to take advantage of ARP as opposed to all F's for the destination. 
            For now, it is kept as is for further testing.
        */
        initalizeComputeSocketResources(&socketfd, compute_intf_head);

        // Store the root VID in the tempoprary array to be sent in the initial messages.
        strcpy(temp_2d_array[0], my_VID);

        // Begin telling tier 2 spines about the existence of this leaf via Hello No-Response (Hello NR) messages 
        for(cp_temp = cp_head; cp_temp; cp_temp = cp_temp->next)
        { 
            send_hello_NR(cp_temp->port_name,temp_2d_array,1);
        }
    }

    char recvOnEtherPort[ETH_LEN] = {'\0'}; // Interface name a message is received on.
    uint16_t numOfVID = 0; // Used to count the number of VIDs on a given interface and table.

    // Prep MTP message socket.
    int recv_len_MTP = 0;
    unsigned char recvBuffer_MTP[MAX_BUFFER_SIZE] = { '\0' };
    struct sockaddr_ll src_addr_MTP; // sockaddr_ll - a structure device-independent physical-layer address
    socklen_t addr_len_MTP = sizeof(src_addr_MTP); //  socklen_t - an unsigned opaque integral type of length of at least 32 bits

    // Prep IPv4 packet socket.
    int recv_len_IP = 0;
    unsigned char recvBuffer_IP[MAX_BUFFER_SIZE] = { '\0' };
    struct sockaddr_ll src_addr_IP; // sockaddr_ll - a structure device-independent physical-layer address
    socklen_t addr_len_IP = sizeof(src_addr_IP); 

    // Receive and send messages until the MTP implementation is stopped.
    while(1)
    {
        recv_len_MTP = recvfrom(sockMTP, recvBuffer_MTP, MAX_BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr*) &src_addr_MTP, &addr_len_MTP); // listening MTP packet

        // If the incoming frame has data, parse and analyze it.
        if(recv_len_MTP > 0)
        {                                   
            unsigned int tcIP = src_addr_MTP.sll_ifindex;
            if_indextoname(tcIP, recvOnEtherPort);     // if_indextoname - map a network interface index to its corresponding name,built in fn.
                // change name new_node - new connection    

            // If the message comes on eth0, do not process.
            if ((strcmp(recvOnEtherPort, "eth0")) == 0)
            { 
                continue;
            }

            // Bytes 0-13 are the Ethernet II header, byte 14 starts the MTP header, whatever type it might be.
            uint8_t MSGType = recvBuffer_MTP[14];

            switch(MSGType)
            {
                case MTP_TYPE_HELLONR_MSG:
                    handle_receive_hello_NR(recvBuffer_MTP, recvOnEtherPort); // hello no response
                    break;
                case MTP_TYPE_JOIN_REQ:
                    handle_receive_join_req(recvBuffer_MTP, recvOnEtherPort); // join request
                    break; 
                case MTP_TYPE_JOIN_RES:
                    handle_receive_join_res(recvBuffer_MTP, recvOnEtherPort); // join response
                    break;                                 
                case MTP_TYPE_JOIN_ACK: 
                    handle_receive_join_ack(recvBuffer_MTP, recvOnEtherPort); // join acknowledge
                    break;
                case MTP_TYPE_START_HELLO:
                    handle_receive_start_hello(recvOnEtherPort);              // start sending hello
                    break;
                case MTP_TYPE_DATA_MSG:                                       // compute node data
                    handle_receive_data_msg(recvBuffer_MTP, recvOnEtherPort,recv_len_MTP); 
                    break;
                case MTP_TYPE_KEEP_ALIVE:
                    handle_receive_keep_alive(recvOnEtherPort);               // keep alive message
                    break;
                case MTP_TYPE_FAILURE_UPDATE: 
                    handle_receive_failure_update(recvBuffer_MTP, recvOnEtherPort,recv_len_MTP); // failure message
                    break;
                case MTP_TYPE_RECOVER_UPDATE:
                    handle_receive_recover_update(recvBuffer_MTP, recvOnEtherPort);              // recover message
                    break;
                default:
                    break;
            }
        }

        // Only the leaf listens to compute device IPv4 traffic.
        if(mtpConfig.isLeaf)
        {
            recv_len_IP = recvfrom(sockIP, recvBuffer_IP, MAX_BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr*) &src_addr_IP, &addr_len_IP);
            
            if(recv_len_IP > 0)
            {
                unsigned int tcIP = src_addr_IP.sll_ifindex;
                
                if_indextoname(tcIP, recvOnEtherPort); // Map a network interface index to its corresponding name.

                // If the message comes on eth0, do not process.
                if (!strcmp(recvOnEtherPort, "eth0"))
                { 
                    continue; 
                }
                
                handle_receive_from_server(recvBuffer_IP,recvOnEtherPort, recv_len_IP); // send data msg here
            }
        }

        // Send KEEP ALIVE and check the fail of the port
        uint8_t working_port_num = get_all_ethernet_interface2(temp_2d_port_array);
        for(cp_temp = cp_head;cp_temp;cp_temp = cp_temp->next)
        {
            if(!cp_temp->start) continue;

            if(cp_temp->last_received_time){ // run the code in this block after received first keep alive message

                int alive = check_port_is_alive(temp_2d_port_array,working_port_num,cp_temp->port_name); // immediate detect
                
                // port fail
                if(!alive && cp_temp->isUP)
                { 
                    // from up to down
                    cp_temp->isUP = 0;
                    cp_temp->fail_type = DETECT_FAIL;
                    cp_temp->continue_count = 0; // reset continue count
                    printf("Stop sending and receiving message due to immediate failure\n");
                    printf("Detected a failure, shut down port %s at time %lld\n",cp_temp->port_name,get_milli_sec(&current_time));

                    if(!mtpConfig.isTopSpine && is_all_offered_ports_down(vop_head)){
                        printf("All upstream ports down, sending all accepted VIDs from downstream ports\n");
                        numOfVID = get_all_accepted_VIDs(vap_head, temp_2d_array);
                        for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){
                            if(vap_temp->cp->isUP){
                                send_failure_update(vap_temp->port_name, temp_2d_array,numOfVID, REACHABLE_OPTION);
                            }
                        }
                    }else if(find_accepted_port_by_name(vap_head,cp_temp->port_name)){
                        printf("Failed on downstream port\n");
                        numOfVID = get_accepted_VIDs_by_port_name(vap_head, cp_temp->port_name, temp_2d_array);
                        for(struct control_port* cp_temp2 = cp_head;cp_temp2;cp_temp2 = cp_temp2->next){
                            if(cp_temp2->isUP){
                                send_failure_update(cp_temp2->port_name,temp_2d_array,numOfVID, UNREACHABLE_OPTION);                
                            }
                        }        
                    }else{ // upstream port
                        printf("Failed on upstream port\n");
                        if(!is_unreachable_and_reachable_empty(vop_head)){
                            printf("All upstream ports are not clean, sending blocked VID from downstream ports\n");
                            if((numOfVID = get_unreachable_VIDs_from_offered_ports(vop_head, temp_2d_array))){
                                for(vop_temp = vop_head;vop_temp;vop_temp = vop_temp->next){ // send black 
                                    if(vop_temp->cp->isUP){
                                        send_failure_update(vop_temp->port_name, temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                                    }
                                }
                            }
                        }else{
                            printf("Some upstream ports are clean, DONE\n");
                        }    
                    }
                }

                // port come back
                else if(alive && cp_temp->fail_type == DETECT_FAIL)
                { 
                    printf("\nPort %s is back at time %lld\n",cp_temp->port_name,get_milli_sec(&current_time));
                    cp_temp->fail_type = 0;
                }

                if(cp_temp->fail_type) continue;

                long long received_time_diff = get_milli_sec(&current_time) - cp_temp->last_received_time;

                if(received_time_diff >= DEAD_TIMER && cp_temp->isUP){ // check whether exceed dead timer
                    printf("Last receive time is %lld\n",cp_temp->last_received_time);
                    printf("--------Disabled for port %s due to a missing KEEP ALIVE at time %lld--------\n",cp_temp->port_name,get_milli_sec(&current_time));
                    cp_temp->continue_count = 0; // reset continue count
                    cp_temp->isUP = 0;
                    cp_temp->fail_type = MISS_FAIL;

                    printf("Stop sending and receiving message due to missing hello\n");

                    printf("Sending FAILURE UPDATE message from other working ports\n");
                    if(!mtpConfig.isTopSpine && is_all_offered_ports_down(vop_head)){
                        printf("All upstream ports down, sending all accepted VIDs from downstream ports\n");
                        numOfVID = get_all_accepted_VIDs(vap_head, temp_2d_array);
                        for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){
                            if(vap_temp->cp->isUP){
                                send_failure_update(vap_temp->port_name, temp_2d_array,numOfVID, REACHABLE_OPTION);
                            }
                        }
                    }else if(find_accepted_port_by_name(vap_head,cp_temp->port_name)){
                        printf("Failed on downstream port\n");
                        numOfVID = get_accepted_VIDs_by_port_name(vap_head, cp_temp->port_name, temp_2d_array);
                        for(struct control_port* cp_temp2 = cp_head;cp_temp2;cp_temp2 = cp_temp2->next){
                            if(cp_temp2->isUP){
                                send_failure_update(cp_temp2->port_name,temp_2d_array,numOfVID, UNREACHABLE_OPTION);                
                            }
                        }        
                    }else{ // upstream port
                        printf("Failed on upstream ports\n");
                        if(!is_unreachable_and_reachable_empty(vop_head)){
                            printf("All upstream ports are not clean, sending blocked VID from downstream ports\n");
                            if((numOfVID = get_unreachable_VIDs_from_offered_ports(vop_head, temp_2d_array))){
                                printf("Sending %d BLACK VID\n",numOfVID);
                                for(vop_temp = vop_head;vop_temp;vop_temp = vop_temp->next){ // send black 
                                    if(vop_temp->cp->isUP){
                                        send_failure_update(vop_temp->port_name, temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                                    }
                                }
                            }
                        }else{
                            printf("Some upstream ports are clean, DONE\n"); 
                        }   
                    }
                    continue;
                }
            }

            long long current_timestamp = get_milli_sec(&current_time);
            // send hello keep live
            if(current_timestamp - cp_temp->last_sent_time >= HELLO_TIMER){ // send keep alive message if condition met
                // printf("Send before time %lld\n",current_timestamp);
                if(send_keep_alive(cp_temp->port_name) != -1){ // send here
                    // printf("Sent KEEP ALIVE at time = %lld, update last sent time for port %s \n",get_milli_sec(&current_time),cp_temp->port_name);
                    cp_temp->last_sent_time = get_milli_sec(&current_time); // update send time
                }
            }
        } // End of control port status check for loop.
    } // End of infinite while loop.     
} // End of main function.


void handleSIGINT(int sig)
{
    long long current_timestamp = get_milli_sec(&current_time);

    printf("\nMTP STOPPED [%lld]\n",current_timestamp);
    // To-Do: Add memory-freeing calls here (control and compute interfaces, temp arrays, etc.)

    FILE* fptr;
    fptr = fopen("node_down.log", "w");

    // checking if the file is opened successfully
    if (fptr == NULL) 
    {
        printf("Stop time could not be written to log.\n");
        exit(0);
    }

    // Write implementation downtime to log file
    fprintf(fptr,"%lld\n", current_timestamp);
    fflush(fptr);
    fclose(fptr);

    exit(0);
}

void handle_receive_hello_NR(unsigned char* recvBuffer_MTP, char* recvOnEtherPort){
    if(get_tier_from_hello_message(recvBuffer_MTP + 15) >= mtpConfig.tier){ // break the case if the message from higher tier
        // printf("\nReceived HelloNR from higher tier, ignored!\n");
        return;
    }

    printf("\n Hello no response Received\n");

    uint16_t numOfVID = extract_VID_from_receive_buff(temp_2d_array,recvBuffer_MTP + 15,0);
    
    send_join_req( recvOnEtherPort, temp_2d_array,numOfVID); 
}


void handle_receive_join_req(unsigned char* recvBuffer_MTP,char* recvOnEtherPort){
    printf("\n Join Request Received\n");

    uint16_t numOfVID = extract_VID_from_receive_buff(temp_2d_array,recvBuffer_MTP + 15,0);

    //build the extension of VID
    for(uint16_t i = 0;i < numOfVID;i++){
        append_port_number_after_VID(recvOnEtherPort,temp_2d_array[i]);
    }

    send_join_res(recvOnEtherPort,temp_2d_array,numOfVID); 
}


void handle_receive_join_res(unsigned char* recvBuffer_MTP,char* recvOnEtherPort){
    printf("\n Join Response Received\n");
    
    uint16_t numOfVID = extract_VID_from_receive_buff(temp_2d_array,recvBuffer_MTP + 15,0);

    
    for(uint16_t i = 0;i < numOfVID;i++){ // add new VIDs to accepted table
        vap_head = add_to_accepted_table(vap_head, recvOnEtherPort, temp_2d_array[i]);
    }

    print_accepted_table(vap_head); // print the accepted table for debug

    vap_temp = find_accepted_port_by_name(vap_head, recvOnEtherPort);
    if(!vap_temp->cp){
        cp_temp = find_control_port_by_name(cp_head,recvOnEtherPort);
        vap_temp->cp = cp_temp;
    }

    if(!mtpConfig.isTopSpine){ // send helloNR from spine to next tier spine 
        for(cp_temp = cp_head;cp_temp;cp_temp = cp_temp->next){
            send_hello_NR(cp_temp->port_name, temp_2d_array, numOfVID);
        }
    }

    send_join_ack(recvOnEtherPort,temp_2d_array,numOfVID);
}


void handle_receive_join_ack(unsigned char* recvBuffer_MTP,char* recvOnEtherPort){
    printf("\n Join Accept Received\n");       

    uint16_t numOfVID = extract_VID_from_receive_buff(temp_2d_array,recvBuffer_MTP + 15,0);

    for(uint16_t i = 0;i < numOfVID;i++){ // add new VIDs to offered table
        vop_head = add_to_offered_table(vop_head,recvOnEtherPort,temp_2d_array[i]);
    }

    print_offered_table(vop_head);

    vop_temp = find_offered_port_by_name(vop_head, recvOnEtherPort);
    if(!vop_temp->cp){ // turn on this port for transferring data message
        cp_temp = find_control_port_by_name(cp_head,recvOnEtherPort);
        vop_temp->cp = cp_temp;
        cp_temp->isUP = 1;
        cp_temp->start = 1;
    }
    
    send_start_hello(recvOnEtherPort);
}


void handle_receive_start_hello(char* recvOnEtherPort){
    
    // turn on the port
    cp_temp = find_control_port_by_name(cp_head,recvOnEtherPort);
    cp_temp->isUP = 1;
    cp_temp->start = 1;
}


void handle_receive_data_msg(unsigned char* recvBuffer_MTP,char* recvOnEtherPort, socklen_t recv_len_MTP)
{
    printf("\nData message Received\n");

    // Find the control port that received the MTP data message and when it last received an MTP message / keep-alive.
    cp_temp = find_control_port_by_name(cp_head, recvOnEtherPort);
    cp_temp->last_received_time = get_milli_sec(&current_time);

    // If this node is a leaf, strip the MTP header from the message and forward it to the appropriate server.
    if(mtpConfig.isLeaf)
    { 
        /* 
            Currently, the manually-set computeIntfName is used because only one compute interface is used for testing, saving resources and time. 

            The payload breakdown is as follows.
            recvBuffer_MTP:
                Ethernet II (14 bytes) - bytes 0-13
                MTP Data Header (5 bytes) - bytes 14-18
                payload - bytes 19+

            Thus, starting at buffer position 0 (recvBuffer_MTP), +19 (14 + 5) gets you to the payload, which is normally IPv4 header.
        */
        route_data_to_server(mtpConfig.computeIntfName, recvBuffer_MTP + 14 + 5, recv_len_MTP - 14 - 5);
    }

    // If this node is a spine, forward the message to either any available spine at the next tier, or to a specific spine at the tier below. 
    else
    { 
        uint16_t src_VID = 0;
        uint16_t dest_VID = 0;

        memcpy(&src_VID, recvBuffer_MTP + 15, 2);
        memcpy(&dest_VID, recvBuffer_MTP + 17, 2);

        char dest_VID_str[VID_LEN];

        int_to_str(dest_VID_str, dest_VID);

        printf("Src VID = %d\n",src_VID);
        printf("Dest VID = %d\n",dest_VID);
        
        
        // check accepted ports first
        if((vap_temp = find_accepted_port_by_VID(vap_head,dest_VID_str))){ // if dest VID exist in accepted port table
            printf("Found VID in VID_Accepted_Table \n");

            if(!vap_temp->cp->isUP || find_unreachable_VID_by_name(vap_temp->ut, vap_temp->port_name)){
                printf("But this port is down or unreachable for VID %s, dumped packet\n",dest_VID_str);
                return;
            }
            find_control_port_by_name(cp_head,vap_temp->port_name)->last_sent_time = get_milli_sec(&current_time);
            // printf("Sent data message at time = %lld, update port sent time\n",t);

            route_data_from_spine(vap_temp->port_name,recvBuffer_MTP + 14,recv_len_MTP - 14);
        }else{ // else push up, pick one from offered port table

            uint8_t src_ip_3 = 0;
            uint8_t src_ip_4 = 0;
            uint8_t dest_ip_3 = 0;
            uint8_t dest_ip_4 = 0;

            memcpy(&src_ip_3, recvBuffer_MTP + 33, 1);
            memcpy(&src_ip_4, recvBuffer_MTP + 34, 1);
            memcpy(&dest_ip_3, recvBuffer_MTP + 37, 1);
            memcpy(&dest_ip_4, recvBuffer_MTP + 38, 1);

            char hash_str[32]; 
            hash_str[0] = src_ip_3;
            hash_str[1] = src_ip_4;
            hash_str[2] = dest_ip_3;
            hash_str[3] = dest_ip_4;
            
            size_t available_offered_port_num = count_available_offered_port(vop_head,temp_2d_array,dest_VID_str);
            if(!available_offered_port_num){
                printf("Found 0 available port, packet dumped\n");
                return;
            }

            uint32_t hash_code = jenkins_one_at_a_time_hash(hash_str,4);
            printf("VID can't be found in accepted port table, push up to next spine\n");
            printf("available_offered_port_num = %lu\n",available_offered_port_num);
            printf("Hash ascii value array = {%d,%d,%d,%d}, hash_code = %u\n", hash_str[0], hash_str[1], hash_str[2], hash_str[3],hash_code);
            printf("Mod pos index = %lu\n",hash_code % available_offered_port_num);
            find_control_port_by_name(cp_head,temp_2d_array[hash_code % available_offered_port_num])->last_sent_time = get_milli_sec(&current_time);
            // printf("Sent data message at time = %lld, update port sent time\n",t);
            route_data_from_spine(temp_2d_array[hash_code % available_offered_port_num],recvBuffer_MTP + 14,recv_len_MTP - 14);
        }
    }
}

void handle_receive_keep_alive(char* recvOnEtherPort){
        // update receive time of receive keep alive message
    if((cp_temp = find_control_port_by_name(cp_head,recvOnEtherPort))){ // find the receive port

        if(cp_temp->fail_type == DETECT_FAIL) return;
        else if(cp_temp->fail_type == MISS_FAIL) cp_temp->fail_type = 0;

        long long current_timestamp = get_milli_sec(&current_time); // get current time stamp

        // printf("Received keep alive at time %lld, update port %s\n",current_timestamp,cp_temp->port_name);
        
        // recover code
        if(!cp_temp->isUP && current_timestamp - cp_temp->last_received_time < DEAD_TIMER && cp_temp->continue_count < 3){
            cp_temp->continue_count++;
            printf("Received from port %s at time %lld\n",recvOnEtherPort,current_timestamp);
            printf("%s -> count =  %d\n",cp_temp->port_name,cp_temp->continue_count);
            if(cp_temp->continue_count == 3){ // received three consecutive on time keep alive, turn on this port again
                uint16_t numOfVID = 0;
                printf("--------Turn on for port %s after received 3 KEEP ALIVE message --------\n",cp_temp->port_name);
                
                //recover code here
                if((vap_temp = find_accepted_port_by_name(vap_head,cp_temp->port_name))){ // downstream port recovered
                    if(is_all_offered_ports_down(vop_head) && !mtpConfig.isTopSpine){
                        cp_temp->isUP = 1;
                        numOfVID = get_all_accepted_VIDs(vap_head, temp_2d_array);
                        for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){
                            if(vap_temp->cp->isUP){
                                send_failure_update(vap_temp->port_name, temp_2d_array,numOfVID, REACHABLE_OPTION);
                            }
                        }
                    }else{

                        numOfVID = get_accepted_VIDs_by_port_name(vap_head, cp_temp->port_name, temp_2d_array);
                        for(struct control_port* cp_temp2 = cp_head;cp_temp2;cp_temp2 = cp_temp2->next){
                            if(cp_temp2->isUP){ 
                                // send
                                printf("Sent recover at time %lld\n",get_milli_sec(&current_time));
                                send_recover_update(cp_temp2->port_name,temp_2d_array,numOfVID,UNREACHABLE_OPTION);
                            }else{ // store 
                                // prn_head = add_to_port_recover_notification_table(prn_head, cp_temp2->port_name, copy_VID_table(vap_temp->VID_head), UNREACHABLE_OPTION);
                            }
                        } 
                        
                        cp_temp->isUP = 1;
                    }
                }else{
                    if(is_all_offered_ports_down(vop_head) && !mtpConfig.isLeaf){
                        cp_temp->isUP = 1;
                        for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){
                            if(vap_temp->cp->isUP){
                                send_recover_update(vap_temp->port_name,NULL,0,REACHABLE_OPTION);
                            }else{
                                // prn_head = add_to_port_recover_notification_table(prn_head, vap_temp->port_name, NULL, REACHABLE_OPTION);
                            }
                        }
                    }
                    cp_temp->isUP = 1;
                }
            }
        }

        cp_temp->last_received_time = current_timestamp; // update receive time for this port
    }
}

void handle_receive_failure_update(unsigned char* recvBuffer_MTP,char* recvOnEtherPort,  socklen_t recv_len_MTP){
    printf("\n FAILURE UPDATE message received at %lld, on port %s \n",get_milli_sec(&current_time), recvOnEtherPort);
    printf("Message size = %d\n",recv_len_MTP);

    uint8_t table_option = recvBuffer_MTP[15];
    printf("Extract option = %d\n",table_option);
    uint16_t numOfVID = extract_VID_from_receive_buff(temp_2d_array,recvBuffer_MTP + 16,1);

    if((vap_temp = find_accepted_port_by_name(vap_head,recvOnEtherPort))){
        printf("Received from downstream\n");
        for(uint16_t k = 0;k < numOfVID;k++){
            printf("Adding VID = %s to unreachable table for port %s\n",temp_2d_array[k],recvOnEtherPort);
            vap_temp->ut = add_to_unreachable_table(vap_temp->ut,temp_2d_array[k]);
        }
        print_unreachable_table(vap_temp->ut);

        printf("Transfer this message from other working ports\n");

        for(cp_temp = cp_head;cp_temp;cp_temp = cp_temp->next){
            if(strcmp(cp_temp->port_name, recvOnEtherPort) && cp_temp->isUP){
                send_failure_update(cp_temp->port_name,temp_2d_array, numOfVID, UNREACHABLE_OPTION);
            }
        }
    }else if((vop_temp = find_offered_port_by_name(vop_head,recvOnEtherPort))){
        printf("Received from upstream\n");
        if(table_option == UNREACHABLE_OPTION){
            printf("Updated unreachable table for port %s\n",recvOnEtherPort);
            vop_temp->rt->VID_head = clear_VID_table(vop_temp->rt->VID_head); // clear reachable table 
            for(uint16_t k = 0;k < numOfVID;k++){
                printf("Adding VID %s to unreachable table\n",temp_2d_array[k]);
                vop_temp->ut = add_to_unreachable_table(vop_temp->ut,temp_2d_array[k]);
            }
        }else{
            printf("Updated reachable table for port %s\n",recvOnEtherPort);
            // vop_temp->ut->VID_head = clear_VID_table(vop_temp->ut->VID_head); // clear unreachable table
            vop_temp->rt->VID_head = clear_VID_table(vop_temp->rt->VID_head); // clear reachable table
            for(uint16_t k = 0;k < numOfVID;k++){
                printf("Adding VID %s to reachable table\n",temp_2d_array[k]);
                vop_temp->rt = add_to_reachable_table(vop_temp->rt,temp_2d_array[k]);
            }
            
        }

        if(mtpConfig.isLeaf){
            printf("I am a tor, do nothing\n");
            printf("Finished processing failure message at time = %lld\n",get_milli_sec(&current_time));
            return;
        }

        if(!is_unreachable_and_reachable_empty(vop_head)){
            printf("All offered ports are not clean, keep sending\n");
            if((numOfVID = get_unreachable_VIDs_from_offered_ports(vop_head, temp_2d_array))){
                for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){ // send black first
                    if(vap_temp->cp->isUP){
                        send_failure_update(vap_temp->port_name, temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                    }
                }
            }
        }else{
            printf("Some upstream ports are clean, DONE\n");
        }
    }
    printf("Finished processing failure message at time = %lld\n",get_milli_sec(&current_time));
}

void handle_receive_recover_update(unsigned char* recvBuffer_MTP,char* recvOnEtherPort){
    printf("\n RECOVER UPDATE message received at %lld, on port %s \n",get_milli_sec(&current_time), recvOnEtherPort);
    uint8_t table_option = recvBuffer_MTP[15];
    printf("Extract option = %d\n",table_option);
    uint16_t numOfVID = extract_VID_from_receive_buff(temp_2d_array,recvBuffer_MTP + 16,1);

    if((vap_temp = find_accepted_port_by_name(vap_head,recvOnEtherPort))){
        printf("Received from downstream\n");
        for(uint16_t k = 0;k < numOfVID;k++){
            printf("Removing VID = %s unreachable table for port %s\n",temp_2d_array[k],recvOnEtherPort);
            vap_temp->ut = remove_unreachable_VID_by_name(vap_temp->ut,temp_2d_array[k]);
        }

        print_unreachable_table(vap_temp->ut);

        printf("Transfer this message from other working ports\n");

        for(cp_temp = cp_head;cp_temp;cp_temp = cp_temp->next){
            if(strcmp(cp_temp->port_name, recvOnEtherPort) && cp_temp->isUP){
                send_recover_update(cp_temp->port_name,temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                printf("Sent out from port %s\n",cp_temp->port_name);
            }
        }
    }else if((vop_temp = find_offered_port_by_name(vop_head,recvOnEtherPort))){
        printf("Received from upstream\n");
        
        if(table_option == UNREACHABLE_OPTION){
            int is_clean_before = is_unreachable_and_reachable_empty(vop_head);
            for(uint16_t k = 0;k < numOfVID;k++){
                printf("Removing VID = %s from unreachable table for port %s\n",temp_2d_array[k],recvOnEtherPort);
                vop_temp->ut = remove_unreachable_VID_by_name(vop_temp->ut,temp_2d_array[k]);
            }
            int is_clean_after = is_unreachable_and_reachable_empty(vop_head);

            if(mtpConfig.isLeaf){
                return;
            }

            if(!is_clean_before && !is_clean_after){
                for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){
                    if(vap_temp->cp->isUP){
                        send_recover_update(vap_temp->port_name, temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                    }
                }
            }else if(!is_clean_before && is_clean_after){
                numOfVID += get_unreachable_VIDs_from_offered_ports(vop_head, temp_2d_array + numOfVID);
                for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){ // send black first
                    if(vap_temp->cp->isUP){
                        send_recover_update(vap_temp->port_name, temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                    }
                }
            }

        }else{
            printf("Clear reachable table for port %s\n",recvOnEtherPort);
            int is_clean_before = is_unreachable_and_reachable_empty(vop_head);
            vop_temp->rt->VID_head = clear_VID_table(vop_temp->rt->VID_head); // clear reachable table   
            
            if(mtpConfig.isLeaf){
                return;
            }
            int is_clean_after = is_unreachable_and_reachable_empty(vop_head);
            if(!is_clean_before && is_clean_after){
                if((numOfVID = get_unreachable_VIDs_from_offered_ports(vop_head, temp_2d_array))){
                    for(vap_temp = vap_head;vap_temp;vap_temp = vap_temp->next){ // send black first
                        if(vap_temp->cp->isUP){
                            send_recover_update(vap_temp->port_name, temp_2d_array, numOfVID, UNREACHABLE_OPTION);
                        }
                    }
                }
            }          
        }
    }
}

void handle_receive_from_server(unsigned char* recvBuffer_IP,char* recvOnEtherPort, socklen_t recv_len_IP){
    printf("\n Received an IP message on port %s from server\n",  recvOnEtherPort); 
    unsigned char *ip_header_with_payload = recvBuffer_IP + 14;

    printf("Src IP = %d.%d.%d.%d\n",ip_header_with_payload[12],ip_header_with_payload[13],ip_header_with_payload[14],ip_header_with_payload[15]);
    printf("Dest IP = %d.%d.%d.%d\n",ip_header_with_payload[16],ip_header_with_payload[17],ip_header_with_payload[18],ip_header_with_payload[19]);

    uint16_t src_VID = ip_header_with_payload[12 + VID_octet - 1];
    uint16_t dest_VID = ip_header_with_payload[16 + VID_octet - 1];
    uint8_t src_ip_3 = ip_header_with_payload[14]; // third byte of src IP
    uint8_t src_ip_4 = ip_header_with_payload[15]; // fourth byte of src IP
    uint8_t dest_ip_3 = ip_header_with_payload[18]; // third byte of dest IP
    uint8_t dest_ip_4 = ip_header_with_payload[19]; // fourth byte of dest IP

    char dest_VID_str[VID_LEN];
    int_to_str(dest_VID_str, dest_VID);

    printf("Src VID = %d\n",src_VID);
    printf("Dest VID = %d\n",dest_VID);

    // hash implementation
    char hash_str[32]; 
    hash_str[0] = src_ip_3;
    hash_str[1] = src_ip_4;
    hash_str[2] = dest_ip_3;
    hash_str[3] = dest_ip_4;

    uint32_t hash_code = jenkins_one_at_a_time_hash(hash_str,4); // hash src VID and dest VID
    size_t available_offered_port_num = count_available_offered_port(vop_head,temp_2d_array,dest_VID_str);
    if(!available_offered_port_num){
        printf("Found 0 available port, packet dumped\n");
    }else{
        printf("available_port_num = %lu\n",available_offered_port_num);
        printf("Hash ascii value array = {%d,%d,%d,%d}, hash_code = %u\n", hash_str[0], hash_str[1], hash_str[2], hash_str[3],hash_code);
        printf("Mod pos index = %lu\n",hash_code % available_offered_port_num);
        // pick one port, then send the message out

        find_control_port_by_name(cp_head,temp_2d_array[hash_code % available_offered_port_num])->last_sent_time = get_milli_sec(&current_time);
        route_data_from_tor_to_spine(temp_2d_array[hash_code % available_offered_port_num], src_VID, dest_VID, ip_header_with_payload, recv_len_IP - 14); 
    }
}