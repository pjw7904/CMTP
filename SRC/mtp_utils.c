
#include "mtp_utils.h"
#include "mtp_struct.h"


// it is different from previous one, it stores the port data to a 2d array
uint8_t get_all_ethernet_interface2(char** dest){
    uint8_t counter = 0;
    struct ifaddrs *addrs, *tmp;                                        
    getifaddrs(&addrs);          // Purpose - get interface addresses
    tmp = addrs;                 // initialize tmp to where addresses are stored

    while( tmp ){
        if ( tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET ){   
            if((strcmp(tmp->ifa_name, "lo")) && (strcmp(tmp->ifa_name, "eth0")) && (tmp->ifa_flags & IFF_UP) != 0 ){
                strcpy(dest[counter],tmp->ifa_name);
                counter++;
            }
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    return counter; // return the size of all working ports
}


int check_port_is_alive(char** port_array, uint8_t port_array_size, char* port_name){
    for(uint8_t i = 0;i < port_array_size;i++){
        if(!strcmp(port_array[i],port_name)){ // if found, return true
            return 1;
        }
    }
    return 0; // can't found, return false
}


void get_VID_by_ethernet_interface(char *dest, char *ethernet_interface_name, uint8_t octet){
    int fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);                 // Socket creation
    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;                   // Getting IP addr.
    /* I want IP address attached to "eth0" */          // Need to get ip and ports all of the interfaces connected!!!
    strncpy(ifr.ifr_name, ethernet_interface_name, IFNAMSIZ-1);           // By Mana - Changed lo to eth0   
    ioctl(fd, SIOCGIFADDR, &ifr);                       // ioctl - Input output control   
                                                        // SIOCGIFADDR - Get, set, or delete the address of the device using ifr_addr, or ifr6_addr with ifr6_prefixlen. 
    close(fd);
    /* display result */
    //printf("\n IP address of the device: %s\n", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));           // inet_ntoa - function converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation
    char *ipaddr = inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);

    int i = 0, dot_counter = 0,j = 0;
    while(  dot_counter < octet - 1 ){
        if(ipaddr[i++] == '.') dot_counter++;
    }

    while(ipaddr[i] != '.'){
        dest[j++] = ipaddr[i++];
    }

    dest[j] = '\0';
}


uint8_t get_tier_from_hello_message(char *payload_with_VID_data){
    uint8_t vid_len = payload_with_VID_data[2]; // get length of the first vid
    uint8_t tier_counter = 1; // each vid at least has 1 tier
    for(int i = 0;i < vid_len;i++){ 
        if(payload_with_VID_data[3 + i] == '.'){ // count dot in the first vid
            tier_counter++;
        }
    }
    return tier_counter;
}

// append ethernet interface number to a VID
void append_port_number_after_VID(char *port_name, char *dest){
    strcat(dest,".");
    int k = 0;
    while(port_name[k] < '0' || port_name[k] > '9') k++;
    strcat(dest, port_name + k);
}

// extract all VID from the receive buff, then store them to a 2d array
uint16_t extract_VID_from_receive_buff(char **VID_array, char *recvBuff_start_ptr,int with_debug){
    uint16_t numOfVID = 0;
    memcpy(&numOfVID,recvBuff_start_ptr,2);
    int p = 2;
    for(int i = 0;i < numOfVID;i++){
        uint8_t vid_len = recvBuff_start_ptr[p];
        p++;
        strncpy(VID_array[i],recvBuff_start_ptr + p,vid_len);
        VID_array[i][vid_len] = '\0';
        p += vid_len;
    }
    return numOfVID;
}


// hash algorithm for loading balancing
uint32_t jenkins_one_at_a_time_hash(uint8_t *key, size_t len){
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

// convert integer to string (char[])
size_t int_to_str(char *dest_storage, unsigned int number){
    size_t num_len = 0;
    while(number){
        dest_storage[num_len++] = number % 10 + '0';
        number /= 10;
    }

    for(int i = 0;i < num_len / 2;i++){
        char t = dest_storage[0];
        dest_storage[0] = dest_storage[num_len - i - 1];
        dest_storage[num_len - i - 1] = t;
    }
    dest_storage[num_len] = '\0';
    return num_len; // return the length of this number in string format
}

// get current time in millisecond
long long get_milli_sec(struct timeval* current_time){
    gettimeofday(current_time, NULL);
    long long milliseconds = current_time->tv_sec*1000LL + current_time->tv_usec/1000;
    return milliseconds;
}

// initialize socket resource for each port
void init_socket_resources(int* socketfd, struct control_port* cp_head,char* network_port_name){
    struct control_port* cp_temp = cp_head;
    while(cp_temp){ // iterate each port
        struct ifreq if_idx;
	    struct ifreq if_mac;
        char ifName[IFNAMSIZ];
        strcpy(ifName, cp_temp->port_name);
        struct ether_header *eh = (struct ether_header*)calloc(1, sizeof(struct ether_header));
        struct sockaddr_ll* socket_address = (struct sockaddr_ll*) calloc(1,sizeof(struct sockaddr_ll));
        memset(&if_idx, 0, sizeof(struct ifreq));
	    strncpy(if_idx.ifr_name, ifName, IFNAMSIZ - 1);
        if (ioctl(*socketfd, SIOCGIFINDEX, &if_idx) < 0)
		    perror("SIOCGIFINDEX - Misprint Compatibility");
        
        memset(&if_mac, 0, sizeof(struct ifreq));
	    strncpy(if_mac.ifr_name, ifName, IFNAMSIZ - 1);
        if (ioctl(*socketfd, SIOCGIFHWADDR, &if_mac) < 0)
		    perror("SIOCGIFHWADDR - Either interface is not correct or disconnected");

        for(int i = 0;i < 6;i++){ // configure src mac
            eh->ether_shost[i] = ((uint8_t *) &if_mac.ifr_hwaddr.sa_data)[i];
        }

        for(int i = 0;i < 6;i++){ // configure dest mac
            eh->ether_dhost[i] = 0xFF;
        }

        if(!strcmp(cp_temp->port_name, network_port_name)){
            eh->ether_type = htons(ETH_IP_CTRL);
        }else{
            eh->ether_type = htons(ETH_MTP_CTRL);
        }

        memcpy(cp_temp->frame, eh, sizeof(struct ether_header));

        socket_address->sll_ifindex = if_idx.ifr_ifindex;

        socket_address->sll_halen = ETH_ALEN;

        for(int i = 0;i < 6;i++){
            socket_address->sll_addr[i] = 0xFF;
        }

        cp_temp->socket_address = socket_address;

        free(eh);
        cp_temp = cp_temp->next;
    }
}
