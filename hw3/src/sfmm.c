/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"


sf_epilogue * currentEpilogue;

sf_free_list_node*  findBlock(size_t size,int findNonEmptyBlock);

void setEpilogue(sf_epilogue* e)
{
    sf_footer footer;
    sf_block_info Footerinfo = {1,1,0,0,0};
    footer.info = Footerinfo;

    sf_epilogue epilogue = {footer};

    *e = epilogue;

}
void setPrologue(sf_prologue * p)
{
    sf_header header;
    sf_block_info Headerinfo = {1,0,0,0,0};
    header.info = Headerinfo;

    sf_footer Epfooter;
    sf_block_info EPFooterinfo = {1,0,0,0,0};
    Epfooter.info = EPFooterinfo;

    sf_prologue prologue = {0,header,Epfooter};
    *p = prologue;

}

void addToList(size_t size,sf_free_list_node* node,sf_header * address)
{

    sf_header blockHead;
    sf_block_info blockHeadInfo = {0,1,0,size>>4,0};
    blockHead.info = blockHeadInfo;

    sf_footer blockFoot;
    sf_block_info blockFootInfo = {0,1,0,size>>4,0};
    blockFoot.info = blockFootInfo;

    sf_free_list_node* sentinel = sf_add_free_list(size, node);

    sf_header* freeBlock = address;

    if(sentinel)
    {
            sf_header* sentinelHeader = &sentinel->head;
            sentinelHeader->links.next = freeBlock;
            sentinelHeader->links.prev = freeBlock;

            blockHead.links.next = sentinelHeader;
            blockHead.links.prev = sentinelHeader;
            *freeBlock = blockHead;

            *(((sf_footer*)((char*)freeBlock+size-8))) = blockFoot;

    }

}

void addToHead(size_t size,sf_free_list_node* node,sf_header * address)
{
    sf_header header;
    sf_block_info Headerinfo = {0,1,0,size>>4,0};
    header.info = Headerinfo;
    sf_footer blockFoot;
    sf_block_info blockFootInfo = {0,1,0,size>>4,0};
    blockFoot.info = blockFootInfo;


    header.links.next = node->head.links.next;
    header.links.prev = &node->head;    //insert at second position right after sentinel node
    (*(sf_header*)(address))= header;

    node->head.links.next = &header;
    node->head.links.next->links.prev = &header;
    *(((sf_footer*)((char*)address+size-8))) = blockFoot;

}

