#include "data.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>



BLOB *blob_create(char *content, size_t size)
{

    BLOB* data = (BLOB*)calloc(1,sizeof(BLOB));
    //pthread_mutex_t lock = malloc(sizeof(pthread_mutex_t));

    //pthread_mutex_t lock;   //lock to be used to secure reference count
    if (pthread_mutex_init(&data->mutex, NULL) != 0)
    {
        debug("mutex init lock failed in blob_create");
        return NULL;
    }
    data->refcnt = 0;
    data->size = size;
    //char* contentCopy = malloc(size+1);
    //memcpy(contentCopy,content,size+1);

    data->content = content;
    //contentCopy;
    data->prefix =content;
    // contentCopy;

    debug("Create blob with content %p, size %lu -> %p",content,size,data);
    char* why = "newly created blob";
    blob_ref(data,why);


    return data;
}


BLOB *blob_ref(BLOB *bp, char *why)
{
    pthread_mutex_lock(&bp->mutex);

    bp->refcnt++;
    debug("Increase reference count on blob %p [%s] (%d -> %d) for %s",bp,bp->content,bp->refcnt-1,bp->refcnt,why);
    pthread_mutex_unlock(&bp->mutex);
    return bp;
}


void blob_unref(BLOB *bp, char *why)
{
    if(bp==NULL) return;
    pthread_mutex_lock(&bp->mutex);

    bp->refcnt--;
    debug("Decrease reference count on blob %p (%d -> %d) for %s",bp,bp->refcnt+1,bp->refcnt,why);

    pthread_mutex_unlock(&bp->mutex);
    if(!bp->refcnt)
    {
        //if refcnt reached zero

        debug("Free blob %p [%s]",bp,bp->content);
        pthread_mutex_destroy(&bp->mutex);
        //free(bp->prefix);//malloc'd
        free(bp->content);
        free(bp);
    }
    //free after mutex unlocked


}


int blob_compare(BLOB *bp1, BLOB *bp2)
{
    if(bp1==NULL || bp2==NULL) return 1;
    return strcmp(bp1->content,bp2->content);
}


int blob_hash(BLOB *bp)
{
    pthread_mutex_lock(&bp->mutex);
    int i = 0,hash = 5381;
    char* stringToHash = bp->content;
    while((*(stringToHash+i)) )//&& (*(stringToHash+i))!=32)
    {
        hash += (hash<<5) + (*(stringToHash+i));
        i++;
    }
    debug("Create hash code %d for blob %s",hash & 0x7FFFFFFF,bp->content);
    pthread_mutex_unlock(&bp->mutex);
    return (hash & 0x7FFFFFFF);
}


KEY *key_create(BLOB *bp)
{
    //key inherits reference to bp, does not need to increase refcnt

    KEY* key = (KEY*) malloc(sizeof(KEY));
    key->hash = blob_hash(bp);
    key->blob = bp;
    debug("Create key from blob %p -> %p [%s]",bp,key,bp->content);
    return key;
}

/*
 * Dispose of a key, decreasing the reference count of the contained blob.
 * A key must be disposed of only once and must not be referred to again
 * after it has been disposed.
 *
 * @param kp  The key.
 */

void key_dispose(KEY *kp)
{
    debug("Dispose of key %p [%s]",kp,kp->blob->content);
    char* why = "blob in key";
    blob_unref(kp->blob,why);
    free(kp);
    kp = NULL;
}


int key_compare(KEY *kp1, KEY *kp2)
{
    debug("%d",blob_compare(kp1->blob,kp2->blob));
    if(blob_compare(kp1->blob,kp2->blob)!=0) return 1;//if zero means it's equal
    debug("%d",(!(kp1->hash==kp2->hash)));
    return (!(kp1->hash==kp2->hash)) ;
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp)
{

    VERSION* version = (VERSION*)calloc(1,sizeof(VERSION));
    if(bp==NULL)
    {
        debug("Create NULL version of blob for transaction %u -> %p",tp->id,tp);
    }
    else
    {
        debug("Create version of blob %p [%s] for transaction %u -> %p",bp,bp->content,tp->id,tp);
    }
    version->creator = tp;
    char * why = "creator of version";
    trans_ref(tp,why);  //increase reference of transaction
    version->blob = bp;
    version->next = NULL; //not connected yet
    version->prev = NULL;
    return version;
}


void version_dispose(VERSION *vp)
{
    debug("Dispose of version %p",vp);
    char*why = "creator of version";
    trans_unref(vp->creator,why);
    if(vp->blob!=NULL)  //if null cannot unreference
        blob_unref(vp->blob,why);

    free(vp);
    vp = NULL;
}