#include "mtp_send.h"

int socketfd = -1;

// Control interfaces information.
extern struct control_port* cp_head;
struct control_port* t;

// Compute interfaces information.
extern compute_interface *compute_intf_head;
compute_interface *computeFrame;

int dataSend(char* port_name, int payloadLen, unsigned char *inPayload) 
{
	t = find_control_port_by_name(cp_head, port_name);

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
	if (sendto(socketfd, t->frame, frame_size, MSG_NOSIGNAL, (struct sockaddr*) t->socket_address, sizeof(struct sockaddr_ll)) < 0) {
		perror("Send error (datasend)");
		return -1;
	}

	return 0;
}

int computeSend(char* port_name, int payloadLen, unsigned char *inPayload) 
{
	computeFrame = compute_intf_head;

	//HEADER_SIZE = 14, as defined by mtp_send.h | payloadLen is 1 at this point (?)
	int frame_size = HEADER_SIZE + payloadLen;

	// Copying payLoad to frame
	//printf("\n\n BEFORE copying payload in ctrlSend\n");
	memcpy(computeFrame->frame + HEADER_SIZE, inPayload, payloadLen);

	if(sendto(socketfd, computeFrame->frame, frame_size, MSG_NOSIGNAL, (struct sockaddr*) computeFrame->socket_address, sizeof(struct sockaddr_ll)) < 0) 
	{
		perror("Send error (computeSend)");
		return -1;
	}

	return 0;
}