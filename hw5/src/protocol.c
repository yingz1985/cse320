#include "protocol.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include "csapp.h"
#include "debug.h"

#define convert(data) htonl(data)

int proto_send_packet(int fd, XACTO_PACKET *pkt, void *data)
{
    debug("sending packet");
    //XACTO_PACKET newPkt = {pkt->type,pkt->status,pkt->null,convert(pkt->size),convert(pkt->timestamp_sec),convert(pkt->timestamp_nsec)};

    //XACTO_PACKET* mpkt = malloc(sizeof(XACTO_PACKET));
    //memcpy(mpkt,pkt,sizeof(XACTO_PACKET));
    int size = pkt->size;
    pkt->size = convert(pkt->size);
    pkt->timestamp_sec = convert(pkt->timestamp_sec);
    pkt->timestamp_nsec = convert(pkt->timestamp_nsec);
    int success = 0;
    if(rio_writen(fd,pkt,sizeof(XACTO_PACKET))!=sizeof(XACTO_PACKET))
    {
        //errno = EIO;
        success = -1;
        return success;    //not successful
    }
    if(size>0)   //if not NULL
    {
        if(rio_writen(fd,data,size)!=size)
        {
           // errno = EIO;
            success = -1;
            return success;
        }
    }


    return success;
}
int proto_recv_packet(int fd, XACTO_PACKET *pkt, void **datap)
{
    debug("recv packet from %d",fd);
    //XACTO_PACKET networkPkt;
    int success = 0;
    if(rio_readn(fd,pkt,sizeof(XACTO_PACKET))!=sizeof(XACTO_PACKET))
    {
        success = -1;
        return success;
    }

    // pkt->type = networkPkt.type;
    // pkt->status = networkPkt.status;
    // pkt->null = networkPkt.null;
     pkt->size = ntohl(pkt->size);
     pkt->timestamp_sec = ntohl(pkt->timestamp_sec);
     pkt->timestamp_nsec = ntohl(pkt->timestamp_nsec);

    if(pkt->size>0)
    {
        void * data = calloc((pkt->size+1),sizeof(char));
        if(rio_readn(fd,data,pkt->size)!=pkt->size)
        {
                free(data);
                success = -1;
                return success;
        }
        *datap = data;
    }




    return success;
}