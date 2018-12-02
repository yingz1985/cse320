#include "data.h"
#include "debug.h"
#include <string.h>


BLOB *blob_create(char *content, size_t size)
{

    BLOB* data = (BLOB*)malloc(sizeof(BLOB));
    //pthread_mutex_t lock;   //lock to be used to secure reference count
    if (pthread_mutex_init(&data->mutex, NULL) != 0)
    {
        debug("mutex init lock failed in blob_create");
        return NULL;
    }
    data->refcnt = 0;
    data->size = size;
    data->content = content;
    data->prefix = content;

    debug("Create blob with content %p, size %lu -> %p",content,size,data);
    char* why = "newly created blob";
    blob_ref(data,why);


    return data;
}


BLOB *blob_ref(BLOB *bp, char *why)
{
    pthread_mutex_lock(&bp->mutex);

    bp->refcnt++;
    debug("Increase reference count on blob %p (%d -> %d) for %s",bp,bp->refcnt-1,bp->refcnt,why);
    pthread_mutex_unlock(&bp->mutex);
    return bp;
}


void blob_unref(BLOB *bp, char *why)
{
    pthread_mutex_lock(&bp->mutex);
    debug("%s",why);
    bp->refcnt--;
    debug("Decrease reference count on blob %p (%d -> %d) for %s",bp,bp->refcnt+1,bp->refcnt,why);
    pthread_mutex_unlock(&bp->mutex);
    //free after mutex unlocked
    if(!bp->refcnt)
    {
        //if refcnt reached zero
        debug("Free blob %p [%s]",bp,bp->content);
        free(bp);
    }

}


int blob_compare(BLOB *bp1, BLOB *bp2)
{
    return strcmp(bp1->content,bp2->content);
}


int blob_hash(BLOB *bp)
{

    int i = 0,hash = 5381;
    char* stringToHash = bp->content;
    while((*(stringToHash+i)))
    {
        hash += (hash<<5) + (*(stringToHash+i));
        i++;
    }

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
    return ((kp1->hash==kp2->hash) && (!blob_compare(kp1->blob,kp2->blob)));
}

VERSION *version_create(TRANSACTION *tp, BLOB *bp)
{
    debug("Create version of blob %p [b] for transaction %u -> %p",bp,tp->id,tp);
    VERSION* version = (VERSION*)malloc(sizeof(VERSION));
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
    blob_unref(vp->blob,why);
    free(vp);
    vp = NULL;
}