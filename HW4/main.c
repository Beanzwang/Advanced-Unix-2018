#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <net/ethernet.h> /* the L2 protocols */
#define BUF_SIZE 1024
/*
Requirements:
1. Enumerate all Ethernet compatible network interfaces.
2. Ask the user to provide his/her username.
3. Repeatedly ask the user to enter his/her message. The message is then broadcasted to all enumerated Ethernet-compatible network interfaces.
4. At the same time, your program should receive messages broadcasted by other host in connected LANs.
*/

char *addr[50];
char *if_name[50];
int sockfd;

int list_if();
void send_pckt(char *if_name, char *line, char *name);
void recv_pckt();

int main()
{
	int if_num;
	int max_len = BUF_SIZE - sizeof(struct ether_header) - 4;
	char *name = malloc(max_len + 1);

	// A raw socket receives or sends the raw datagram not including link level headers.
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(0x801))) == -1)
		perror("Fail to open socket.");
	if_num = list_if();

	printf("Enter your name: ");
	if (fgets(name, max_len, stdin) != NULL) {
		name[strlen(name)-1] = '\0';
		printf("Welcome, \'%s\'!\n", name);
	}

	pid_t pid;
	pid = fork();
	if (pid < 0) {
		perror("Error");
	} else if(pid == 0) {
		while(1){
			recv_pckt();
		}
	} else {
		while (1)
		{
			char *line = malloc(max_len - strlen(name));
			
			if (fgets(line, max_len-strlen(name), stdin) != NULL) {
				// send to all ethernet adapter.
				line[strlen(line)-1] = '\0';
				for (int i = 0; i < if_num; ++i)
					send_pckt(if_name[i], line, name);
			}

			free(line);
		}
	}

	free(name);

	return 0;
}

int list_if() {
	struct ifaddrs *ifap, *ifa;
	struct sockaddr_in *sa, *sa_nm, *sa_bc;
	struct ifreq if_idx, if_mac; // char ifr_name[size], union
	unsigned char *netmask;
	char *broadcast;
	int if_num = 0;

	puts("Enumerated network interfaces:");

	getifaddrs(&ifap);
	for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			sa = (struct sockaddr_in *)ifa->ifa_addr;
			sa_nm = (struct sockaddr_in *)ifa->ifa_netmask;

			// do not broadcast to yourself (localhost)
			if (16777343 == sa->sin_addr.s_addr)
				continue;
			addr[if_num] = inet_ntoa(sa->sin_addr);
			if_name[if_num] = ifa->ifa_name;
			sa_bc = (struct sockaddr_in *)ifa->ifa_ifu.ifu_broadaddr;
			broadcast = inet_ntoa(sa_bc->sin_addr);
			netmask = (unsigned char *)&sa_nm->sin_addr.s_addr;

			/* Get the index of the interface to send on */
			memset(&if_idx, 0, sizeof(struct ifreq));
			strncpy(if_idx.ifr_name, if_name[if_num], IFNAMSIZ - 1);
			if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
				perror("SIOCGIFINDEX");

			/* Get the MAC address of the interface to send on */
			memset(&if_mac, 0, sizeof(struct ifreq));
			strncpy(if_mac.ifr_name, if_name[if_num], IFNAMSIZ - 1);
			if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
				perror("SIOCGIFHWADDR");

			// TODO: format string & testing
			printf("%d - %s\t%s 0x%02x%02x%02x%02x (%s) %02x:%02x:%02x:%02x:%02x:%02x\n", if_idx.ifr_ifindex, if_name[if_num], addr[if_num],
				   netmask[0], netmask[1], netmask[2], netmask[3], broadcast,((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0],
				   ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1], ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2], ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3],
				   ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4], ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5]);
			if_num++;
		}
	}

	return if_num;
}

void send_pckt(char *if_name, char *line, char *name) {
	struct ifreq if_idx, if_mac; // char ifr_name[size], union
	char sendbuf[BUF_SIZE];
	// u_char ether_dhost[6], u_char ether_shost[6], u_short ether_type;
	struct ether_header *eh = (struct ether_header *)sendbuf;
	struct sockaddr_ll socket_address;  // link level 
	int tx_len = 0;

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, if_name, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
		perror("SIOCGIFINDEX");
	
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, if_name, IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
		perror("SIOCGIFHWADDR");

	memset(sendbuf, 0, BUF_SIZE);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = 0xff;
	eh->ether_dhost[1] = 0xff;
	eh->ether_dhost[2] = 0xff;
	eh->ether_dhost[3] = 0xff;
	eh->ether_dhost[4] = 0xff;
	eh->ether_dhost[5] = 0xff;
	/* Ethertype field */
	eh->ether_type = htons(0x801);
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	// name
	int name_len =  strlen(name);
	sendbuf[tx_len++] = '[';
	for (int i = 0; i < name_len; ++i) {
		sendbuf[tx_len++] = name[i];
	}
	sendbuf[tx_len++] = ']';
	sendbuf[tx_len++] = ':';
	sendbuf[tx_len++] = ' ';

	// msg
	for (int i = 0; i < strlen(line); ++i) {
		sendbuf[tx_len++] = line[i];
	}

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = 0xff;
	socket_address.sll_addr[1] = 0xff;
	socket_address.sll_addr[2] = 0xff;
	socket_address.sll_addr[3] = 0xff;
	socket_address.sll_addr[4] = 0xff;
	socket_address.sll_addr[5] = 0xff;

	/* Send packet, non-blocking */
	if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr *)&socket_address, sizeof(struct sockaddr_ll)) < 0)
		printf("Send failed\n");
}

void recv_pckt() {
	char buf[BUF_SIZE];
	struct ether_header *eh = (struct ether_header *)buf;
	int num_byte;
	memset(buf, 0, BUF_SIZE);
	if ((num_byte = recvfrom(sockfd, buf, BUF_SIZE, 0, NULL, NULL)) < 0) {
		printf("Recv failed\n");
	} else {
		char *content = buf + sizeof(struct ether_header);
		printf(">>> <%02x:%02x:%02x:%02x:%02x:%02x> %s\n", eh->ether_shost[0], eh->ether_shost[1],
			   eh->ether_shost[2], eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5], content);
	}
}
