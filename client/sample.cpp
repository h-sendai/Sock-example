#include <sys/ioctl.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

#include <stdlib.h>
#include "Sock.h"
#include "logUtil.h"

using namespace std;

int usage()
{
    char msg[] = "Usage: sample ip_addr port bufsize";
    cerr << msg << endl;

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        usage();
        exit(1);
    }

    string ip_addr = argv[1];
    int    port    = strtol(argv[2], NULL, 0);
    int    bufsize = strtol(argv[3], NULL, 0);

    unsigned char *buf = new unsigned char[bufsize];

    DAQMW::Sock *m_sock = new DAQMW::Sock();
    m_sock->connect(ip_addr, port);
    int sockfd = m_sock->getSockFd();
    fprintfwt(stderr, "sockfd: %d\n", sockfd);

    buf = new unsigned char[bufsize];

    for ( ; ; ) {
        int nbytes;
        ioctl(sockfd, FIONREAD, &nbytes);
        fprintfwt(stderr, "data in socket recv buf: %d\n", nbytes);

        int n = m_sock->readAll(buf, bufsize);
        if (n == DAQMW::Sock::ERROR_FATAL) {
            fprintfwt(stderr, "DAQMW::Sock::ERROR_FATAL\n");
            exit(1);
        }
        else if (n == DAQMW::Sock::ERROR_TIMEOUT) {
            fprintfwt(stderr, "DAQMW::Sock::ERROR_TIMEOUT\n");
        }
        else if (n == DAQMW::Sock::ERROR_NOTSAMESIZE) {
            fprintfwt(stderr, "DAQMW::Sock::ERROR_NOTSAMESIZE\n");
        }
        else {
            fprintfwt(stderr, "no error. read done\n");
        }
    }


    delete [] buf;
    return 0;
}
