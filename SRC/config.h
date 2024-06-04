#ifndef CONFIG_H
#define CONFIG_H

/*
 * Standard library imports.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netdb.h>
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
/*
    Defines the configuration given by the configuration file 
    and the interface of the compute subnet if its a leaf.
*/
typedef struct Config {
    bool isLeaf;
    bool isTopSpine;
    uint8_t tier;
    char computeIntfName[ETH_LEN];
} Config;

/*****************************************
 * FUNCTION PROTOTYPES 
 *****************************************/
void readConfigurationFile(Config *config);

#endif