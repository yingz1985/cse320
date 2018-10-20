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
sf_prologue * currentPrologueEnd;

sf_free_list_node*  findBlock(size_t size,int findNonEmptyBlock);
void removeBlockFromFreeList(sf_header* header);
void insert(size_t difference,sf_free_list_node*newFreeBlock,void * blockAddress);

void setEpilogue(sf_epilogue* e)
{
    sf_footer footer;
    sf_block_info Footerinfo = {1,0,0,0,0};
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
    //size, node to insert in front of , address of header
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
            sf_footer * foot = (((sf_footer*)(((char*)freeBlock)+size-8)));
            *foot = blockFoot;


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


    node->head.links.next->links.prev = address;
    node->head.links.next = address;

    *(((sf_footer*)(((char*)address)+size-8))) = blockFoot;
    //sf_show_heap();

}

void coalesce(sf_header* header,int AddBlock)
{
    sf_footer * previousBlockFooter = ((sf_footer*)(((sf_footer*)header)-1));
    size_t currentBlockSize = header->info.block_size;
    sf_header* headerToInsert = header;
    int coalesce = 0;
    //if(previousBlockFooter->info.allocated == 0)
    if(header->info.prev_allocated==0)
    {
        coalesce = 1;

        //if previous block is not allocated, coalsce
        size_t previousBlockSize = previousBlockFooter->info.block_size<<4;
        sf_header* previousHeader = ((sf_header*)(((char*)header) - previousBlockSize));

       // previousHeader->links.prev->links.next = previousHeader->links.next;
       // previousHeader->links.next->links.prev = previousHeader->links.prev;
        removeBlockFromFreeList(previousHeader);

        //need to remove previous header from free list
        //removeBlockFromFreeList(previousHeader);
        previousHeader->info.block_size = previousHeader->info.block_size+ currentBlockSize;
        headerToInsert = previousHeader;
        //sf_footer* newFooter = (((sf_footer*)((char*)previousHeader+(previousHeader->info.block_size<<4)))-1);
        //newFooter->info = previousHeader->info;
        //printf("header%d",headerToInsert->info.block_size);
    }

    if(coalesce || AddBlock)
    {
        sf_free_list_node* node =  findBlock(headerToInsert->info.block_size<<4,0);
        insert(headerToInsert->info.block_size<<4,node,headerToInsert);
        size_t difference = headerToInsert->info.block_size<<4;

        sf_header * nextBlockHead = ((sf_header*)((char*)headerToInsert+difference));
        //if(nextBlockHead!=(sf_header*)currentEpilogue)
        nextBlockHead->info.prev_allocated = 0;

    }

    //
    //sf_show_heap();




}
void initHeap(int initial)
{
    if(initial)
    {

        setEpilogue((((sf_epilogue *)sf_mem_end())-1));
        setPrologue(((sf_prologue *)sf_mem_start()));
        currentEpilogue = (((sf_epilogue *)sf_mem_end())-1);
        currentPrologueEnd = ((sf_prologue *)sf_mem_start()+1);
        //*((sf_prologue *)sf_mem_start()) = prologue;

        size_t pageBlock = (PAGE_SZ - sizeof(sf_prologue)-sizeof(sf_epilogue));
        addToList(pageBlock,&sf_free_list_head,((sf_header*)(    ((sf_prologue *)sf_mem_start())   +1)));

        //sf_show_heap();
    }
    else
    {
        setEpilogue((((sf_epilogue *)sf_mem_end())-1)); //set new epilogue
        size_t newPageSize = PAGE_SZ;   //header - epilogue = 0

        //sf_block_info oldEpiNewHeader = {0,1,0,newPageSize>>4,0};
        ((sf_header*)currentEpilogue)->info.block_size = newPageSize>>4;
        //oldEpiNewHeader;

        coalesce((sf_header*)currentEpilogue,1);

        //sf_show_heap();
        currentEpilogue = (((sf_epilogue *)sf_mem_end())-1);

    }
}
size_t findBlockSize(size_t size)
{
    size_t block = size + sizeof(sf_block_info);
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

void removeBlockFromFreeList(sf_header* header)
{
    /*sf_header* header = list->head.links.next;  //first actual header
    header->info.allocated = 1;
    header->info.block_size = blockSize>>4;
    if(header->links.prev == header->links.next)
    {
        //if there's only a single block of blockSize
        //remove list from seg list of lists

        list->head.links.next = &list->head;
        list->head.links.prev = &list->head;
        header->links.prev = NULL;
        header->links.next = NULL;

    }
    else*/
    //if(header->links.next==NULL && header->links.prev ==NULL) return;
    {

        header->links.prev->links.next = header->links.next;
        header->links.next->links.prev = header->links.prev;
        header->links.prev = NULL;
        header->links.next = NULL;
    }

}
void insert(size_t difference,sf_free_list_node*newFreeBlock,void * blockAddress)
{
    if(newFreeBlock==NULL || newFreeBlock->size > difference)
    {
        if(newFreeBlock==NULL)
            addToList(difference,&sf_free_list_head,blockAddress);
        else
            addToList(difference,newFreeBlock,blockAddress);

    }

    else if(newFreeBlock->size == difference)
    {
        addToHead(difference,newFreeBlock,blockAddress);

    }
   /* sf_header * nextBlockHead = ((sf_header*)((char*)blockAddress+difference));
    if(nextBlockHead!=(sf_header*)currentEpilogue)
        nextBlockHead->info.prev_allocated = 0;

*/

}

void splitBlock(sf_free_list_node* found,size_t block_size)
{
        sf_header* blockAddress = found->head.links.next;

        size_t difference = found->size - block_size;



        if(difference >= 32)
        {
            sf_header* header = found->head.links.next;
            header->info.allocated = 1;
            header->info.block_size = block_size>>4;
            removeBlockFromFreeList(found->head.links.next);



            //if block is big enough to produce another block of >32 size,
            //allocate only necessary size

            sf_free_list_node* newFreeBlock = findBlock(difference,0);//an empty list is acceptable

            insert(difference,newFreeBlock,((sf_header *)(((char*)blockAddress)+block_size)));
            sf_header* nextHeader = (sf_header*)((char*)header +block_size);
            nextHeader->info.prev_allocated = 1;


        }
        else
        {
            //else, use the whole block
            sf_header* header = found->head.links.next;
            header->info.allocated = 1;
            header->info.block_size = found->size>>4;
            removeBlockFromFreeList(found->head.links.next);
            sf_header* nextHeader = (sf_header*)((char*)header +(header->info.block_size<<4));
            nextHeader->info.prev_allocated = 1;

        }


}
void *sf_malloc(size_t size) {
    if(size==0) return NULL;

/////
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
        return ((sf_block_info*)blockAddress)+1;

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
        return ((sf_block_info*)blockAddress)+1;

    }

    sf_errno = ENOMEM;//did not satisfy condition
    return NULL;
}
int pointerValidity(void*pp)
{
    if(pp==NULL)
    {
            //sf_errno = EINVAL;
            return 0;
    }
    sf_header* header = (sf_header*)((sf_block_info*)pp-1);
    size_t block_size = header->info.block_size<<4;
    if((void*)header < (void*)currentPrologueEnd) return 0;  //if header appears before the end of prologue
    if(header->info.allocated==0) return 0;
    if(block_size%16!=0  || block_size<32 ) return 0;
    if(header->info.requested_size + sizeof(sf_block_info) > block_size) return 0;

    sf_footer* previousFoot = ((sf_footer*)pp-2);
    sf_header* previousHead = (sf_header*)(((char*)pp)-(previousFoot->info.block_size<<4));
    int allocated = 1;
    if(header->info.prev_allocated==0)
    {
        if(previousHead->info.allocated == 0 && previousFoot->info.allocated == 0)
            allocated = 1;
        else allocated = 0;
    }
    if(!allocated) return 0;
    return 1;

}

