#include "data.h"

#include "transaction.h"
#include "store.h"
#include "debug.h"


struct map table;
// /*
//  * A map entry represents one entry in the map.
//  * Each map entry contains associated key, a singly linked list of versions,
//  * and a pointer to the next entry in the same bucket.
//  */
// typedef struct map_entry {
//     KEY *key;
//     VERSION *versions;
//     struct map_entry *next;
// } MAP_ENTRY;


//  * The map is an array of buckets.
//  * Each bucket is a singly linked list of map entries whose keys all hash
//  * to the same location.

// struct map {
//     MAP_ENTRY **table;      // The hash table.
//     int num_buckets;        // Size of the table.
//     pthread_mutex_t mutex;  // Mutex to protect the table.
// } the_map;




/*
 * Initialize the store.
 */
void store_init(void)
{
    debug("Initialize object store");
    table.table = calloc(NUM_BUCKETS,sizeof(MAP_ENTRY*));
    if(table.table==NULL)
        exit(EXIT_FAILURE);
    table.num_buckets = NUM_BUCKETS;
    if ( pthread_mutex_init(&table.mutex, NULL ) == -1 )
    {
        exit(EXIT_FAILURE);    //failed
    }

}

/*
 * Finalize the store.
 */
void store_fini(void)
{
    debug("Finalize object store");
    for(int i = 0;i<table.num_buckets;i++)
    {

        MAP_ENTRY * cursor = table.table[i];//first map entry
        while(cursor)
        {
            MAP_ENTRY* next = cursor->next;
            key_dispose(cursor->key);
            VERSION* version = cursor->versions;
            while(version)
            {
                VERSION* next = version->next;
                version_dispose(version);
                version = next;
            }
            free(cursor);
            cursor = next;
        }
    }

    free(table.table);

    //free versions
}

 // * When a transaction performs a GET or PUT operation for a particular key,
 // * there is an initial "garbage collection" pass that is made over the version list.
 // * In the garbage collection pass, all except the most recent committed version
 // * is removed.  In addition, if any aborted version exists, then it and all later
 // * versions in the list are removed and their creators are aborted.
 // * After garbage collection, a version list will consist of at most one committed
 // * version, followed by some number of pending versions.  Note that if there was
 // * at least one committed version before garbage collection, there will be exactly
 // * one committed version afterwards.  Also, if there were only aborted versions
 // * before garbage collection, then the version list will be empty afterwards.
void garbage_collection(MAP_ENTRY*map,VERSION* found)
{
    TRANS_STATUS status= found->creator->status;
    if(status==TRANS_COMMITTED)
    {
        debug("Removing old committed version (creator=%d)",found->creator->id);
        VERSION * current = map->versions;
        VERSION* next;
        while(current!=found)
        {
            next = current->next;
            version_dispose(current);
            current = next;
        }
        //leave the most committed version

    }
    else if(status==TRANS_ABORTED)
    {
        debug("Aborted version encountered (creator=%d), aborting subsequent versions",found->creator->id);

        VERSION * current = found;
        VERSION * next;
        while(current!=NULL )
        {
            next = current->next;

            if(trans_get_status(current->creator)!=TRANS_ABORTED)
            {
                char* why = "reference to creator for aborting";
                trans_ref(current->creator,why);
                trans_abort(current->creator);
            }
            version_dispose(current);

            current = next;
        }

    }
}
TRANS_STATUS new_bucket_entry(MAP_ENTRY* map,TRANSACTION* tp, BLOB*value)
{
    VERSION* version_to_add = version_create(tp,value);
    map->versions = version_to_add;

    debug("No previous version");
    pthread_mutex_unlock(&table.mutex);
    return tp->status;
}

