/*
 * Copyright (c) 2001-2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
 
#include "IOPMWorkArbiter.h"
#include "IOKit/IOLocks.h"
#undef super
#define super IOEventSource
OSDefineMetaClassAndStructors(IOPMWorkArbiter, IOEventSource);

/*************************************************************************/
static
void *    _dequeue(void ** inList, SInt32 inOffset)
{
    void *    oldListHead;
    void *    newListHead;

    do {
        oldListHead = *inList;
        if (oldListHead == NULL) {
            break;
        }
        
        newListHead = *(void **) (((char *) oldListHead) + inOffset);
    } while (! OSCompareAndSwap((UInt32)oldListHead,
                    (UInt32)newListHead, (UInt32 *)inList));
    return oldListHead;
}

/*************************************************************************/
static
void    _enqueue(void ** inList, void * inNewLink, SInt32 inOffset)
{
    void *    oldListHead;
    void *    newListHead = inNewLink;
    void **    newLinkNextPtr = (void **) (((char *) inNewLink) + inOffset);

    do {
        oldListHead = *inList;
        *newLinkNextPtr = oldListHead;
    } while (! OSCompareAndSwap((UInt32)oldListHead, (UInt32)newListHead,
                    (UInt32 *)inList));
}

/*************************************************************************/
IOPMWorkArbiter *IOPMWorkArbiter::pmWorkArbiter(
    IOPMrootDomain *inOwner)
{
    IOPMWorkArbiter     *me = new IOPMWorkArbiter;

    if(me && !me->init((OSObject *)inOwner, 0) )
    {
        me->release();
        return NULL;
    }

    return me;
}

/*************************************************************************/
bool IOPMWorkArbiter::init(OSObject *owner, Action action)
{
    if(!(super::init(owner, (IOEventSource::Action) action))) 
        return false;
        
    events = NULL;
    fRootDomain = (IOPMrootDomain *)owner;
    
    if (!(tmpLock = IOLockAlloc())) {
        panic("IOPMWorkArbiter::init can't alloc lock");
    }
    return true;
}


/*************************************************************************/
bool IOPMWorkArbiter::clamshellStateChangeOccurred(uint32_t messageValue)
{
    PMEventEntry                 *new_one = NULL;

    new_one = (PMEventEntry *)IOMalloc(sizeof(PMEventEntry));
    if(!new_one) return false;
    
    new_one->actionType = IOPMWorkArbiter::kRootDomainClamshellChanged;
    new_one->target = (IOService *)fRootDomain;
    new_one->intArgument = messageValue;
       new_one->target->retain();
    
    IOLockLock(tmpLock);
    _enqueue((void **)&events, (void *)new_one, 0);
    IOLockUnlock(tmpLock);
    signalWorkAvailable();

    return true;
}


/*************************************************************************/
// checkForWork() is called in a gated context
bool IOPMWorkArbiter::checkForWork()
{
    PMEventEntry                *theNode;
    IOService                   *theTarget;
    UInt16                      theAction;
    
    // Dequeue and process the state change request
    IOLockLock(tmpLock);
    if((theNode = (PMEventEntry *)_dequeue((void **)&events, 0)))
    {
      IOLockUnlock(tmpLock);
        theTarget = theNode->target;
        theAction = theNode->actionType;

        switch (theAction)
        {
            case kRootDomainClamshellChanged:
                theTarget->messageClients(
                                kIOPMMessageClamshellStateChange, 
                                (void *)theNode->intArgument);
                break;
        }

               if (theTarget)
                       theTarget->release();
        IOFree((void *)theNode, sizeof(PMEventEntry));
    }
    else {
      IOLockUnlock(tmpLock);
    }
    // Return true if there's more work to be done
    if(events) return true;
    else return false;
}


