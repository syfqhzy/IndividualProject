//CLIENT
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
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

int useage(const char *path)
{
        printf("Useage:\n\t%s <server address>\n", path);
        return 1;
}


int opnCnn(const char* server)
{
        int socket_desc;
        struct addrinfo *saddr;
        socket_desc = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_desc == -1) {
                puts("Can not create socket.\n");
        }

        if (0 != getaddrinfo(server, "8123", NULL, &saddr)) {
                puts("Server address not correct.\n");
        }

        if (connect(socket_desc, saddr->ai_addr, saddr->ai_addrlen) != 0) {
                puts("Connect error\n");
        }

        freeaddrinfo(saddr);

        return socket_desc;
}
/* cc= jagd (C.Wu) */	
void req(int fd)
{
        unsigned char buf[48] = {0};
        uint32_t tts[2]; /* Transmit Timestamp */

        buf[0] = 0x23;

        gettime64(tts);
        (*(uint32_t *)&buf[40]) = htonl(tts[0]);
        (*(uint32_t *)&buf[44])= htonl(tts[1]);
        if (send(fd, buf, 48, 0) !=48 ) {
                puts("Send error\n");
        }
}	

/* cc= jagd (C.Wu) */	
void reply(int fd)
{
        unsigned char buf[48];
        uint32_t *pt;
        uint32_t t1[2]; /* t1 = Originate Timestamp  */
        uint32_t t2[2]; /* t2 = Receive Timestamp @ Server */
        uint32_t t3[2]; /* t3 = Transmit Timestamp @ Server */
        uint32_t t4[2]; /* t4 = Receive Timestamp @ Client */
        double T1, T2, T3, T4;
        double tfrac = 4294967296.0;
        time_t currtm;
        time_t diff_sec;

        if (recv(fd, buf, 48, 0) < 48) {
                puts("Receive error\n");
        }
        gettime64(t4);
        pt = (uint32_t *)&buf[24];

        t1[0] = htonl(*pt++);
        t1[1] = htonl(*pt++);

        t2[0] = htonl(*pt++);
        t2[1] = htonl(*pt++);

        t3[0] = htonl(*pt++);
        t3[1] = htonl(*pt++);

        T1 = t1[0] + t1[1]/tfrac;
        T2 = t2[0] + t2[1]/tfrac;
        T3 = t3[0] + t3[1]/tfrac;
        T4 = t4[0] + t4[1]/tfrac;

        printf( "\t\tDelay: %lf " " Offset: %lf\n",
                (T4-T1) - (T3-T2),((T2 - T1) + (T3 - T4)) /2 );

        diff_sec = ((int32_t)(t2[0] - t1[0]) + (int32_t)(t3[0] - t4[0])) /2;
        currtm = time(NULL) - diff_sec;
        printf("\t\t==   Current Time at Server:   ==\n\t\t%s \n", ctime(&currtm));
}

int client(const char* server)
{
        int fd;

        fd = opnCnn(server);
        req(fd);
		puts(  "\n\t\t=================================\t\n"
               "\t\t=   Getting reply from SERVER   =\t\n"
               "\t\t=================================\t\n");

        reply(fd);
        close(fd);

        return 0;
}

int main(int argc, char *argv[], char **env)
{
        if (argc < 2) {
                return useage(argv[0]);
        }

        return client(argv[1]);
}

