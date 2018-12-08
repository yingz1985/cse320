#include "transaction.h"
#include "debug.h"
#include <stdlib.h>
int transactionId;

void trans_init(void)
{

    debug("Initialize transaction manager");
    trans_list.id = 0;
    pthread_mutex_init(&trans_list.mutex,NULL);
    trans_list.next = &trans_list;
    trans_list.prev = &trans_list;

}

/*
 * Finalize the transaction manager.
 */
void trans_fini(void)
{
    debug("Finalize transaction manager");

}

/*
 * Create a new transaction.
 *
 * @return  A pointer to the new transaction (with reference count 1)
 * is returned if creation is successful, otherwise NULL is returned.
 */
TRANSACTION *trans_create(void)
{
    pthread_mutex_lock(&trans_list.mutex);
    TRANSACTION* transaction = calloc(1,sizeof(TRANSACTION));
    transaction->id = trans_list.id++;
    debug("Create new transaction %d",transaction->id);
    char*why = "newly created transaction";

    transaction->depends =NULL;
    transaction->waitcnt = 0;
    if ( sem_init(&transaction->sem, 0, 0) == -1 ) {
        // error
        return NULL;
    }
    if ( pthread_mutex_init(&transaction->mutex, NULL ) == -1 ) {
        return NULL;
    }
    trans_ref(transaction,why); //initialize lock before use
    transaction->next = &trans_list;//points head
    TRANSACTION * cursor = &trans_list;
    while(cursor->next!=&trans_list)
    {
        cursor = cursor->next;  //advance cursor
    }
    debug("add transaction after %d",cursor->id);
    cursor->next = transaction;//cursor points to new transaction
    transaction->prev = cursor;
    //DEPENDENCY * depend = calloc(1,sizeof(DEPENDENCY));   //try null pointer
    pthread_mutex_unlock(&trans_list.mutex);
    return transaction;
}




/*
 * Increase the reference count on a transaction.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the increase.
 * @return  The transaction pointer passed as the argument.
 */
TRANSACTION *trans_ref(TRANSACTION *tp, char *why)
{
    pthread_mutex_lock(&tp->mutex);

    tp->refcnt++;
    debug("Increase ref count on transaction %d (%d -> %d) as %s",tp->id,tp->refcnt-1,tp->refcnt,why);

    pthread_mutex_unlock(&tp->mutex);
    return tp;
}
void release_dependents(TRANSACTION* tp);
/*
 * Decrease the reference count on a transaction.
 * If the reference count reaches zero, the transaction is freed.
 *
 * @param tp  The transaction.
 * @param why  Short phrase explaining the purpose of the decrease.
 */
void trans_unref(TRANSACTION *tp, char *why)
{
    if(tp==NULL) return;
    pthread_mutex_lock(&tp->mutex);

    tp->refcnt--;
    debug("Decrease reference count on transaction %d (%d -> %d) as %s",tp->id,tp->refcnt+1,tp->refcnt,why);

    pthread_mutex_unlock(&tp->mutex);

    pthread_mutex_lock(&tp->mutex);
    if(!tp->refcnt)
    {
        //if refcnt reached zero
        //has to be removed from transaction list

        tp->prev->next = tp->next;
        tp->next->prev = tp->prev;

        pthread_mutex_unlock(&tp->mutex);
        release_dependents(tp);

        debug("Free transaction %d",tp->id);
        pthread_mutex_destroy(&tp->mutex);
        //free(bp->prefix);//malloc'd



        free(tp);
        tp = NULL;
    }
    else
        pthread_mutex_unlock(&tp->mutex);
}

/*
 * Add a transaction to the dependency set for this transaction.
 *
 * @param tp  The transaction to which the dependency is being added.
 * @param dtp  The transaction that is being added to the dependency set.
 */
void trans_add_dependency(TRANSACTION *tp, TRANSACTION *dtp)
{
    pthread_mutex_lock(&tp->mutex);
    DEPENDENCY * depend = malloc(sizeof(DEPENDENCY));
    int dependent = 0;
    DEPENDENCY* cursor = tp->depends;
    DEPENDENCY* prev = cursor;
    while(cursor!=NULL)
    {
        TRANSACTION*transaction = cursor->trans;
        if(transaction->id == dtp->id)// if dtp already depends on tp
        {
            dependent = 1;
            break;
        }
        prev = cursor;
        cursor = cursor->next;

    }
    pthread_mutex_unlock(&tp->mutex);
    if(!dependent)
    {
        pthread_mutex_lock(&tp->mutex);
        depend->trans = dtp;
        depend->next = NULL;//add to end


        //
        if(prev)
            prev->next = depend;
        else
            tp->depends = depend;
        pthread_mutex_unlock(&tp->mutex);
        debug("Make transaction %d dependent on transaction %d",tp->id,dtp->id);
        char* why = "transaction in dependency";
        trans_ref(dtp,why);
    }

    //pthread_mutex_unlock(&tp->mutex);

    //tp->dependency
}

void release_dependents(TRANSACTION* tp)
{

    pthread_mutex_lock(&tp->mutex);
    DEPENDENCY *depends = tp->depends;

    while(depends!=NULL)
    {

        DEPENDENCY * temp = depends;

        depends = depends->next;
        pthread_mutex_unlock(&tp->mutex);
        char*why = "transaction in dependency";
        trans_unref(temp->trans,why);   //free dependency on trans
        pthread_mutex_lock(&tp->mutex);

        free(temp);
    }
    tp->depends = NULL;
    pthread_mutex_unlock(&tp->mutex);
}

