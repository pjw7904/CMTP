/*
 * mtp_send.c
 * Peter Willis - MARKED UP FOR MY UNDERSTANDING
 */

//header file which contains function prototypes, and destination MAC addrs (FF) repeated along with the HEADER_SIZE constant
#include "mtp_send.h"

int socketfd = -1;
extern struct control_port* cp_head;
struct control_port* t;
/*
 *
 * 
 * 
 * ---------- COMMENTS FROM OLD CODE -------------------------
 * This function is called in main.c at line 180 after possibly (if statement) building a JOIN or PERIODIC message.
 * The function call in main is: ctrlSend(interfaceNames[i], payload, payloadlen)
 */
int dataSend(char* port_name, int payloadLen,unsigned char *inPayload) {

	t = find_control_port_by_name(cp_head,port_name);

	//HEADER_SIZE = 14, as defined by mtp_send.h | payloadLen is 1 at this point (?)
	int frame_size = HEADER_SIZE + payloadLen;

	// Copying payLoad to frame
	//printf("\n\n BEFORE copying payload in ctrlSend\n");
	memcpy(t->frame + HEADER_SIZE, inPayload, payloadLen);


	/* Send frame
	 * sockfd is the socket being utilized, frame is the message being sent, frame_size is the length of the message
	 * &socket_address is the destination address being used (has to be of struct sockaddr)
	 *
	 */
	if (sendto(socketfd, t->frame, frame_size, 0, (struct sockaddr*) t->socket_address, sizeof(struct sockaddr_ll)) < 0) {
		perror("Send error");
		return -1;
	}

	//free Ethernet memory, close out the socket, and return a successful status code
	return 0;
}





// int dataSend(char *etherPort, uint8_t *inPayload, int payloadLen) {
//   int sockfd;
//   struct ifreq if_idx;

//   struct sockaddr_ll socket_address;

//   // Open RAW socket to send on
//   if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
//     perror("Socket Error");
//   }

//   memset(&if_idx, 0, sizeof(struct ifreq));
//   strncpy(if_idx.ifr_name, etherPort, strlen(etherPort));
//   if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
//     perror("SIOCGIFINDEX - Misprint Compatibility");
//   }

//   // Index of the network device
//   socket_address.sll_ifindex = if_idx.ifr_ifindex;

//   // Address length - 6 bytes
//   socket_address.sll_halen = ETH_ALEN;

//   // Send packet
//   if (sendto(sockfd, inPayload, payloadLen, 0, (struct sockaddr*) &socket_address, sizeof(struct sockaddr_ll)) < 0) {
//     printf("ERROR: Send failed\n");
//   }

//   close(sockfd);
//   return 0;
// }
