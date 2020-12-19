//SERVER
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define UTC_NTP 2208988800U

void gettime64(uint32_t ts[])
{
        struct timeval tv;
        gettimeofday(&tv, NULL);

        ts[0] = tv.tv_sec + UTC_NTP;
        ts[1] = (4294*(tv.tv_usec)) + ((1981*(tv.tv_usec))>>11);
}

void ReqArrive(uint32_t *ntp_time)
{
        time_t tm;

        if (ntp_time)
		{
            tm = *ntp_time - UTC_NTP;
        } else
		{
            tm = time(NULL);
        }
         printf("\t=== A request from Client comes at : ===\n\t\t%s\t\n", ctime(&tm));
}

int ntp_reply(
        int socket_fd,
        struct sockaddr *saddr_p,
        socklen_t saddrlen,
        unsigned char recv_buf[],
        uint32_t recv_time[]){

        /* Assume that recv_time is in local endian ! */
        unsigned char send_buf[48];
        uint32_t *u32p;

        if ((recv_buf[0] & 0x07) != 0x3) {
				puts("Invalid request: found error at the first byte");
                return 1;
        }

        send_buf[0] = (recv_buf[0] & 0x38) + 4;
        send_buf[1] = 0x01;

        *(uint32_t*)&send_buf[12] = htonl(0x4C4F434C);

        send_buf[2] = recv_buf[2];

        send_buf[3] = (signed char)(-6);

        u32p = (uint32_t *)&send_buf[4];
        *u32p++ = 0;
        *u32p++ = 0;

        u32p++;

        gettime64(u32p);
        *u32p = htonl(*u32p - 60); 
        u32p++;
        *u32p = htonl(*u32p);   
        u32p++;

        /* Originate Time = Transmit Time @ Client */
        *u32p++ = *(uint32_t *)&recv_buf[40];
        *u32p++ = *(uint32_t *)&recv_buf[44];

        /* Receive Time @ Server */
        *u32p++ = htonl(recv_time[0]);
        *u32p++ = htonl(recv_time[1]);

        /* Transmit Time*/
        gettime64(u32p);
        *u32p = htonl(*u32p);   
        u32p++;
        *u32p = htonl(*u32p);   

		if ( sendto( socket_fd,
                     send_buf,
                     sizeof(send_buf), 0,
                     saddr_p, saddrlen)< 48)
		{
                perror("sendto error");
                return 1;
        }
        return 0;
}

void coming_request(int fd)
{
        struct sockaddr src_addr;
        socklen_t src_addrlen = sizeof(src_addr);
        unsigned char buf[48];
        uint32_t recv_time[2];
        pid_t pid;

        while (1) {
                while (recvfrom(fd, buf,
                                48, 0,
                                &src_addr,
                                &src_addrlen)< 48 );

                gettime64(recv_time);
                /* recv_time in local endian */
                ReqArrive(recv_time);

                pid = fork();
                if (pid == 0) {
                        /* Child */
                        ntp_reply(fd, &src_addr , src_addrlen, buf, recv_time);
                        exit(0);
                } 
				else if (pid == -1) {
                        perror("fork() error");
                }
                /* return to parent */
        }
}

void waitsig()
{
	int w;
	wait(&w);
}

int main(int argc, char *argv[], char **env)
{
	int socket_desc;
        struct sockaddr_in sinaddr;

        signal(SIGCHLD,waitsig);
        
        socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_desc == -1) {
            perror("Can not create socket.");
        }

        memset(&sinaddr, 0, sizeof(sinaddr));
        sinaddr.sin_family = AF_INET;
        sinaddr.sin_port = htons(8123);
        sinaddr.sin_addr.s_addr = INADDR_ANY;

        if (0 != bind(socket_desc, (struct sockaddr *)&sinaddr, sizeof(sinaddr))) {
                perror("Bind error");
        }
        puts(  "\n\t========================================\t\n"
               "\t= Server started, waiting for requests =\t\n"
               "\t========================================\t\n");

        coming_request(socket_desc);
        close(socket_desc);
        return 0;
}