void waitOnTrans(TRANSACTION * tp)
{
    pthread_mutex_lock(&tp->mutex);
    tp->waitcnt++;
    pthread_mutex_unlock(&tp->mutex);
}


TRANS_STATUS trans_commit(TRANSACTION *tp)
{
    pthread_mutex_lock(&tp->mutex);
    if(tp->status==TRANS_ABORTED)
    {
        debug("Cannot commit an already aborted transaction");
        return -1;
    }
    debug("Transaction %d trying to commit",tp->id);
    DEPENDENCY *depends = tp->depends;  //one transaction can ever commit only once
    int counter = 0;
    debug("num_transactions waiting %d",tp->waitcnt);

    while(depends!=NULL)
    {
        debug("Transaction %d checking status of dependency %d",tp->id,counter);
        TRANSACTION*transaction = depends->trans;
        pthread_mutex_unlock(&tp->mutex);

        if(trans_get_status(transaction)==TRANS_COMMITTED)
        {
            // commited
            //tp->waitcnt--;
            pthread_mutex_lock(&tp->mutex);
            depends = depends->next;
            counter++;
            pthread_mutex_unlock(&tp->mutex);
            continue;
        }
        if(trans_get_status(transaction)==TRANS_ABORTED)
        {
            return trans_abort(tp);//already aborted
        }

        debug("Transaction %d waiting for dependency %d",tp->id,counter);
        waitOnTrans(transaction);
        sem_wait(&transaction->sem);

        debug("Transaction %d finished waiting for dependency %d",tp->id,counter);
        if(trans_get_status(transaction)==TRANS_ABORTED)
        {
           // tp->waitcnt--;
            //if any transaction in the dependency list aborts, abort current
            return trans_abort(tp);

        }
        //tp->waitcnt--;
        pthread_mutex_lock(&tp->mutex);
        depends = depends->next;
        counter++;
    }

    pthread_mutex_unlock(&tp->mutex);
    char*why = "commiting transaction";

    //release_dependents(tp);
    pthread_mutex_lock(&tp->mutex);
    tp->status =    TRANS_COMMITTED;
    while(tp->waitcnt)
    {
        sem_post(&tp->sem);
        tp->waitcnt--;
    }
    pthread_mutex_unlock(&tp->mutex);
    trans_unref(tp,why);
    return TRANS_COMMITTED;

}

/*
 * Abort a transaction.  If the transaction has already committed, it is
 * a fatal error and the program crashes.  If the transaction has already
 * aborted, no change is made to its state.  If the transaction is pending,
 * then it is set to the aborted state, and any transactions dependent on
 * this transaction must also abort.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be aborted.
 * @return  TRANS_ABORTED.
 */
TRANS_STATUS trans_abort(TRANSACTION *tp)
{
    debug("Try to abort transaction %d",tp->id);
    if(trans_get_status(tp) == TRANS_COMMITTED)
    {
        debug("FATAL: transaction %d already commited",tp->id);
        abort();
    }
    else if(trans_get_status(tp)==TRANS_ABORTED)
    {
        return TRANS_ABORTED;
    }
    else
    {
        pthread_mutex_lock(&tp->mutex);
        tp->status = TRANS_ABORTED;
        debug("transaction %d waitcnt:%d",tp->id,tp->waitcnt);
        while(tp->waitcnt)
        {
            sem_post(&tp->sem);
            tp->waitcnt--;
        }
        pthread_mutex_unlock(&tp->mutex);
    }
    debug("Transaction %d has aborted",tp->id);
    //release_dependents(tp);
    char*why = "aborting transaction";
    trans_unref(tp,why);
    return TRANS_ABORTED;
}



/*
 * Get the current status of a transaction.
 * If the value returned is TRANS_PENDING, then we learn nothing,
 * because unless we are holding the transaction mutex the transaction
 * could be aborted at any time.  However, if the value returned is
 * either TRANS_COMMITTED or TRANS_ABORTED, then that value is the
 * stable final status of the transaction.
 *
 * @param tp  The transaction.
 * @return  The status of the transaction, as it was at the time of call.
 */
TRANS_STATUS trans_get_status(TRANSACTION *tp)
{
    TRANS_STATUS status;
    pthread_mutex_lock(&tp->mutex);
    status = tp->status;
    pthread_mutex_unlock(&tp->mutex);
    return status;
}

/*
 * Print information about a transaction to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 *
 * @param tp  The transaction to be shown.
 */
void trans_show(TRANSACTION *tp)
{
    fprintf(stderr,"[id=%d,status=%d,refcnt=%d]",tp->id,trans_get_status(tp),tp->refcnt);
}

/*
 * Print information about all transactions to stderr.
 * No locking is performed, so this is not thread-safe.
 * This should only be used for debugging.
 */
void trans_show_all(void)
{
    fprintf(stderr,"TRANSACTIONS:\n");
    TRANSACTION* head = &trans_list;
    int count = 0;
    int num_transaction = head->id;
    while(count<num_transaction)
    {
        //TRANSACTION* k = head->next;
        trans_show(head->next);
        head = head->next;
        count++;
    }

}