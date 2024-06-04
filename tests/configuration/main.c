/*
 * Standard library imports.
 */
#include <stdint.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Custom MTP imports.
 */
#include "../../SRC/config.h"
#include "../../SRC/mtp_struct.h"
#include "../../SRC/mtp_utils.h"

/*
 * Configuration variables
 */ 
Config mtpConfig = {0};
uint8_t VID_octet = 3; // Third octet in IPv4 address, example: octet value C in address A.B.C.D
char my_VID[VID_LEN] = {'\0'};

/*
 * Control Ports (MTP-speaking interfaces)
 */
struct control_port* cp_head = NULL;

/*
 * Compute Ports (IPv4-speaking interfaces)
 */
compute_interface *compute_intf_head = NULL;

void print_interfaces(struct ifaddrs *ifaddr) 
{
    struct ifaddrs *ifa;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (ifa->ifa_addr == NULL)
            continue;

        printf("Interface: %s\n", ifa->ifa_name);
    }
}

int main(int argc, char **argv)
{
    // Read the file and populate the config
    readConfigurationFile(&mtpConfig);

    // Use ifaddrs structure to loop through network interfaces on the system.
    struct ifaddrs *ifaddr;
    if(getifaddrs(&ifaddr) == -1) 
    {
        perror("\nGetting network interfaces failed (getifaddrs).\n");
        exit(EXIT_FAILURE);
    }

    // Print the interfaces to stdout.
    print_interfaces(ifaddr);

    // Find if a compute interface exists on the node and then find the control (MTP) interfaces.
    compute_intf_head = setComputeInterfaces(ifaddr, mtpConfig.computeIntfName, mtpConfig.isLeaf);
    cp_head = setControlInterfaces(ifaddr, mtpConfig.computeIntfName, mtpConfig.isLeaf);

    // Leaf nodes are the root of the trees, they define the starting (root) VID.
    if(mtpConfig.isLeaf)
    {
        getRootVID(my_VID, mtpConfig.computeIntfName, VID_octet);
        printf("\nThe root VID: %s\n\n", my_VID);
    }

    // Print the tables out.
    printf("===MTP START-UP CONFIG===tier = %d\nisTopSpine = %d\nisLeaf = %d\ncomputeIntfName = %s\n\n", 
            mtpConfig.tier, mtpConfig.isTopSpine, mtpConfig.isLeaf, mtpConfig.computeIntfName);
    print_control_port_table(cp_head);
    printComputeInterfaces(compute_intf_head);

    // Free all of the memory
    freeifaddrs(ifaddr);
    cp_head = clear_control_port(cp_head);
    compute_intf_head = freeComputeInterfaces(compute_intf_head);

    return 0;
}