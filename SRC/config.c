#include "config.h"
#include "mtp_utils.h"
#include "mtp_struct.h"

void readConfigurationFile(Config *config) 
{
    // Access the configuration file.
    FILE *fp = fopen("dcn.conf", "r");
    if (!fp) 
    {
        perror("\nFailed to open config file\n");
        return;
    }

    // Read through each line of the configuration file.
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
        
        // Determine if the MTP node is a spine and at the top tier. 
        else if (strcmp(configName, "isTopSpine") == 0) 
        {
            config->isTopSpine = strcmp("true", value) == 0 ? 1 : 0;
        } 
       
       // Determine the tier of the MTP node. 
        else if (strcmp(configName, "tier") == 0) 
        {
            config->tier = atoi(value);
        } 
        
        // Grab the IP address of the compute interface on a leaf.
        else if (strcmp(configName, "computeIP") == 0 && config->isLeaf) 
        {
            strcpy(config->computeIP, value);
        }
    }

    fclose(fp);
}

struct control_port* initialInterfaceConfiguration(const char *computeSubnetIPAddress, char *computeSubnetIntfName, bool isLeaf) 
{
    
    // Use ifaddrs structure to loop through network interfaces on the system.
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];

    if(getifaddrs(&ifaddr) == -1) 
    {
        perror("\nGetting network interfaces failed (getifaddrs)\n");
        exit(EXIT_FAILURE);
    }

    // Define the head of the MTP-speaking interfaces linked list (AKA the control ports).
    struct control_port* cp_head = NULL;

    // Loop through each network interface on the system.
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
    {
        // if the interface is null, skip it.
        if (ifa->ifa_addr == NULL)
        {
            continue;
        }

        // Grab the interface family (AF_INET = IPv4 addressing, AF_PACKET = raw layer 2).
        family = ifa->ifa_addr->sa_family;

        // Check to make sure the interface name is not eth0 or loopback and it.
        if (strcmp(ifa->ifa_name, "eth0") != 0 && strcmp(ifa->ifa_name, "lo") != 0 && (ifa->ifa_flags & IFF_UP) != 0) 
        {
            // If the interface contains an IPv4 address and is it a leaf, analyze it.
            if (isLeaf && family == AF_INET) 
            {
                // Get a string copy of the IPv4 address.
                int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                    host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s != 0) 
                {
                    printf("\ngetnameinfo() failed: %s\n", gai_strerror(s));
                    exit(EXIT_FAILURE);
                }

                // Check if the IPv4 address matches the compute subnet IPv4 address.
                if (computeSubnetIPAddress && strcmp(host, computeSubnetIPAddress) == 0) 
                {
                    // Copy the compute subnet interface name.
                    strcpy(computeSubnetIntfName, ifa->ifa_name);
                    printf("\nAdded interface %s as a compute port.\n", ifa->ifa_name);
                }
            }

            else if(family == AF_PACKET)
            {
                cp_head = add_to_control_port_table(cp_head, ifa->ifa_name);
                printf("\nAdded interface %s as a control port.\n", ifa->ifa_name);
            }
        } 
    }

    // Free the ifaddr memory.
    freeifaddrs(ifaddr);

    // return the head of the linked list.
    return cp_head;
}