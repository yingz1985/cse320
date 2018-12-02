#include "server.h"
#include <stdint.h>
#include "client_registry.h"
#include <stdlib.h>
#include <pthread.h>
#include "transaction.h"
#include "debug.h"
#include <unistd.h>
#include "protocol.h"
#include "data.h"
#include "store.h"
#include <sys/time.h>


CLIENT_REGISTRY * client_registry;

void sendReplyPacket(int fd)
{
    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);

    XACTO_PACKET pkt = {XACTO_REPLY_PKT,0,0,0,currentTime.tv_sec,currentTime.tv_usec};
    proto_send_packet(fd, &pkt, NULL);
}
void sendDataPacket(int fd,BLOB*blob)
{

    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);
    int size;
    if(!blob->size)
        size = 0;
    else
        size = blob->size;

    XACTO_PACKET pkt = {XACTO_DATA_PKT,0,!size,size,currentTime.tv_sec,currentTime.tv_usec};
    proto_send_packet(fd, &pkt, blob->content);

}
KEY* retrieveKey(int fd,XACTO_PACKET* pkt,void**data)
{
    int key;
    int check = proto_recv_packet(fd,pkt,data);
                //data packet
    if(check==-1)
    {

        debug("DATA key packet err");
        return NULL;
    }
    else
    {
        key = pkt->size;
        debug("[%d] Received key, size %d",fd,pkt->size);
    }
        BLOB* keyBlob = blob_create(*data, key);
        KEY * hashKey = key_create(keyBlob);
    return hashKey;
}
/*
 * Thread function for the thread that handles client requests.
 *
 * @param  Pointer to a variable that holds the file descriptor for
 * the client connection.  This pointer must be freed once the file
 * descriptor has been retrieved.
 */
void *xacto_client_service(void *arg)
{
    debug("starting client service");
    int fd = *(int*)arg;

    creg_register(client_registry, fd);
    pthread_detach(pthread_self()); //always succeeds, doesnt need to check return val
    free(arg);

    TRANSACTION * transaction = trans_create();
    void ** data = (void**)malloc(sizeof(char*)*1);
    void ** value = (void**)malloc(sizeof(char*)*1);
    XACTO_PACKET *pkt = malloc(sizeof(XACTO_PACKET));

    while(transaction->status == TRANS_PENDING)
    {


        int check = proto_recv_packet(fd,pkt,NULL);
        if(check==-1)
        {

            debug("PUT packet err");
            break;
        }
        else
        {

            if(pkt->type == XACTO_PUT_PKT)
            {

                int key,val;
                debug("[%d] PUT packet received",fd);
                int check = proto_recv_packet(fd,pkt,data);
                //data packet
                if(check==-1)
                {

                    //something done fucked up
                    debug("DATA key packet err");
                    break;
                }
                else
                {
                    key = pkt->size;
                    debug("[%d] Received key, size %d",fd,pkt->size);

                }

                check = proto_recv_packet(fd,pkt,value);
                if(check==-1)
                {
                    //free(pkt);
                    debug("DATA value packet err");
                    break;
                }
                else
                {
                    val = pkt->size;
                    debug("[%d] Received value, size %d",fd,pkt->size);
                }
                BLOB* keyBlob = blob_create(*data, key);
                KEY * hashKey = key_create(keyBlob);//create a hash key from key blob
                BLOB* valueBlob = blob_create(*value,val);
                transaction->status = store_put(transaction, hashKey, valueBlob);

                // free(pkt);
                sendReplyPacket(fd);


            }
            else if(pkt->type == XACTO_GET_PKT)
            {

                debug("[%d] GET packet received",fd);
                KEY* hashKey = retrieveKey(fd,pkt,data);
                if(hashKey==NULL) break;
                BLOB** valueBlob = (BLOB**)malloc(sizeof(BLOB*)*1);
                transaction->status = store_get(transaction, hashKey, valueBlob);


                sendReplyPacket(fd);
                sendDataPacket(fd,*valueBlob);

            }
            else//commit
            {
                debug("[%d] Ending client service",fd);
                break;
            }
        }
        store_show();
        trans_show_all();
    }

    //if commits or aborts, close client connection
    free(pkt);
    free(data);
    free(value);
    creg_unregister(client_registry, fd);
    close(fd);




    return 0;   //thread terminates
}