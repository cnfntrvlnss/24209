/*************************************************************************
	> File Name: socket_ex.h
	> Author: 
	> Mail: 
	> Created Time: Sun 30 Oct 2016 04:22:56 AM EDT
 ************************************************************************/

#ifndef _SOCKET_EX_H
#define _SOCKET_EX_H

#include <netinet/in.h>
#include <string>

const struct sockaddr_in str2addr(const char* szIp, unsigned short port = 0);
std::string addr2str(const struct sockaddr *addr);
//int initTcpServer(const char* szIp, unsigned short port);
int initserver(int type, const struct sockaddr *addr, socklen_t alen, int qlen);

std::string GetLocalIP();
std::string GetLocalIPByIF(const char *eth);

#endif