void coalesce(sf_header* header)
{
    sf_footer * previousBlockFooter = ((sf_footer*)((char*)header-8));
    size_t currentBlockSize = header->info.block_size;
    if(previousBlockFooter->info.allocated == 0)
    {
        //if previous block is not allocated, coalsce
        size_t previousBlockSize = previousBlockFooter->info.block_size<<4;
        sf_header* previousHeader = ((sf_header*)((char*)header - previousBlockSize));
        previousHeader->links.prev->links.next = previousHeader->links.next;
        previousHeader->links.next->links.prev = previousHeader->links.prev;

        //need to remove previous header from free list
        //removeBlockFromFreeList(previousHeader);
        previousHeader->info.block_size = previousHeader->info.block_size+ currentBlockSize;
        //sf_footer* newFooter = (((sf_footer*)((char*)previousHeader+(previousHeader->info.block_size<<4)))-1);
        //newFooter->info = previousHeader->info;
        sf_free_list_node* node =  findBlock(previousHeader->info.block_size<<4,0);

        if(node==NULL)//couldn't find a block bigger than it, insert at last position
            addToList(previousHeader->info.block_size<<4,&sf_free_list_head,previousHeader);
        else
            if(node->size > previousHeader->info.block_size<<4)
                addToList(previousHeader->info.block_size<<4,node,previousHeader);
            else //if size is exactly the same, insert at head
                addToHead(previousHeader->info.block_size<<4,node,previousHeader);



    }


}
void initHeap(int initial)
{
    if(initial)
    {
        /*sf_header header;
        sf_block_info Headerinfo = {1,0,0,0,0};
        header.info = Headerinfo;

        sf_footer Epfooter;
        sf_block_info EPFooterinfo = {1,0,0,0,0};
        Epfooter.info = EPFooterinfo;

        sf_prologue prologue = {0,header,Epfooter};
*/
        /*sf_footer footer;
        sf_block_info Footerinfo = {1,1,0,0,0};
        footer.info = Footerinfo;

        sf_epilogue epilogue = {footer};
        */
        setEpilogue((((sf_epilogue *)sf_mem_end())-1));
        setPrologue(((sf_prologue *)sf_mem_start()));
        currentEpilogue = (((sf_epilogue *)sf_mem_end())-1);
        //*((sf_prologue *)sf_mem_start()) = prologue;

        size_t pageBlock = (PAGE_SZ - sizeof(sf_prologue)-sizeof(sf_epilogue));
        addToList(pageBlock,&sf_free_list_head,((sf_header*)(    ((sf_prologue *)sf_mem_start())   +1)));
        /*sf_header blockHead;
        sf_block_info blockHeadInfo = {0,1,0,pageBlock>>4,0};
        blockHead.info = blockHeadInfo;

        sf_footer blockFoot;
        sf_block_info blockFootInfo = {0,1,0,pageBlock>>4,0};
        blockFoot.info = blockFootInfo;

        //(*(((sf_footer *)sf_mem_end())-2)) = blockFoot;
        //.info.block_size = pageBlock>>4;

        currentEpilogue = (((sf_epilogue *)sf_mem_end())-1);
        // *currentEpilogue = epilogue;

        sf_free_list_node* sentinel = sf_add_free_list(pageBlock, sf_free_list_head.next);

        sf_header* freeBlock = ((sf_header*)(    ((sf_prologue *)sf_mem_start())   +1));
        //sf_free_list_node* sentinel = &sf_free_list_head;

        //&sentinel->next->head;

        if(sentinel)
        {
            sf_header* sentinelHeader = &sentinel->head;
            sentinelHeader->links.next = freeBlock;
            sentinelHeader->links.prev = freeBlock;

            blockHead.links.next = sentinelHeader;
            blockHead.links.prev = sentinelHeader;
            *freeBlock = blockHead;

            *(((sf_footer*)((char*)freeBlock+pageBlock-8))) = blockFoot;

        }*/
        sf_show_heap();
    }
    else
    {
        setEpilogue((((sf_epilogue *)sf_mem_end())-1)); //set new epilogue
        size_t newPageSize = PAGE_SZ;   //header - epilogue = 0

        sf_block_info oldEpiNewHeader = {0,1,0,newPageSize>>4,0};
        ((sf_header*)currentEpilogue)->info = oldEpiNewHeader;

        coalesce((sf_header*)currentEpilogue);


        currentEpilogue = (((sf_epilogue *)sf_mem_end())-1);

    }
}
size_t findBlockSize(size_t size)
{
    size_t block = size + sizeof(sf_header);
    if(block <= 32) return 32;
    else
    {
        if(block%16)
        {
            return ((block+16)/16)*16;
        }
        else
            return block;
    }

}
sf_free_list_node* findBlock(size_t requestSize,int findNonEmptyBlock)
{
    sf_free_list_node *current = sf_free_list_head.next;
    int check = 0;


    //int found = 0;
    while(current != &sf_free_list_head)
    {
        if(findNonEmptyBlock)
            check = (current->head.links.next!=&current->head);
        else check = 1;
        if(current->size >= requestSize && check)
        {
            //if a block big enough is found, return that block
            return current;
        }
        else
            current = current->next;
    }


    return NULL;    //return null if not found

}

void removeBlockFromFreeList(sf_free_list_node* list,size_t blockSize)
{
    sf_header* header = list->head.links.next;  //first actual header
    header->info.allocated = 1;
    header->info.block_size = blockSize>>4;
    if(header->links.prev == header->links.next)
    {
        //if there's only a single block of blockSize
        //remove list from seg list of lists
        //header->links.prev = &list->head;
        //header->links.next = &list->head;
        list->head.links.next = &list->head;
        list->head.links.prev = &list->head;

        /*if(sf_free_list_head.next == sf_free_list_head.prev)
        {
            //if there's only one seg list
            sf_free_list_head.next = &sf_free_list_head;
            sf_free_list_head.prev = &sf_free_list_head;
                //if there's nothing else in the free list, empty free list
        }
        else
        {
            list->next->prev = list->prev;
            list->prev->next = list->next;

        }*/
    }
    else
    {
        //list->head.links.next = list->head.links.next->links.next;
        //list->head.links.next->links.next->links.prev = &list->head;

        header->links.prev->links.next = header->links.next;
        header->links.next->links.prev = header->links.prev;
        //list->head = *list->head.links.next;
        //*header = *header->links.next;
    }

}

