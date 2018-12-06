#include "transaction.h"
#include "debug.h"
#include <stdlib.h>
int transactionId;

void trans_init(void)
{

    debug("Initialize transaction manager");
    trans_list.id = 0;
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
    TRANSACTION* transaction = calloc(1,sizeof(TRANSACTION));
    transaction->id = trans_list.id++;
    debug("Create new transaction %d",transaction->id);
    char*why = "newly created transaction";
    trans_ref(transaction,why);
    transaction->depends =NULL;
    transaction->waitcnt = 0;
    if ( sem_init(&transaction->sem, 0, 0) == -1 ) {
        // error
        return NULL;
    }
    if ( pthread_mutex_init(&transaction->mutex, NULL ) == -1 ) {
        return NULL;
    }
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
    int count = 0;
    int dependent = 0;
    DEPENDENCY* cursor = tp->depends;
    while(count<tp->waitcnt)
    {
        TRANSACTION*transaction = cursor->trans;
        if(transaction->id == dtp->id)// if dtp already depends on tp
            dependent = 1;

        cursor = cursor->next;
        count++;

    }

    if(!dependent)
    {
        depend->trans = dtp;
        depend->next = tp->depends;
        tp->depends = depend;
        tp->waitcnt++;  //one more transaction that waits on tp
        debug("Make transaction %d dependent on transaction %d",tp->id,dtp->id);
        char* why = "transaction in dependency";
        trans_ref(dtp,why);
    }

    pthread_mutex_unlock(&tp->mutex);

    //tp->dependency
}
// typedef struct transaction {
//   unsigned int id;           // Transaction ID.
//   unsigned int refcnt;       // Number of references (pointers) to transaction.
//   TRANS_STATUS status;       // Current transaction status.
//   DEPENDENCY *depends;       // Singly-linked list of dependencies.
//   int waitcnt;               // Number of transactions waiting for this one.
//   sem_t sem;                 // Semaphore to wait for transaction to commit or abort.
//   pthread_mutex_t mutex;     // Mutex to protect fields.
//   struct transaction *next;  // Next in list of all transactions
//   struct transaction *prev;  // Prev in list of all transactions.
// } TRANSACTION;

void release_dependents(TRANSACTION* tp)
{
    int counter = 0;
    DEPENDENCY *depends = tp->depends;
    while(counter< tp->waitcnt)
    {
        DEPENDENCY * temp = depends;

        counter++;
        depends = depends->next;
        char*why = "release dependencies";
        trans_unref(tp,why);

        free(temp);
    }
    debug("Release %d waiters dependent on transaction %d",tp->waitcnt,tp->id);
}
/*
 * Try to commit a transaction.  Committing a transaction requires waiting
 * for all transactions in its dependency set to either commit or abort.
 * If any transaction in the dependency set abort, then the dependent
 * transaction must also abort.  If all transactions in the dependency set
 * commit, then the dependent transaction may also commit.
 *
 * In all cases, this function consumes a single reference to the transaction
 * object.
 *
 * @param tp  The transaction to be committed.
 * @return  The final status of the transaction: either TRANS_ABORTED,
 * or TRANS_COMMITTED.
 */
TRANS_STATUS trans_commit(TRANSACTION *tp)
{
    debug("Transaction %d trying to commit",tp->id);
    int counter =  0 ;
    DEPENDENCY *depends = tp->depends;
    while(counter< tp->waitcnt)
    {
        debug("Transaction %d checking status of dependency %d",tp->id,counter);
        TRANSACTION*transaction = depends->trans;
        if(trans_get_status(tp)==TRANS_COMMITTED)
        {
            // commited
            counter++;
            continue;
        }
        debug("Transaction %d waiting for dependency %d",tp->id,counter);
        sem_wait(&transaction->sem);
        debug("Transaction %d finished waiting for dependency %d",tp->id,counter);
        if(trans_get_status(tp)==TRANS_ABORTED)
        {
            //if any transaction in the dependency list aborts, abort current
            return trans_abort(tp);

        }
        counter++;
        depends = depends->next;
    }


    sem_post(&tp->sem);
    //if(tp->status==TRANS_PENDING)
    release_dependents(tp);
    tp->status =    TRANS_COMMITTED;
    return tp->status;

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
    if(tp->status == TRANS_COMMITTED)
    {
        debug("FATAL: transaction %d already commited",tp->id);
        exit(0);
    }
    else if(trans_get_status(tp)==TRANS_ABORTED)
    {
        return tp->status;
    }
    else
    {
        tp->status = TRANS_ABORTED;
        sem_post(&tp->sem);
    }
    debug("Transaction %d has aborted",tp->id);
    release_dependents(tp);
    return trans_get_status(tp);
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
    return tp->status;
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