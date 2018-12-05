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

int sendReplyPacket(int fd,int status)
{
    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);

    //XACTO_PACKET pkt = {XACTO_REPLY_PKT,status,0,0,currentTime.tv_sec,currentTime.tv_usec};
    int success = 0;
    //success = proto_send_packet(fd, &pkt, NULL);

    XACTO_PACKET* pkt = calloc(1,sizeof(XACTO_PACKET));
    pkt->type = XACTO_REPLY_PKT;
    pkt->status = status;
    pkt->null = 0;
    pkt->size = 0;
    pkt->timestamp_sec = currentTime.tv_sec;
    pkt->timestamp_nsec = currentTime.tv_usec;
    //{XACTO_DATA_PKT,status,!size,size,currentTime.tv_sec,currentTime.tv_usec};
    success = proto_send_packet(fd, pkt, NULL);
    free(pkt);
    return success;
}
int sendDataPacket(int fd,BLOB*blob,int status)
{

    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);
    int size = 0;
    void* content = NULL;
    if(blob==NULL)
    {
        size = 0;
        content = NULL;
    }
    else
    {
        size = blob->size;
        content = blob->content;
    }


    XACTO_PACKET* pkt = calloc(1,sizeof(XACTO_PACKET));
    pkt->type = XACTO_DATA_PKT;
    pkt->status = status;
    pkt->null = !size;
    pkt->size = size;
    pkt->timestamp_sec = currentTime.tv_sec;
    pkt->timestamp_nsec = currentTime.tv_usec;
    //{XACTO_DATA_PKT,status,!size,size,currentTime.tv_sec,currentTime.tv_usec};
    int success;
    success = proto_send_packet(fd, pkt, content);
    free(pkt);
    return success;
}
KEY* retrieveKey(int fd,XACTO_PACKET* pkt,void**data)
{
    int key = 0;
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
    void ** data = (void**)calloc(1,sizeof(char*));
    void ** value = (void**)calloc(1,sizeof(char*));
    XACTO_PACKET *pkt = calloc(1,sizeof(XACTO_PACKET));
    BLOB** valueBlob = (BLOB**)calloc(1,sizeof(BLOB*));

    //while(transaction->status == TRANS_PENDING)
    // int commit = 0;
    while(transaction->status == TRANS_PENDING)
    {


        int check = proto_recv_packet(fd,pkt,NULL);
        if(check==-1)
        {

            debug("client service err");
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
                //free(*data);

                // free(pkt);
                sendReplyPacket(fd,transaction->status);


            }
            else if(pkt->type == XACTO_GET_PKT)
            {

                debug("[%d] GET packet received",fd);
                KEY* hashKey = retrieveKey(fd,pkt,data);
                if(hashKey==NULL) break;

                transaction->status = store_get(transaction, hashKey, valueBlob);


                sendReplyPacket(fd,transaction->status);
                sendDataPacket(fd,*valueBlob,transaction->status);
                char*why = "obtained from store_get";
                blob_unref(*valueBlob,why);
                //free(*valueBlob);


            }
            else//commit
            {
                debug("[%d] COMMIT packet received",fd);

                //creg_unregister(client_registry, fd);
                transaction->status = trans_commit(transaction);
                sendReplyPacket(fd,transaction->status);
                //char*why = "attempting to commit transaction";
                //trans_unref(transaction, why);

                //commit = 1;
            }
        }
        store_show();
        trans_show_all();

    }

    //if commits or aborts, close client connection
    free(pkt);
    free(data);
    free(valueBlob);
    free(value);
    debug("[%d] Ending client service",fd);
    creg_unregister(client_registry, fd);
    close(fd);




    return 0;   //thread terminates
}