void splitBlock(sf_free_list_node* found,size_t block_size)
{
        sf_header* blockAddress = found->head.links.next;

        size_t difference = found->size - block_size;



        if(difference >= 32)
        {
            removeBlockFromFreeList(found,block_size);

            //if block is big enough to produce another block of >32 size,
            //allocate only necessary size

            sf_free_list_node* newFreeBlock = findBlock(difference,0);//an empty list is acceptable

            /*sf_footer blockFooter;
            sf_block_info i= {0,1,0,difference>>4,0};
            blockFooter.info = i;

            sf_header insertHead;
            sf_block_info info = {0,1,0,difference>>4,0};
            insertHead.info = info;
*/
            if(newFreeBlock==NULL || newFreeBlock->size > difference)
            {
                //sf_free_list_node *newListNode;
                if(newFreeBlock==NULL)
                    addToList(difference,&sf_free_list_head,((sf_header *)(((char*)blockAddress)+block_size)));
                    //newListNode = sf_add_free_list(difference, sf_free_list_head.next);
                else
                    addToList(difference,newFreeBlock,((sf_header *)(((char*)blockAddress)+block_size)));
                    //newListNode = sf_add_free_list(difference, newFreeBlock);

                /*sf_header* freeBlock = ((sf_header *)(((char*)blockAddress)+block_size));

                sf_header* sentinelHeader = &newListNode->head;
                sentinelHeader->links.next = freeBlock;
                sentinelHeader->links.prev = freeBlock;

                insertHead.links.next = sentinelHeader;
                insertHead.links.prev = sentinelHeader;
                *freeBlock = insertHead;


                *(sf_footer*)(((char*)blockAddress)+found->size-8) = blockFooter;

*/

            }

            else if(newFreeBlock->size == difference)
            {
                //if found a block exactly the size, insert into head
                addToHead(difference,newFreeBlock,(sf_header*)((char*)blockAddress+block_size));
                /*sf_header header;
                sf_block_info Headerinfo = {0,1,0,difference>>4,0};
                header.info = Headerinfo;
                header.links.next = &newFreeBlock->head;
                header.links.prev = newFreeBlock->head.links.prev;
                (*(sf_header*)((char*)blockAddress+found->size))= header;

                newFreeBlock->head.links.prev->links.next = &header;
                newFreeBlock->head = header;*/

            }

        }
        else
        {
            //else, use the whole block
            removeBlockFromFreeList(found,found->size);

        }


}
void *sf_malloc(size_t size) {
    if(size==0) return NULL;

    if(sf_mem_start()==sf_mem_end())    // heap is size 0
    {
        sf_mem_grow();
        initHeap(1);//initial

    }
    size_t block_size = findBlockSize(size);
    sf_free_list_node* found = findBlock(block_size,1);//0 = find next non-empty block


    if(found)//if node is not null, meaning it found a block to use
    {
        sf_header* blockAddress = found->head.links.next;

        //size_t difference = found->size - block_size;
        found->head.links.next->info.requested_size = size;
        splitBlock(found,block_size);
        return blockAddress+1;

       /* if(difference >= 32)
        {
            removeBlockFromFreeList(found,block_size);

            //if block is big enough to produce another block of >32 size,
            //allocate only necessary size

            sf_free_list_node* newFreeBlock = findBlock(difference,0);//an empty list is acceptable

            sf_footer blockFooter;
            sf_block_info i= {0,1,0,difference>>4,0};
            blockFooter.info = i;

            sf_header insertHead;
            sf_block_info info = {0,1,0,difference>>4,0};
            insertHead.info = info;

            if(newFreeBlock==NULL || newFreeBlock->size > difference)
            {

                sf_free_list_node * newListNode = sf_add_free_list(difference, sf_free_list_head.next);

                sf_header* freeBlock = ((sf_header *)(((char*)blockAddress)+block_size));

                sf_header* sentinelHeader = &newListNode->head;
                sentinelHeader->links.next = freeBlock;
                sentinelHeader->links.prev = freeBlock;

                insertHead.links.next = sentinelHeader;
                insertHead.links.prev = sentinelHeader;
                *freeBlock = insertHead;


                *(sf_footer*)(((char*)blockAddress)+found->size-8) = blockFooter;




            }
            //else if(newFreeBlock->size > difference)
            {
                //if didnt find a block of right size to insert into
                sf_free_list_node * newListNode = sf_add_free_list(difference, newFreeBlock);





                *((sf_header*)((char*)blockAddress+block_size)) = insertHead;


                (*(sf_header*)(blockAddress+block_size)).info.prev_allocated = 1;

                *(sf_footer*)(((char*)blockAddress)+found->size-8) = blockFooter;



            }
            else if(newFreeBlock->size == difference)
            {
                //if found a block exactly the size, insert into head
                sf_header header;
                sf_block_info Headerinfo = {1,0,0,0,0};
                header.info = Headerinfo;
                header.links.next = &newFreeBlock->head;
                header.links.prev = newFreeBlock->head.links.prev;
                (*(sf_header*)((char*)blockAddress+found->size))= header;

                newFreeBlock->head.links.prev->links.next =&header;
                newFreeBlock->head = header;

            }

            return blockAddress+1;


        }
        else
        {
            //else, use the whole block
            removeBlockFromFreeList(found,found->size);
            return blockAddress+1;

        }
*/
    }
    else
    {
        //did not find a block size big enough, need to allocate a new page
        while(!found)
        {
            void * newPage = sf_mem_grow();
            if(newPage)
            {
                initHeap(0);
                found = findBlock(block_size,1);


            }
            else
            {
                sf_errno = ENOMEM;
                return NULL;
            }


        }

        sf_header* blockAddress = found->head.links.next;
        found->head.links.next->info.requested_size = size;
        splitBlock(found,block_size);
        return blockAddress+1;


    }



    return NULL;
}

void sf_free(void *pp) {
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    return NULL;
}
