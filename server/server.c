#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_signal.h"
#include "my_socket.h"
#include "accept_connection.h"
#include "get_num.h"
#include "bz_usleep.h"
#include "logUtil.h"

int debug = 0;
int enable_quick_ack = 0;
int set_so_sndbuf_size = 0;

int print_result(struct timeval start, struct timeval stop, int so_snd_buf, unsigned long long send_bytes)
{
    struct timeval diff;
    double elapse;

    timersub(&stop, &start, &diff);
    fprintfwt(stderr, "server: SO_SNDBUF: %d (final)\n", so_snd_buf);
    elapse = diff.tv_sec + 0.000001*diff.tv_usec;
    fprintfwt(stderr, "server: %.3f MB/s ( %lld bytes %ld.%06ld sec )\n",
        (double) send_bytes / elapse  / 1024.0 / 1024.0,
        send_bytes, diff.tv_sec, diff.tv_usec);

    return 0;
}

int child_proc(int connfd, int bufsize, int sleep_usec)
{
    int n;
    unsigned char *buf;
    struct timeval start, stop;
    unsigned long long send_bytes = 0;
    size_t buf_head_num = 0;

    buf = malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc sender buf in child_proc");
    }
    //for (size_t i = 0; i < bufsize; ++i) {
    //    buf[i] = (i % 256);
    //}

    if (set_so_sndbuf_size > 0) {
        set_so_sndbuf(connfd, set_so_sndbuf_size);
    }

    int so_snd_buf = get_so_sndbuf(connfd);
    fprintfwt(stderr, "server: SO_SNDBUF: %d (init)\n", so_snd_buf);

    gettimeofday(&start, NULL);
    for ( ; ; ) {
        if (enable_quick_ack) {
            int qack = 1;
            setsockopt(connfd, IPPROTO_TCP, TCP_QUICKACK, &qack, sizeof(qack));
        }
        for (size_t i = 0; i < bufsize; ++i) {
            buf[i] = ((buf_head_num + i) % 256);
        }
        buf_head_num += bufsize;
        n = write(connfd, buf, bufsize);
        if (n < 0) {
            gettimeofday(&stop, NULL);
            int so_snd_buf = get_so_sndbuf(connfd);
            print_result(start, stop, so_snd_buf, send_bytes);
            if (errno == ECONNRESET) {
                fprintfwt(stderr, "server: connection reset by client\n");
                break;
            }
            else if (errno == EPIPE) {
                fprintfwt(stderr, "server: connection reset by client\n");
                break;
            }
            else {
                err(1, "write");
            }
        }
        else if (n == 0) {
            warnx("write returns 0");
            exit(0);
        }
        else {
            send_bytes += n;
            fprintfwt(stderr, "server: write: %d bytes\n", n);
        }
        if (sleep_usec > 0) {
            bz_usleep(sleep_usec);
        }
    }
    return 0;
}

void sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        ;
    }
    return;
}

int usage(void)
{
    char *msg =
"Usage: server [-b bufsize (1460)] [-s sleep_usec (0)] [-q] [-S so_sndbuf]\n"
"-b bufsize:    one send size (may add k for kilo, m for mega)\n"
"-s sleep_usec: sleep useconds after write\n"
"-q:            enable quick ack\n"
"-S: so_sndbuf: set socket send buffer size\n";

    fprintf(stderr, msg);

    return 0;
}

int main(int argc, char *argv[])
{
    int port = 1234;
    pid_t pid;
    struct sockaddr_in remote;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int listenfd;
    int c;
    int bufsize = 1460;
    int sleep_usec = 0;

    while ( (c = getopt(argc, argv, "b:dhqs:S:")) != -1) {
        switch (c) {
            case 'b':
                bufsize = get_num(optarg);
                break;
            case 'd':
                debug += 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'q':
                enable_quick_ack = 1;
                break;
            case 's':
                sleep_usec = get_num(optarg);
                break;
            case 'S':
                set_so_sndbuf_size = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    my_signal(SIGCHLD, sig_chld);
    my_signal(SIGPIPE, SIG_IGN);

    listenfd = tcp_listen(port);
    if (listenfd < 0) {
        errx(1, "tcp_listen");
    }

    for ( ; ; ) {
        int connfd = accept(listenfd, (struct sockaddr *)&remote, &addr_len);
        if (connfd < 0) {
            err(1, "accept");
        }
        
        pid = fork();
        if (pid == 0) { //child
            if (close(listenfd) < 0) {
                err(1, "close listenfd");
            }
            if (child_proc(connfd, bufsize, sleep_usec) < 0) {
                errx(1, "child_proc");
            }
            exit(0);
        }
        else { // parent
            if (close(connfd) < 0) {
                err(1, "close for accept socket of parent");
            }
        }
    }
        
    return 0;
}
