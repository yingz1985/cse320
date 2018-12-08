#include "debug.h"
#include "client_registry.h"
#include "transaction.h"
#include "store.h"
#include <getopt.h>
#include <string.h>
#include <csapp.h>

#include <server.h>


static void terminate(int status);

CLIENT_REGISTRY *client_registry;
int listenfd;

void sighup(int sig)
{
    //printf("sig hang up\n");
    terminate(EXIT_SUCCESS);
}

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    char optval;
    //char * hostname = NULL;
    char *portNum = NULL;
    int port = -1;
    int * connfd;
    socklen_t client;
    while(optind < argc)
    {
        if((optval = getopt(argc, argv, "p:")) != -1) {
            switch(optval) {
            /*case 'h':
                hostname = malloc(sizeof(char)*(strlen(optarg)+1));
                strncpy(hostname,optarg,strlen(optarg)+1);  //plus null terminator
                //hostname = optarg;
                printf("hostname: %s",hostname);
                break;*/
            case 'p':
                for(int i = 0;i<strlen(optarg);i++)
                {
                    if(!(*(optarg+i)>='0' && *(optarg+i)<='9'))
                    {
                        //not a number;
                        fprintf(stderr, "invalid port %s\n",optarg);
                        exit(EXIT_FAILURE);
                    }
                }
                port = atoi(optarg);
                portNum = malloc(sizeof(char)*(strlen(optarg)+1));
                strncpy(portNum,optarg,strlen(optarg)+1);
                printf("via port %d",port);
                break;
            /*case 'q':
                printf("reading from file");
                break;*/

            case '?':
              fprintf(stderr, "Usage: %s [-h <hostname>] [-p <port>] [-q <filename>]\n", argv[0]);
              exit(EXIT_FAILURE);
              break;
            default:
            break;
            }
        }
        else
        {
            break;
        }
    }
    if(port==-1)
    {
        fprintf(stderr, "A port number is required\n" );
        exit(EXIT_FAILURE);
    }

    // Perform required initializations of the client_registry,
    // transaction manager, and object store.
    client_registry = creg_init();
    trans_init();
    store_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function xacto_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.
    listenfd = Open_listenfd(portNum);
    struct sockaddr_storage clientaddr;
    pthread_t threadid;
    Signal(SIGHUP,sighup);
    free(portNum);
    while(1)
    {

        client = sizeof(struct sockaddr_storage);
        connfd = Malloc(sizeof(int));
        *connfd = Accept(listenfd, (SA*)&clientaddr,&client);
        Pthread_create(&threadid,NULL,xacto_client_service,connfd);




    }

    fprintf(stderr, "You have to finish implementing main() "
	    "before the Xacto server will function.\n");

    terminate(EXIT_FAILURE);
}

/*
 * Function called to cleanly shut down the server.
 */
void terminate(int status) {
    // Shutdown all client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    debug("Waiting for service threads to terminate...");
    creg_wait_for_empty(client_registry);
    debug("All service threads terminated.");

    // Finalize modules.
    creg_fini(client_registry);
    trans_fini();
    store_fini();

    shutdown(listenfd,SHUT_RD);
    close(listenfd);

    debug("Xacto server terminating");
    exit(status);
}
