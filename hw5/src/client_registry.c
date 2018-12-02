#include "client_registry.h"
#include <stdint.h>
#include "csapp.h"
#include "debug.h"
#include <sys/socket.h>
#include <semaphore.h>


typedef struct client_registry
{
    uint64_t fd_bits[16];

}CLIENT_REGISTRY;   //an array of 1024 bits

CLIENT_REGISTRY * client_registry;


static int numClients;
int closing;
sem_t sem;

pthread_mutex_t lock;

CLIENT_REGISTRY *creg_init()
{
    numClients = 0;
    closing = 0;

    client_registry= (CLIENT_REGISTRY*)Calloc(1,sizeof(CLIENT_REGISTRY));
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        debug("mutex init lock failed");
        return NULL;
    }


    //initially a client registry with all zero's - no file descriptor used by client sock
    return client_registry;
}
void creg_fini(CLIENT_REGISTRY *cr)
{
    pthread_mutex_destroy(&lock);
    free(cr);
    cr = NULL;
}
void creg_register(CLIENT_REGISTRY *cr, int fd)
{
    debug("file desc %d",fd);
    pthread_mutex_lock(&lock);
    numClients++;
    int fd_pos = fd-3;  //0,1,2 are reserved for stdin,stdout,and stderr
    //lock access to client registry
    int array_num = fd_pos/64;
    long bit_index = 0x1<<(fd_pos%64);
    cr->fd_bits[array_num]|=bit_index;

    debug("array:%d, bit %lu set %lu",array_num,bit_index,cr->fd_bits[array_num]);

    pthread_mutex_unlock(&lock);
}


void creg_unregister(CLIENT_REGISTRY *cr, int fd)
{
    pthread_mutex_lock(&lock);
    debug("file desc %d",fd);
    numClients--;

    if(!numClients && closing)//has no more clients
    {
        if(sem_post(&sem)<0)
        {
            debug("semaphore post failed");
            exit(0);
        }
    }

    int fd_pos = fd-3;
    int array_num = fd_pos/64;
    long bit_index = 0x1<<(fd_pos%64);
    cr->fd_bits[array_num]^=bit_index;  //flip bit

    debug("array:%d, bit %lu set %lu",array_num,bit_index,cr->fd_bits[array_num]);

    pthread_mutex_unlock(&lock);
}

void creg_wait_for_empty(CLIENT_REGISTRY *cr)
{
    //if(!numClients) numClients++;
    debug("%d clients",numClients);
    if(numClients)
    {
        if(sem_init(&sem,0,0)<0)
        {
            debug("semaphore failed");
            exit(0);
        }
        else
        {
            if(sem_wait(&sem)<0)
            {
                debug("semaphore wait failed");
                exit(0);
            }
        }
    }
}

void creg_shutdown_all(CLIENT_REGISTRY *cr)
{
    pthread_mutex_lock(&lock);
    closing = 1;
    if(numClients)
    {
        int pending = numClients;
        for(int i = 0 ;i<16;i++)
        {
            if(!pending) break;
            for(int k = 0;k<64;k++)
            {
                if(!pending) break;
                if((cr->fd_bits[i] & (0x1<<k)) && pending)//if file descriptor open
                {
                    shutdown(((i*64)+k+3),SHUT_RD);
                    debug("closing socket %d",((i*64)+k+3));
                    if(errno!=0)
                        debug("something went wrong");
                    pending--;
                }
            }
        }
    }
    pthread_mutex_unlock(&lock);
}