TRANS_STATUS add_version(MAP_ENTRY* map,TRANSACTION* tp,KEY*key ,BLOB**value,int get)
{
    pthread_mutex_lock(&table.mutex);
    debug("Trying to put version in map entry for key %p [%s]",key,key->blob->prefix);



    if(map->versions==NULL)//a newEntry in the map bucket
    {
        debug("Add new version for key %p [%s]",key,key->blob->prefix);
        return new_bucket_entry(map,tp,*value);
    }
    else//already has versions there
    {
        VERSION* current = map->versions;
        VERSION* prev = current;
        while(current)
        {
            debug("Examine version %p for key %p [%s]",map->versions,key,key->blob->prefix);
            if(trans_get_status(current->creator)== TRANS_COMMITTED && current!=map->versions)
            {
                garbage_collection(map,current);
                map->versions = current;
                //everything before is removed

            }
            if(trans_get_status(current->creator)== TRANS_ABORTED)
            {
                garbage_collection(map,current);
                if(current==prev)
                {
                    map->versions = NULL;
                    debug("Add new version for key %p [%s]",key,key->blob->prefix);
                    return new_bucket_entry(map,tp,*value);
                }
                else
                    current = prev;

                //current is now null
            }

            //if after garbage collection it aborts still create new one



            if(current->creator->id > tp->id)
            {
                debug("Current transaction ID (%d) is less than version creator (%d) -- aborting",tp->id,current->creator->id);
                pthread_mutex_unlock(&table.mutex);
                return trans_abort(tp);
            }
            else if(current->creator->id < tp->id)
            {
                //make tp dependent on current version's transaction
                trans_add_dependency(tp,current->creator);
            }

            if(current->next==NULL)//going back to head
            {
                //last transaction in the list
                if(current->creator->id == tp->id)
                {
                    if(get)
                    {
                        *value = current->blob;
                        if(*value)
                            blob_ref(*value,"creator of version");
                    }
                    VERSION* version_to_add = version_create(tp,*value);

                    debug("Replace existing version for key %p [%s]",key,key->blob->prefix);
                    debug("tp=%p(%d), creator=%p(%d)",tp,tp->id,current->creator,current->creator->id);
                    if(current == prev)    //if only one version available
                    {
                        debug("Add new version for key %p [%s]",key,key->blob->prefix);
                        map->versions = version_to_add;

                    }
                    else
                    {
                        if(!get)
                            debug("Previous version is %p",map->versions->prev);
                        prev->next = version_to_add;
                    }

                    version_dispose(current);

                    pthread_mutex_unlock(&table.mutex);
                    return tp->status;
                }
                else
                {

                    if(get)
                    {
                        *value = current->blob;
                        if(*value)
                            blob_ref(*value,"creator of version");
                    }
                    VERSION* version_to_add = version_create(tp,*value);
                    current->next = version_to_add;


                    pthread_mutex_unlock(&table.mutex);
                    return tp->status;
                }


            }
            prev = current;
            current = current->next;
        }


        //prev->next = version_to_add;


        //map->versions->prev->next = version_to_add;//add to end



    }
    pthread_mutex_unlock(&table.mutex);
    return tp->status;

}
MAP_ENTRY* find_map_entry(KEY*key)//0 if found, -1 else
{
    int index = key->hash%NUM_BUCKETS;
    //go to that table and compare key
    pthread_mutex_lock(&table.mutex);
    MAP_ENTRY * bucketValue = table.table[index];
    int match = 0;
    MAP_ENTRY * last = NULL;
    while(bucketValue)
    {
        //compare each key and see if round match
        if(!key_compare(key,bucketValue->key))
        {
            match = 1;
            last = bucketValue;
            break;
        }
        if(bucketValue->next==NULL)
            last = bucketValue;
        bucketValue = bucketValue->next;


    }
    if(match)
    {
        debug("Matching entry exists, disposing of redundant key %p [%s]",key,key->blob->prefix);
        key_dispose(key);
    }
    else
    {
        debug("Create new map entry for key %p [%s] at table index %d",key,key->blob->prefix,index);
        MAP_ENTRY* newEntry = calloc(1,sizeof(MAP_ENTRY));
        if(last)
            last->next = newEntry;
        else
        {
                table.table[index] = newEntry;
                last = newEntry;
        }

        newEntry->key = key;
        //versions and next pointer is init to zero's

    }
    pthread_mutex_unlock(&table.mutex);
    return last;
}

