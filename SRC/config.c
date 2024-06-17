/*
 * Custom MTP imports.
 */
#include "config.h"
#include "mtp_utils.h"
#include "mtp_struct.h"

void readConfigurationFile(Config *config) 
{
    // Access the configuration file.
    FILE *fp = fopen("mtp.conf", "r");
    if (!fp) 
    {
        perror("\nFailed to open config file\n");
        return;
    }

    /* 
        Read through each line of the configuration file.
        A configuration line is in the format:

        key:value

        where the key and value is deliminated by a colon (:).
    */
    char buff[255];
    while(fgets(buff, sizeof(buff), fp)) 
    {
        // Grab the configuration key by splitting on the colon delimiter.
        char *configName = strtok(buff, ":");
        if(configName == NULL) continue;
        
        // Grab the configuration value and remove the newline.
        char *value = strtok(NULL, "\n"); 
        if(value == NULL) continue;
        
        
        // Determine if the MTP node is a leaf. 
        if (strcmp(configName, "isLeaf") == 0) 
        {
            config->isLeaf = strcmp("true", value) == 0 ? 1 : 0;
        } 
        

        // Determine if the MTP node is a leaf. 
        if (strcmp(configName, "isLeaf") == 0) 
        {
            config->isLeaf = strcmp("true", value) == 0 ? 1 : 0;
        } 
        
        // Determine if the MTP node is a spine and at the top tier. 
        if(strcmp(configName, "isTopSpine") == 0) 
        {
            config->isTopSpine = strcmp("True", value) == 0 ? true : false;
        } 

       // Determine the tier of the MTP node. 
        else if(strcmp(configName, "tier") == 0) 
        {
            // To-Do: Add error check for atoi conversion.
            uint8_t tierValue = atoi(value);

            config->tier = tierValue;

            // Any tier that is not 1 (0 is the compute tier) is not a leaf
            config->isLeaf = tierValue == 1 ? true : false;
        } 
    }

    fclose(fp);
}

compute_interface* setComputeInterfaces(struct ifaddrs *ifaddr, char *computeSubnetIntfName, bool isLeaf)
{
    struct ifaddrs *ifa;
    int family;

    // Define the head of the Non-MTP-speaking interfaces linked list (AKA the IPv4 compute ports).
    compute_interface *compute_intf_head = NULL;

    // The node is not a leaf, thus it is a spine and does not have a compute interface.
    if(!isLeaf)
    {
        strcpy(computeSubnetIntfName, "None");
        printf("\nNode is a spine, no compute interface.\n");
        return NULL;
    }

    // Iterate over the network interfaces.
    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if(ifa->ifa_addr == NULL) continue;

        // AF_INET = IPv4 addressing.
        family = ifa->ifa_addr->sa_family;

        // If the interface is active/up, contains an IPv4 address, and is not named eth0 or lo (loopback).
        if(family == AF_INET && 
            strcmp(ifa->ifa_name, "eth0") != 0 && 
            strcmp(ifa->ifa_name, "lo") != 0 && 
            (ifa->ifa_flags & IFF_UP) != 0)
        {
            // Mark the interface name as part of the compute interface table, and then copy the interface name seperately.
            compute_intf_head = addComputeInteface(compute_intf_head, ifa->ifa_name);

            strcpy(computeSubnetIntfName, ifa->ifa_name);
            printf("\nInterface %s is set as the compute port.\n", ifa->ifa_name);
        }
    }
    
    // return the head of the linked list.
    return compute_intf_head;
}

struct control_port* setControlInterfaces(struct ifaddrs *ifaddr, char *computeSubnetIntfName, bool isLeaf) 
{
    // Use ifaddrs structure to loop through network interfaces on the system.
    struct ifaddrs *ifa;
    int family;

    // Define the head of the MTP-speaking interfaces linked list (AKA the control ports).
    struct control_port* cp_head = NULL;

    // Loop through each network interface on the system.
    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if(ifa->ifa_addr == NULL) continue;

        // Grab the interface family (AF_INET = IPv4 addressing, AF_PACKET = raw layer 2).
        family = ifa->ifa_addr->sa_family;

        // If the interface is active/up, and is not named eth0 or lo (loopback).
        if(family == AF_PACKET && 
            strcmp(ifa->ifa_name, "eth0") != 0 && 
            strcmp(ifa->ifa_name, "lo") != 0 &&
            (ifa->ifa_flags & IFF_UP) != 0) 
        {
            // If the node is a leaf and this is the compute interface, skip it.
            if(isLeaf && strcmp(ifa->ifa_name, computeSubnetIntfName) == 0)
            {
                continue;
            }

            cp_head = add_to_control_port_table(cp_head, ifa->ifa_name);
            printf("\nAdded interface %s as a control port.\n", ifa->ifa_name);
        } 
    }

    // return the head of the linked list.
    return cp_head;
}