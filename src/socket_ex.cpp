/*************************************************************************
	> File Name: socket_ex.cpp
	> Author: 
	> Mail: 
	> Created Time: Sun 30 Oct 2016 04:22:42 AM EDT
 ************************************************************************/

#include "socket_ex.h"

#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <string>
#include<iostream>
using namespace std;

string GetLocalIP()    
{          
	int MAXINTERFACES=16;    
    char retIP[50];
    char condIP[50];
    retIP[0] = '\0';
    condIP[0] = '\0';
    //if(retIP[0] != '\0') return retIP;
	const char *ip = "127.0.0.1";    
	int fd, intrface;      
	struct ifreq buf[MAXINTERFACES];      
	struct ifconf ifc;      
	int firstNum = 0;
    int thrdNum = 0;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)      
	{      
		ifc.ifc_len = sizeof(buf);      
		ifc.ifc_buf = (caddr_t)buf;      
		if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc))      
		{      
			intrface = ifc.ifc_len / sizeof(struct ifreq);      
			while (intrface-- > 0)      
			{      
				if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface])))      
				{      
					ip=(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));      
					//fetch management ip. determined by wheath the third part is even.
					if(strcmp(ip, "127.0.0.1") == 0) continue;
					if(sscanf(ip, "%d.%*d.%d.%*d", &firstNum, &thrdNum) != 2){ continue; }
                    strncpy(condIP, ip, 50);
                    if(firstNum == 10 && thrdNum % 2 == 0){
                      strncpy(retIP, ip, 50);
                    }
				}                          
			}    
		}      
		close (fd);
	} 
    if(retIP[0] != '\0') return retIP;
    else return condIP;
} 

string GetLocalIPByIF(const char *eth)
{          
    char retIP[50];
    retIP[0] = '\0';
	const char *ip = "127.0.0.1";    
	int fd;      
    struct ifreq ifr;
    strncpy(ifr.ifr_name, eth, IF_NAMESIZE);
    ifr.ifr_name[IF_NAMESIZE - 1] = '\0';

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)      
	{      
        if (ioctl (fd, SIOCGIFADDR, (char *) &ifr) == 0)      
        {      
            ip=(inet_ntoa(((struct sockaddr_in*)(&ifr.ifr_addr))->sin_addr));      
            strncpy(retIP, ip, 50);
        }
        else{
            fprintf(stderr, "failed to fetch eth divce addr info. eth: %s.\n", eth);
        }
		close (fd);
	} 
    if(retIP[0] != '\0') return retIP;
    else return GetLocalIP();
} 
string addr2str(const struct sockaddr *addr)
{
    const struct sockaddr_in *addrin = reinterpret_cast<const struct sockaddr_in*>(addr);
    char szIp[50];
    char szAddr[50];
    szAddr[0] = '\0';
    const char *retSz = inet_ntop(AF_INET, &addrin->sin_addr.s_addr, szIp, 50);
    if(retSz == NULL){
        fprintf(stderr, "fail to express addr as string.\n");
        return szAddr;
    }
    sprintf(szAddr, "%s:%d", szIp, ntohs(addrin->sin_port));

    return szAddr;
}

const struct sockaddr_in str2addr(const char* szIp, unsigned short port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    errno = 0;
    int err = inet_pton(AF_INET, szIp, &addr.sin_addr.s_addr);
    if(err != 1){
        fprintf(stderr, "fail to form sockaddr_in, src: %s, err: %d.\n", szIp, err);
    }

    return addr;
}

int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen)
{
	int fd, err;
	int reuse = 1;
	if((fd = socket(addr->sa_family, type, 0)) < 0) return -1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) goto errout;
	if(bind(fd, addr, alen) < 0) goto errout;
	if(type == SOCK_STREAM || type == SOCK_SEQPACKET){
		if(listen(fd, qlen) < 0) goto errout;
	}
	return fd;
errout:
	err = errno;
	close(fd);
	errno = err;
	return -1;
}

