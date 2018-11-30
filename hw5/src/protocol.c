#include "protocol.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include "csapp.h"
#include "debug.h"

#define convert(data) htonl(data)

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data)
{
    //debug("custom send packet");
    XACTO_PACKET newPkt = {pkt->type,pkt->status,pkt->null,convert(pkt->size),convert(pkt->timestamp_sec),convert(pkt->timestamp_nsec)};

    if(rio_writen(fd,&newPkt,sizeof(XACTO_PACKET))!=sizeof(XACTO_PACKET))
    {
        errno = EIO;
        return -1;    //not successful
    }
    if(pkt->size>0)   //if not NULL
    {
        if(rio_writen(fd,data,pkt->size)!=pkt->size)
        {
            errno = EIO;
            return -1;
        }
    }


    return 0;
}
int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap)
{
    debug("custom recev packet");
    XACTO_PACKET networkPkt;
    if(rio_readn(fd,&networkPkt,sizeof(XACTO_PACKET))!=sizeof(XACTO_PACKET))
    {
            return -1;
    }

    pkt->type = networkPkt.type;
    pkt->status = networkPkt.status;
    pkt->null = networkPkt.null;
    pkt->size = ntohl(networkPkt.size);
    pkt->timestamp_sec = ntohl(networkPkt.timestamp_sec);
    pkt->timestamp_nsec = ntohl(networkPkt.timestamp_nsec);

    if(pkt->size>0)
    {
        void * data = malloc(sizeof(char)*pkt->size);
        if(rio_readn(fd,data,pkt->size)!=pkt->size)
        {
                free(data);
                return -1;
        }
        *datap = data;
    }




    return 0;
}