void sf_free(void *pp) {
    //pp is the first byte of payload
    if(pointerValidity(pp) ==0) abort();
    sf_header* header = (sf_header*)((sf_block_info*)pp-1);

    sf_header* nextBlockHeader = ((sf_header*)(((char*)header)+(header->info.block_size<<4)));
    //(((sf_header*)(((char*)header)+size-8)))
    //sf_show_heap();
    coalesce(header,1);

    //sf_show_heap();

    if(nextBlockHeader->info.allocated==0)
    {
            removeBlockFromFreeList(nextBlockHeader);
            //header->links.prev->links.next = header->links.next;
            //header->links.next->links.prev = header->links.prev;
            coalesce(nextBlockHeader,1);
            //nextBlockHeader->info.prev_allocated = 0;

    }

   // sf_show_heap();
    //see if block after can be coalesced as well





    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    //what constitutes an invalid pointer
    if(pointerValidity(pp)==0)
    {
        sf_errno = EINVAL;
        abort();
    }
    if (rsize ==0)
    {
        sf_free(pp);
        return NULL;
    }

    sf_header* header = (sf_header*)((sf_block_info*)pp-1);
    size_t requiredBlockSize = findBlockSize(rsize);
    if(requiredBlockSize > header->info.block_size<<4)
    {
        void * block = sf_malloc(rsize);
        if(block==NULL) return NULL;
        memcpy(block,pp,header->info.requested_size);
        sf_free(pp);
        return block;

    }
    else
    {
        size_t SplitBlockSize = (header->info.block_size<<4)-requiredBlockSize;
        if(SplitBlockSize <32 || requiredBlockSize<32)
        {   // if splitting would result in a splinter, dont split
            header->info.requested_size = rsize;
            return pp;

        }
        else
        {
            //size_t requiredBlockSize = findBlockSize(rsize);
            header->info.block_size = requiredBlockSize>>4;
            header->info.requested_size = rsize;

            //void * footer = (char*)header+(header->info.block_size<<4)-8;
            //*((sf_block_info*)footer) = header->info;
            //doesn't require footer because block is already allocated
            //sf_free_list_node* splitBlock = findBlock(size,0);//an empty list is acceptable
            void * splitBlockAddress = (char*)header+(header->info.block_size<<4);
            sf_block_info info = {1,1,0,SplitBlockSize>>4,0};
            *((sf_block_info*)splitBlockAddress) = info;

            *((sf_block_info*)((char*)splitBlockAddress+SplitBlockSize-8)) =  info;
            //insert(size,splitBlock,splitBlockAddress);
            sf_free((sf_block_info*)splitBlockAddress+1);
            return pp;


        }

    }
    return NULL;
}
