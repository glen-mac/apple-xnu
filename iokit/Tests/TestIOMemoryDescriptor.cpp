/*
 * Copyright (c) 2014 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <sys/cdefs.h>

#include <IOKit/assert.h>
#include <IOKit/system.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOMapper.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOKitKeysPrivate.h>

#ifndef __LP64__
#include <IOKit/IOSubMemoryDescriptor.h>
#endif /* !__LP64__ */
#include <IOKit/IOSubMemoryDescriptor.h>
#include <IOKit/IOMultiMemoryDescriptor.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include <IOKit/IOKitDebug.h>
#include <libkern/OSDebug.h>
#include <sys/uio.h>

__BEGIN_DECLS
#include <vm/pmap.h>
#include <vm/vm_pageout.h>
#include <mach/memory_object_types.h>
#include <device/device_port.h>

#include <mach/vm_prot.h>
#include <mach/mach_vm.h>
#include <vm/vm_fault.h>
#include <vm/vm_protos.h>
__END_DECLS


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#if DEVELOPMENT || DEBUG

static int IOMultMemoryDescriptorTest(int newValue)
{
    IOMemoryDescriptor * mds[3];
    IOMultiMemoryDescriptor * mmd;
    IOMemoryMap * map;
    void * addr;
    uint8_t * data;
    uint32_t i;
    IOAddressRange ranges[2];

    data = (typeof(data)) IOMallocAligned(ptoa(8), page_size);
    for (i = 0; i < ptoa(8); i++) data[i] = atop(i) | 0xD0;
   
    ranges[0].address = (IOVirtualAddress)(data + ptoa(4));
    ranges[0].length  = ptoa(4);
    ranges[1].address = (IOVirtualAddress)(data + ptoa(0));
    ranges[1].length  = ptoa(4);

    mds[0] = IOMemoryDescriptor::withAddressRanges(&ranges[0], 2, kIODirectionOutIn, kernel_task);

    mds[1] = IOSubMemoryDescriptor::withSubRange(mds[0], ptoa(3), ptoa(2), kIODirectionOutIn);
    mds[2] = IOSubMemoryDescriptor::withSubRange(mds[0], ptoa(7), ptoa(1), kIODirectionOutIn);

    mmd = IOMultiMemoryDescriptor::withDescriptors(&mds[0], sizeof(mds)/sizeof(mds[0]), kIODirectionOutIn, false);
    mds[2]->release();
    mds[1]->release();
    mds[0]->release();
    map = mmd->createMappingInTask(kernel_task, 0, kIOMapAnywhere, ptoa(7), mmd->getLength() - ptoa(7));
    mmd->release();
    assert(map);

    addr = (void *) map->getVirtualAddress();
    assert(ptoa(4) == map->getLength());
    assert(0xd3d3d3d3 == ((uint32_t *)addr)[ptoa(0) / sizeof(uint32_t)]);
    assert(0xd7d7d7d7 == ((uint32_t *)addr)[ptoa(1) / sizeof(uint32_t)]);
    assert(0xd0d0d0d0 == ((uint32_t *)addr)[ptoa(2) / sizeof(uint32_t)]);
    assert(0xd3d3d3d3 == ((uint32_t *)addr)[ptoa(3) / sizeof(uint32_t)]);
    map->release();
    IOFreeAligned(data, ptoa(8));

    return (0);
}


