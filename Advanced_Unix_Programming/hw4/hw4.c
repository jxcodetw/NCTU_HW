#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <netinet/ether.h>
#include <byteswap.h>

#define ETHER_TYPE ETH_P_IP + 1

const unsigned char DEST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct interface {
	struct ether_header *eh;
	char packet[1024];
	char *name;
	struct sockaddr_ll socket_address;
	int sockfd;

	union {
		unsigned int addr;
		unsigned char addr_[4];
	};
	unsigned int netmask;
	union {
		unsigned int broadaddr;
		unsigned char broadaddr_[4];
	};

	unsigned char mac[6];
};

struct interface_list {
	struct interface *data;
	int len;
};

unsigned int sockaddr_to_unsigned(struct sockaddr *addr) {
	struct sockaddr_in * addr_in = (struct sockaddr_in*)addr;
	return __bswap_32(addr_in->sin_addr.s_addr);
}

void print_interfaces(struct interface_list *list) {
	for(int i=0;i<list->len;++i) {
		struct interface *iface = &(list->data[i]);
		struct ether_header *eh = (struct ether_header*)(iface->packet);
		printf("%d - %s\t%03d.%03d.%03d.%03d 0x%x (%03d.%03d.%03d.%03d) %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", 
			iface->socket_address.sll_ifindex, iface->name,
			iface->addr_[3], iface->addr_[2], iface->addr_[1], iface->addr_[0],
			iface->netmask,
			iface->broadaddr_[3], iface->broadaddr_[2], iface->broadaddr_[1], iface->broadaddr_[0],
			eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2],
			eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5]
			);
	}
}

void init_interfaces(struct interface_list *list) {
	struct ifaddrs *ifaddrs_list;
	int ret = getifaddrs(&ifaddrs_list);
	if (ret == -1) {
		perror("error getifaddrs()");
	}

	list->len = 0;
	for(struct ifaddrs *next=ifaddrs_list;next != NULL;next=next->ifa_next) {
		if (next->ifa_addr->sa_family == AF_INET
			&& !(next->ifa_flags & IFF_LOOPBACK)) {
				list->len+=1;
		}
	}
	list->data = (struct interface*)malloc(sizeof(struct interface) * list->len);

	int idx = 0;
	for(struct ifaddrs *next=ifaddrs_list; next != NULL; next=next->ifa_next) {
		if (next->ifa_addr->sa_family == AF_INET && !(next->ifa_flags & IFF_LOOPBACK)) {
			struct interface *iface = &(list->data[idx]);

			// init
			iface->eh = (struct ether_header*)(iface->packet);
			struct ether_header *eh = iface->eh;
			struct sockaddr_ll *socket_address = &(iface->socket_address);
			memset(iface, 0, sizeof(struct interface));
			memset(eh, 0, sizeof(struct ether_header));
			memset(socket_address, 0, sizeof(struct sockaddr_ll));

			// copy name
			iface->name = next->ifa_name;

			// copy addr
			iface->addr = sockaddr_to_unsigned(next->ifa_addr);
			iface->netmask = sockaddr_to_unsigned(next->ifa_netmask);
			iface->broadaddr = sockaddr_to_unsigned(next->ifa_broadaddr);

			// get sockfd
			int sockfd;
			if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
				perror("socket");
			}
			iface->sockfd = sockfd;
			
			// get mac
			struct ifreq ifr;
			ifr.ifr_addr.sa_family = AF_INET;
			strncpy(ifr.ifr_name, next->ifa_name, IFNAMSIZ-1);
			if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
				puts(next->ifa_name);
				perror("SIOCGIFHWADDR");
			}

			for(unsigned char i=0;i<6;++i) {
				eh->ether_shost[i] = ifr.ifr_hwaddr.sa_data[i];
				eh->ether_dhost[i] = DEST_MAC[i];
				socket_address->sll_addr[i] = DEST_MAC[i];
				
			}
			eh->ether_type = htons(ETHER_TYPE);

			
			// get ifindex
			struct ifreq if_idx;
			memset(&if_idx, 0, sizeof(struct ifreq));
			strncpy(if_idx.ifr_name, next->ifa_name, IFNAMSIZ-1);
			if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
				puts(next->ifa_name);
				perror("SIOCGIFINDEX");
			}

			// get socket_address
			socket_address->sll_family = AF_PACKET;
			socket_address->sll_protocol = htons(ETHER_TYPE);
			socket_address->sll_ifindex = if_idx.ifr_ifindex;
			socket_address->sll_halen = ETH_ALEN;

			idx += 1;
		}
	}
}

#define BUF_SIZE 1024
char name[BUF_SIZE], prefix[BUF_SIZE], msg_buf[BUF_SIZE], recv_buf[BUF_SIZE];

void broadcast(struct interface_list *list, char *msg) {
	int msg_len = strlen(msg);
	int prefix_len = strlen(prefix);
	for(int i=0;i<list->len;++i) {
		struct interface *iface = &(list->data[i]);


		int tx_start = sizeof(struct ether_header);
		int tx_len = tx_start + prefix_len + msg_len + 1;
		char *payload = iface->packet + tx_start;
		payload[0] = '\0';
		strncat(payload, prefix, prefix_len);
		strncat(payload, msg, msg_len);
		payload[tx_len-1] = '\0';

		struct ether_header *eh = (struct ether_header*)(iface->packet);
		if (sendto(iface->sockfd, iface->packet, tx_len, 0, (struct sockaddr*)&(iface->socket_address), sizeof(struct sockaddr_ll)) < 0) {
			printf("Send failed\n");
			perror("sendto");
		}
	}
}

void recv_packet(struct interface_list *list) {
	//struct interface *iface = &(list->data[0]);
	int sockfd;
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
		perror("recv socket");
	}

	struct sockaddr_ll addr;
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETHER_TYPE);
	addr.sll_pkttype = PACKET_BROADCAST;
	addr.sll_hatype = ARPHRD_ETHER;
	addr.sll_ifindex = 0;
	addr.sll_halen = ETH_ALEN;

	unsigned int sz = sizeof(addr);

	//int sockopt = 1;

	//if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
	//	perror("REUSEADDR");
	//}

	//if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, iface->name, IFNAMSIZ-1) == -1)  {
	//	perror("BINDTODEVICE");
	//}

	while(1) {
		if (recvfrom(sockfd, recv_buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &sz) < 0) {
			perror("recvfrom");
		}
		char *payload = recv_buf + sizeof(struct ether_header);
		struct ether_header *eh = (struct ether_header*)(recv_buf);
		printf("<%.2x:%.2x:%.2x:%.2x:%.2x:%.2x> %s\n",
		        eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2],
		        eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5],
		        payload);
	}
}

int main(int argc, char* argv[]) {
	struct interface_list list;
	init_interfaces(&list);
	print_interfaces(&list);
	
	printf("Enter your name: ");
	fgets(name, 1024, stdin);
	name[strlen(name)-1] = '\0';
	printf("Welcome, \'%s\'!\n", name);

	prefix[0] = '\0';
	strncat(prefix, "[", 1);
	strncat(prefix, name, strlen(name));
	strncat(prefix, "]: ", 3);

	pid_t pid;
	pid = fork();
	if (pid < 0) {
		perror("fork");
	} else if (pid == 0) {
		recv_packet(&list);
	} else {
		while(1) {
			printf(">>> ");
			fgets(msg_buf, 1024, stdin);
			msg_buf[strlen(msg_buf)-1] = '\0';
			broadcast(&list, msg_buf);
		}
	}

	return 0;
}