/*
 * Put a key/value mapping in the store.  The key must not be NULL.
 * The value may be NULL, in which case this operation amounts to
 * deleting any existing mapping for the given key.
 *
 * This operation inherits the key and consumes one reference on
 * the value.
 *
 * @param tp  The transaction in which the operation is being performed.
 * @param key  The key.
 * @param value  The value.
 * @return  Updated status of the transation, either TRANS_PENDING,
 *   or TRANS_ABORTED.  The purpose is to be able to avoid doing further
 *   operations in an already aborted transaction.
 */
TRANS_STATUS store_put(TRANSACTION *tp, KEY *key, BLOB *value)
{
    if(key==NULL)
    {
        debug("key is null");
        exit(EXIT_FAILURE);
    }
    debug("Put mapping (key=%p [%s] -> value=%p [%s]) in store for transaction 0",key,key->blob->prefix,value,value->prefix);
    MAP_ENTRY* entry = find_map_entry(key);
    TRANS_STATUS status = add_version(entry,tp,entry->key,&value,0);
    //blob_unref(value,"");
    return status;

}


 // * Get the value associated with a specified key.  A pointer to the
 // * associated value is stored in the specified variable.
 // *
 // * This operation inherits the key.  The caller is responsible for
 // * one reference on any returned value.
 // *
 // * @param tp  The transaction in which the operation is being performed.
 // * @param key  The key.
 // * @param valuep  A variable into which a returned value pointer may be
 // *   stored.  The value pointer store may be NULL, indicating that there
 // *   is no value currently associated in the store with the specified key.
 // * @return  Updated status of the transation, either TRANS_PENDING,
 // *   or TRANS_ABORTED.  The purpose is to be able to avoid doing further
 // *   operations in an already aborted transaction.

TRANS_STATUS store_get(TRANSACTION *tp, KEY *key, BLOB **valuep)
{
    debug("Get mapping of key=%p [%s] in store for transaction %d",key,key->blob->prefix,tp->id);
    MAP_ENTRY* entry = find_map_entry(key);
    TRANS_STATUS status;
    if(entry->versions==NULL)
         status = add_version(entry,tp,key ,valuep,1); //add a null blob
    else
        status = add_version(entry,tp,key,valuep,1);

    if(*valuep)
        blob_ref(*valuep,"returning from store_get");
    return status;
}

/*
 * Print the contents of the store to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 */
void store_show(void)
{
    for(int i = 0;i<table.num_buckets;i++)
    {

        MAP_ENTRY * cursor = table.table[i];//first map entry
        fprintf(stderr,"%d:\t",i);
        while(cursor)
        {
            fprintf(stderr,"{key:%p [%s],",cursor->key,cursor->key->blob->prefix);
            VERSION* version = cursor->versions;
            while(version)
            {
                char*status_string;
                TRANS_STATUS status = version->creator->status;
                if(status==TRANS_PENDING)
                    status_string = "pending";
                else if(status==TRANS_COMMITTED)
                    status_string = "committed";
                else
                    status_string="aborted";


                if(version->blob==NULL)
                    fprintf(stderr," versions: {creator=%d (%s), (NULL blob)}}",version->creator->id,status_string);

                else
                {
                    char* content = version->blob->prefix;
                    fprintf(stderr," versions: {creator=%d (%s), blob=%p [%s]}}",version->creator->id,status_string,version->blob,content);

                }

                version = version->next;
            }

            cursor = cursor->next;
        }
        fprintf(stderr,"\n");//new line for every new bucket
    }


}