int IOMemoryDescriptorTest(int newValue)
{
    int result;

#if 0
    if (5 == newValue)
    {
	IOReturn             ret;
	IOMemoryDescriptor * md;
	IODMACommand       * dma;
	IODMACommand::SegmentOptions segOptions =
	{
	    .fStructSize      = sizeof(segOptions),
	    .fNumAddressBits  = 64,
	    .fMaxSegmentSize  = 4096,
	    .fMaxTransferSize = 128*1024,
	    .fAlignment       = 4,
	    .fAlignmentLength = 4,
	    .fAlignmentInternalSegments = 0x1000
	};

	IOAddressRange ranges[3][2] =
	{
	    {
		{ (uintptr_t) &IOMemoryDescriptorTest, 0x2ffc },
		{ 0, 0 },
	    },
	    {
		{ ranges[0][0].address, 0x10 },
		{ 0x3000 + ranges[0][0].address, 0xff0 },
	    },
	    {
		{ ranges[0][0].address, 0x2ffc },
		{ trunc_page(ranges[0][0].address), 0x800 },
	    },
	};
	static const uint32_t rangesCount[3] = { 1, 2, 2 };
	uint32_t test;

	for (test = 0; test < 3; test++)
	{
	    kprintf("---[%d] address 0x%qx-0x%qx, 0x%qx-0x%qx\n", test, 
	    			ranges[test][0].address, ranges[test][0].length,
	    			ranges[test][1].address, ranges[test][1].length);

	    md = IOMemoryDescriptor::withAddressRanges((IOAddressRange*)&ranges[test][0], rangesCount[test], kIODirectionOut, kernel_task);
	    assert(md);
	    ret = md->prepare();
	    assert(kIOReturnSuccess == ret);
	    dma = IODMACommand::withSpecification(kIODMACommandOutputHost64, &segOptions,
						  IODMACommand::kMapped, NULL, NULL);
	    assert(dma);
	    ret = dma->setMemoryDescriptor(md, true);
	    if (kIOReturnSuccess == ret)
	    {
		IODMACommand::Segment64 segments[1];
		UInt32                  numSegments;
		UInt64                  offset;

		offset = 0;
		do
		{
		    numSegments = 1;
		    ret = dma->gen64IOVMSegments(&offset, &segments[0], &numSegments);
		    assert(kIOReturnSuccess == ret);
		    assert(1 == numSegments);
		    kprintf("seg 0x%qx, 0x%qx\n", segments[0].fIOVMAddr, segments[0].fLength);
		}
		while (offset < md->getLength());

		ret = dma->clearMemoryDescriptor(true);
		assert(kIOReturnSuccess == ret);
		dma->release();
	    }
	    md->release();
        }

	return (kIOReturnSuccess);
    }
    else if (4 == newValue)
    {
	IOService * isp;
	IOMapper *  mapper;
	IOBufferMemoryDescriptor * md1;
	IODMACommand * dma;
	IOReturn       ret;
	size_t         bufSize = 8192 * 8192 * sizeof(uint32_t);
	uint64_t start, time, nano;

	isp = IOService::copyMatchingService(IOService::nameMatching("isp"));
	assert(isp);
        mapper = IOMapper::copyMapperForDeviceWithIndex(isp, 0);
	assert(mapper);

	md1 = IOBufferMemoryDescriptor::inTaskWithOptions(TASK_NULL, 
		kIODirectionOutIn | kIOMemoryPersistent | kIOMemoryPageable,
		bufSize, page_size);

	ret = md1->prepare();
	assert(kIOReturnSuccess == ret);

	IODMAMapSpecification mapSpec;
	bzero(&mapSpec, sizeof(mapSpec));
	uint64_t mapped;
	uint64_t mappedLength;

	start = mach_absolute_time();

	ret =  md1->dmaMap(mapper, NULL, &mapSpec, 0, bufSize, &mapped, &mappedLength);
	assert(kIOReturnSuccess == ret);

	time = mach_absolute_time() - start;

	absolutetime_to_nanoseconds(time, &nano);
	kprintf("time %lld us\n", nano / 1000ULL);
	kprintf("seg0 0x%qx, 0x%qx\n", mapped, mappedLength);

	assert(md1);

	dma = IODMACommand::withSpecification(kIODMACommandOutputHost32, 
				32, 0, IODMACommand::kMapped, 0, 1, mapper, NULL);

	assert(dma);

	start = mach_absolute_time();
	ret = dma->setMemoryDescriptor(md1, true);
	assert(kIOReturnSuccess == ret);
	time = mach_absolute_time() - start;

	absolutetime_to_nanoseconds(time, &nano);
	kprintf("time %lld us\n", nano / 1000ULL);

	
	IODMACommand::Segment32 segments[1];
	UInt32                  numSegments = 1;
	UInt64                  offset;

	offset = 0;
	ret = dma->gen32IOVMSegments(&offset, &segments[0], &numSegments);
	assert(kIOReturnSuccess == ret);
	assert(1 == numSegments);
	kprintf("seg0 0x%x, 0x%x\n", (int)segments[0].fIOVMAddr, (int)segments[0].fLength);

	ret = dma->clearMemoryDescriptor(true);
	assert(kIOReturnSuccess == ret);

	md1->release();

	return (kIOReturnSuccess);
    }

    if (3 == newValue)
    {
	IOBufferMemoryDescriptor * md1;
	IOBufferMemoryDescriptor * md2;
	IOMemoryMap * map1;
	IOMemoryMap * map2;
	uint32_t * buf1;
	uint32_t * buf2;
	IOReturn err;

	md1 = IOBufferMemoryDescriptor::inTaskWithOptions(TASK_NULL, 
		kIODirectionOutIn | kIOMemoryPersistent | kIOMemoryPageable,
		64*1024, page_size);
	assert(md1);
	map1 = md1->createMappingInTask(kernel_task, 0, kIOMapAnywhere | kIOMapUnique);
	assert(map1);
	buf1 = (uint32_t *) map1->getVirtualAddress();

	md2 = IOBufferMemoryDescriptor::inTaskWithOptions(TASK_NULL, 
		kIODirectionOutIn | kIOMemoryPersistent | kIOMemoryPageable,
		64*1024, page_size);
	assert(md2);
	map2 = md2->createMappingInTask(kernel_task, 0, kIOMapAnywhere | kIOMapUnique);
	assert(map2);
	buf2 = (uint32_t *) map2->getVirtualAddress();

	memset(buf1, 0x11, 64*1024L);
	memset(buf2, 0x22, 64*1024L);

	kprintf("md1 %p, map1 %p, buf2 %p; md2 %p, map2 %p, buf2 %p\n", md1, map1, buf1, md2, map2, buf2);

	kprintf("no redir 0x%08x, 0x%08x\n", buf1[0], buf2[0]);
	assert(0x11111111 == buf1[0]);
	assert(0x22222222 == buf2[0]);
	err = map1->redirect(md2, 0, 0ULL);
	kprintf("redir md2(0x%x) 0x%08x, 0x%08x\n", err, buf1[0], buf2[0]);
	assert(0x11111111 == buf2[0]);
	assert(0x22222222 == buf1[0]);
	err = map1->redirect(md1, 0, 0ULL);
	kprintf("redir md1(0x%x) 0x%08x, 0x%08x\n", err, buf1[0], buf2[0]);
	assert(0x11111111 == buf1[0]);
	assert(0x22222222 == buf2[0]);
	map1->release();
	map2->release();
	md1->release();
	md2->release();
    }
#endif

    result = IOMultMemoryDescriptorTest(newValue);
    if (result) return (result);

    IOGeneralMemoryDescriptor * md;
    vm_offset_t data[2];
    vm_size_t  bsize = 16*1024*1024;
    vm_size_t  srcsize, srcoffset, mapoffset, size;
    kern_return_t kr;

    kr = vm_allocate(kernel_map, &data[0], bsize, VM_FLAGS_ANYWHERE);
    vm_inherit(kernel_map, data[0] + ptoa(1), ptoa(1), VM_INHERIT_NONE);
    vm_inherit(kernel_map, data[0] + ptoa(16), ptoa(4), VM_INHERIT_NONE);

    IOLog("data 0x%lx, 0x%lx\n", (long)data[0], (long)data[1]);

    uint32_t idx, offidx;
    for (idx = 0; idx < (bsize / sizeof(uint32_t)); idx++)
    {
	((uint32_t*)data[0])[idx] = idx;    
    }

    for (srcoffset = 0; srcoffset < bsize; srcoffset = ((srcoffset << 2) + 0x40c))
    {
	for (srcsize = 4; srcsize < (bsize - srcoffset - 1); srcsize = ((srcsize << 2) + 0x3fc))
	{
	    IOAddressRange ranges[3];
	    uint32_t rangeCount = 1;

	    bzero(&ranges[0], sizeof(ranges));
	    ranges[0].address = data[0] + srcoffset;
	    ranges[0].length  = srcsize;

	    if (srcsize > ptoa(5))
	    {
		ranges[0].length  = 7634;
		ranges[1].length  = 9870;
		ranges[2].length  = srcsize - ranges[0].length - ranges[1].length;
		ranges[1].address = ranges[0].address + ranges[0].length;
		ranges[2].address = ranges[1].address + ranges[1].length;
		rangeCount = 3;	    
	    }
	    else if ((srcsize > ptoa(2)) && !(page_mask & srcoffset))
	    {
		ranges[0].length  = ptoa(1);
		ranges[1].length  = ptoa(1);
		ranges[2].length  = srcsize - ranges[0].length - ranges[1].length;
		ranges[0].address = data[0] + srcoffset + ptoa(1);
		ranges[1].address = data[0] + srcoffset;
		ranges[2].address = ranges[0].address + ranges[0].length;
		rangeCount = 3;	    
	    }

	    md = OSDynamicCast(IOGeneralMemoryDescriptor, 
	    	IOMemoryDescriptor::withAddressRanges(&ranges[0], rangeCount, kIODirectionInOut, kernel_task));
	    assert(md);

	    IOLog("IOMemoryDescriptor::withAddressRanges [0x%lx @ 0x%lx]\n[0x%llx, 0x%llx],\n[0x%llx, 0x%llx],\n[0x%llx, 0x%llx]\n", 
	    	    (long) srcsize, (long) srcoffset,
		    (long long) ranges[0].address - data[0], (long long) ranges[0].length,
		    (long long) ranges[1].address - data[0], (long long) ranges[1].length,
		    (long long) ranges[2].address - data[0], (long long) ranges[2].length);

	    if (kIOReturnSuccess == kr)
	    {
		for (mapoffset = 0; mapoffset < srcsize; mapoffset = ((mapoffset << 1) + 0xf00))
		{
		    for (size = 4; size < (srcsize - mapoffset - 1); size = ((size << 2) + 0x200))
		    {
		    	IOMemoryMap     * map;
			mach_vm_address_t addr = 0;
			uint32_t          data;

//			IOLog("<mapRef [0x%lx @ 0x%lx]\n", (long) size, (long) mapoffset);

			map = md->createMappingInTask(kernel_task, 0, kIOMapAnywhere, mapoffset, size);
			if (map) addr = map->getAddress();
			else kr = kIOReturnError;

//			IOLog(">mapRef 0x%x %llx\n", kr, addr);

			if (kIOReturnSuccess != kr) break;
			kr = md->prepare();
			if (kIOReturnSuccess != kr)
			{
			    panic("prepare() fail 0x%x\n", kr);
			    break;
			}
			for (idx = 0; idx < size; idx += sizeof(uint32_t))
			{
			    offidx = (idx + mapoffset + srcoffset);
			    if ((srcsize <= ptoa(5)) && (srcsize > ptoa(2)) && !(page_mask & srcoffset))
			    {
			    	if (offidx < ptoa(2)) offidx ^= ptoa(1);
			    }
			    offidx /= sizeof(uint32_t);

			    if (offidx != ((uint32_t*)addr)[idx/sizeof(uint32_t)]) 
			    {
				panic("vm mismatch md %p map %p, @ 0x%x, 0x%lx, 0x%lx, \n", md, map, idx, (long) srcoffset, (long) mapoffset);
				kr = kIOReturnBadMedia;
			    }
			    else
			    {
			        if (sizeof(data) != md->readBytes(mapoffset + idx, &data, sizeof(data))) data = 0;
				if (offidx != data) 
				{
				    panic("phys mismatch md %p map %p, @ 0x%x, 0x%lx, 0x%lx, \n", md, map, idx, (long) srcoffset, (long) mapoffset);
				    kr = kIOReturnBadMedia;
				}
			    }
			}
			md->complete();
			map->release();
//			IOLog("unmapRef %llx\n", addr);
		    }
		    if (kIOReturnSuccess != kr) break;
		}
	    }
            md->release();
            if (kIOReturnSuccess != kr) break;
	}
	if (kIOReturnSuccess != kr) break;
    }

    if (kIOReturnSuccess != kr) IOLog("FAIL: src 0x%lx @ 0x%lx, map 0x%lx @ 0x%lx\n", 
    					(long) srcsize, (long) srcoffset, (long) size, (long) mapoffset);

    assert(kr == kIOReturnSuccess);

    vm_deallocate(kernel_map, data[0], bsize);
//    vm_deallocate(kernel_map, data[1], size);

    return (0);
}

#endif  /* DEVELOPMENT || DEBUG */
