/*      $OpenBSD: bus_dma.c,v 1.16 2004/11/09 19:17:01 claudio Exp $        */
/*      $NetBSD: bus_dma.c,v 1.2 2001/06/10 02:31:25 briggs Exp $        */

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/extent.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>

#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <machine/intr.h>

int     _bus_dmamap_load_buffer (bus_dma_tag_t, bus_dmamap_t, void *,
            bus_size_t, struct proc *, int, paddr_t *, int *, int);

/*
 * Common function for DMA map creation.  May be called by bus-specific
 * DMA map creation functions.
 */
int
_bus_dmamap_create(t, size, nsegments, maxsegsz, boundary, flags, dmamp)
        bus_dma_tag_t t;
        bus_size_t size;
        int nsegments;
        bus_size_t maxsegsz;
        bus_size_t boundary;
        int flags;
        bus_dmamap_t *dmamp;
{
        struct powerpc_bus_dmamap *map;
        void *mapstore;
        size_t mapsize;

        /*
         * Allocate and initialize the DMA map.  The end of the map
         * is a variable-sized array of segments, so we allocate enough
         * room for them in one shot.
         *
         * Note we don't preserve the WAITOK or NOWAIT flags.  Preservation
         * of ALLOCNOW notifies others that we've reserved these resources,
         * and they are not to be freed.
         *
         * The bus_dmamap_t includes one bus_dma_segment_t, hence
         * the (nsegments - 1).
         */
        mapsize = sizeof(struct powerpc_bus_dmamap) +
            (sizeof(bus_dma_segment_t) * (nsegments - 1));
        if ((mapstore = malloc(mapsize, M_DEVBUF,
            (flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK)) == NULL)
                return (ENOMEM);

        memset(mapstore, 0, mapsize);
        map = (struct powerpc_bus_dmamap *)mapstore;
        map->_dm_size = size;
        map->_dm_segcnt = nsegments;
        map->_dm_maxsegsz = maxsegsz;
        map->_dm_boundary = boundary;
        map->_dm_bounce_thresh = t->_bounce_thresh;
        map->_dm_flags = flags & ~(BUS_DMA_WAITOK|BUS_DMA_NOWAIT);
        map->dm_mapsize = 0;                /* no valid mappings */
        map->dm_nsegs = 0;

        *dmamp = map;
        return (0);
}

/*
 * Common function for DMA map destruction.  May be called by bus-specific
 * DMA map destruction functions.
 */
void
_bus_dmamap_destroy(t, map)
        bus_dma_tag_t t;
        bus_dmamap_t map;
{

        free(map, M_DEVBUF);
}

/*
 * Utility function to load a linear buffer.  lastaddrp holds state
 * between invocations (for multiple-buffer loads).  segp contains
 * the starting segment on entrance, and the ending segment on exit.
 * first indicates if this is the first invocation of this function.
 */
int
_bus_dmamap_load_buffer(t, map, buf, buflen, p, flags, lastaddrp, segp, first)
        bus_dma_tag_t t;
        bus_dmamap_t map;
        void *buf;
        bus_size_t buflen;
        struct proc *p;
        int flags;
        paddr_t *lastaddrp;
        int *segp;
        int first;
{
        bus_size_t sgsize;
        bus_addr_t curaddr, lastaddr, baddr, bmask;
        vaddr_t vaddr = (vaddr_t)buf;
        int seg;
	pmap_t pmap;

	if (p != NULL)
		pmap = p->p_vmspace->vm_map.pmap;
	else
		pmap = pmap_kernel();

        lastaddr = *lastaddrp;
        bmask = ~(map->_dm_boundary - 1);

        for (seg = *segp; buflen > 0 ; ) {
                /*
                 * Get the physical address for this segment.
                 */
		if (pmap_extract(pmap, vaddr, (paddr_t *)&curaddr) == FALSE) {
			panic("dmamap_load_buffer pmap %p vaddr %lx "
			    "pmap_extract failed", pmap, vaddr);
		}

                /*
                 * If we're beyond the bounce threshold, notify
                 * the caller.
                 */
                if (map->_dm_bounce_thresh != 0 &&
                    curaddr >= map->_dm_bounce_thresh)
                        return (EINVAL);

                /*
                 * Compute the segment size, and adjust counts.
                 */
                sgsize = NBPG - ((u_long)vaddr & PGOFSET);
                if (buflen < sgsize)
                        sgsize = buflen;

                /*
                 * Make sure we don't cross any boundaries.
                 */
                if (map->_dm_boundary > 0) {
                        baddr = (curaddr + map->_dm_boundary) & bmask;
                        if (sgsize > (baddr - curaddr))
                                sgsize = (baddr - curaddr);
                }

                /*
                 * Insert chunk into a segment, coalescing with
                 * the previous segment if possible.
                 */
                if (first) {
                        map->dm_segs[seg].ds_addr = PHYS_TO_PCI_MEM(curaddr);
                        map->dm_segs[seg].ds_len = sgsize;
                        first = 0;
                } else {
                        if (curaddr == lastaddr &&
                            (map->dm_segs[seg].ds_len + sgsize) <=
                             map->_dm_maxsegsz &&
                            (map->_dm_boundary == 0 ||
                             (map->dm_segs[seg].ds_addr & bmask) ==
                             (PHYS_TO_PCI_MEM(curaddr) & bmask)))
                                map->dm_segs[seg].ds_len += sgsize;
                        else {
                                if (++seg >= map->_dm_segcnt)
                                        break;
                                map->dm_segs[seg].ds_addr =
                                        PHYS_TO_PCI_MEM(curaddr);
                                map->dm_segs[seg].ds_len = sgsize;
                        }
                }

                lastaddr = curaddr + sgsize;
                vaddr += sgsize;
                buflen -= sgsize;
        }

        *segp = seg;
        *lastaddrp = lastaddr;

        /*
         * Did we fit?
         */
        if (buflen != 0)
                return (EFBIG);                /* XXX better return value here? */

        return (0);
}

/*
 * Common function for loading a DMA map with a linear buffer.  May
 * be called by bus-specific DMA map load functions.
 */
int
_bus_dmamap_load(t, map, buf, buflen, p, flags)
        bus_dma_tag_t t;
        bus_dmamap_t map;
        void *buf;
        bus_size_t buflen;
        struct proc *p;
        int flags;
{
        paddr_t lastaddr;
        int seg, error;

        /*
         * Make sure that on error condition we return "no valid mappings".
         */
        map->dm_mapsize = 0;
        map->dm_nsegs = 0;

        if (buflen > map->_dm_size)
                return (EINVAL);

        seg = 0;
        error = _bus_dmamap_load_buffer(t, map, buf, buflen, p, flags,
                &lastaddr, &seg, 1);
        if (error == 0) {
                map->dm_mapsize = buflen;
                map->dm_nsegs = seg + 1;
        }
        return (error);
}

/*
 * Like _bus_dmamap_load(), but for mbufs.
 */
int
_bus_dmamap_load_mbuf(t, map, m0, flags)
        bus_dma_tag_t t;
        bus_dmamap_t map;
        struct mbuf *m0;
        int flags;
{
        paddr_t lastaddr;
        int seg, error, first;
        struct mbuf *m;

        /*
         * Make sure that on error condition we return "no valid mappings."
         */
        map->dm_mapsize = 0;
        map->dm_nsegs = 0;

#ifdef DIAGNOSTIC
        if ((m0->m_flags & M_PKTHDR) == 0)
                panic("_bus_dmamap_load_mbuf: no packet header");
#endif

        if (m0->m_pkthdr.len > map->_dm_size)
                return (EINVAL);

        first = 1;
        seg = 0;
        error = 0;
        for (m = m0; m != NULL && error == 0; m = m->m_next) {
		if (m->m_len == 0)
			continue;
                error = _bus_dmamap_load_buffer(t, map, m->m_data, m->m_len,
                    NULL, flags, &lastaddr, &seg, first);
                first = 0;
        }
        if (error == 0) {
                map->dm_mapsize = m0->m_pkthdr.len;
                map->dm_nsegs = seg + 1;
        }
        return (error);
}

/*
 * Like _bus_dmamap_load(), but for uios.
 */
int
_bus_dmamap_load_uio(t, map, uio, flags)
        bus_dma_tag_t t;
        bus_dmamap_t map;
        struct uio *uio;
        int flags;
{
        paddr_t lastaddr;
        int seg, i, error, first;
        bus_size_t minlen, resid;
        struct proc *p = NULL;
        struct iovec *iov;
        caddr_t addr;

        /*
         * Make sure that on error condition we return "no valid mappings."
         */
        map->dm_mapsize = 0;
        map->dm_nsegs = 0;

        resid = uio->uio_resid;
        iov = uio->uio_iov;

	if (resid > map->_dm_size)
		return (EINVAL);

        if (uio->uio_segflg == UIO_USERSPACE) {
                p = uio->uio_procp;
#ifdef DIAGNOSTIC
                if (p == NULL)
                        panic("_bus_dmamap_load_uio: USERSPACE but no proc");
#endif
        }

        first = 1;
        seg = 0;
        error = 0;
        for (i = 0; i < uio->uio_iovcnt && resid != 0 && error == 0; i++) {
                /*
                 * Now at the first iovec to load.  Load each iovec
                 * until we have exhausted the residual count.
                 */
                minlen = resid < iov[i].iov_len ? resid : iov[i].iov_len;
                addr = (caddr_t)iov[i].iov_base;

                error = _bus_dmamap_load_buffer(t, map, addr, minlen,
                    p, flags, &lastaddr, &seg, first);
                first = 0;

                resid -= minlen;
        }
        if (error == 0) {
                map->dm_mapsize = uio->uio_resid;
                map->dm_nsegs = seg + 1;
        }
        return (error);
}

/*
 * Like _bus_dmamap_load(), but for raw memory allocated with
 * bus_dmamem_alloc().
 */
int
_bus_dmamap_load_raw(t, map, segs, nsegs, size, flags)
        bus_dma_tag_t t;
        bus_dmamap_t map;
        bus_dma_segment_t *segs;
        int nsegs;
        bus_size_t size;
        int flags;
{

        panic("_bus_dmamap_load_raw: not implemented");
}

/*
 * Common function for unloading a DMA map.  May be called by
 * chipset-specific DMA map unload functions.
 */
void
_bus_dmamap_unload(t, map)
        bus_dma_tag_t t;
        bus_dmamap_t map;
{

        /*
         * No resources to free; just mark the mappings as
         * invalid.
         */
        map->dm_mapsize = 0;
        map->dm_nsegs = 0;
}

/*
 * Common function for DMA map synchronization.  May be called
 * by chipset-specific DMA map synchronization functions.
 */

void
_bus_dmamap_sync(t, map, offset, len, op)
        bus_dma_tag_t t;
        bus_dmamap_t map;
	bus_addr_t offset;
	bus_size_t len;
	int op;
{
	int i;

	/* XXX and if it's not a PCI device??? */
	if ((op & (BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE)) == 0)
		return;

	for (i = map->dm_nsegs; i--; )
		invdcache((void *)PCI_MEM_TO_PHYS(map->dm_segs[i].ds_addr), 
		    len);
}

/*
 * Common function for DMA-safe memory allocation.  May be called
 * by bus-specific DMA memory allocation functions.
 */
int
_bus_dmamem_alloc(t, size, alignment, boundary, segs, nsegs, rsegs, flags)
        bus_dma_tag_t t;
        bus_size_t size, alignment, boundary;
        bus_dma_segment_t *segs;
        int nsegs;
        int *rsegs;
        int flags;
{
        paddr_t avail_start = 0xffffffff, avail_end = 0;
        int bank;

        for (bank = 0; bank < vm_nphysseg; bank++) {
                if (avail_start > vm_physmem[bank].avail_start << PGSHIFT)
                        avail_start = vm_physmem[bank].avail_start << PGSHIFT;
                if (avail_end < vm_physmem[bank].avail_end << PGSHIFT)
                        avail_end = vm_physmem[bank].avail_end << PGSHIFT;
        }

        return _bus_dmamem_alloc_range(t, size, alignment, boundary, segs,
            nsegs, rsegs, flags, avail_start, avail_end - PAGE_SIZE);
}

/*
 * Common function for freeing DMA-safe memory.  May be called by
 * bus-specific DMA memory free functions.
 */
void
_bus_dmamem_free(t, segs, nsegs)
        bus_dma_tag_t t;
        bus_dma_segment_t *segs;
        int nsegs;
{
        struct vm_page *m;
        bus_addr_t addr;
        struct pglist mlist;
        int curseg;

        /*
         * Build a list of pages to free back to the VM system.
         */
        TAILQ_INIT(&mlist);
        for (curseg = 0; curseg < nsegs; curseg++) {
                for (addr = PCI_MEM_TO_PHYS(segs[curseg].ds_addr);
                    addr < (PCI_MEM_TO_PHYS(segs[curseg].ds_addr)
                        + segs[curseg].ds_len);
                    addr += PAGE_SIZE) {
                        m = PHYS_TO_VM_PAGE(addr);
                        TAILQ_INSERT_TAIL(&mlist, m, pageq);
                }
        }

        uvm_pglistfree(&mlist);
}

/*
 * Common function for mapping DMA-safe memory.  May be called by
 * bus-specific DMA memory map functions.
 */
int
_bus_dmamem_map(t, segs, nsegs, size, kvap, flags)
        bus_dma_tag_t t;
        bus_dma_segment_t *segs;
        int nsegs;
        size_t size;
        caddr_t *kvap;
        int flags;
{
        vaddr_t va;
        bus_addr_t addr;
        int curseg;

        size = round_page(size);

        va = uvm_km_valloc(kernel_map, size);

        if (va == 0)
                return (ENOMEM);

        *kvap = (caddr_t)va;

        for (curseg = 0; curseg < nsegs; curseg++) {
                for (addr = PCI_MEM_TO_PHYS(segs[curseg].ds_addr);
                    addr < (PCI_MEM_TO_PHYS(segs[curseg].ds_addr)
                        + segs[curseg].ds_len);
                    addr += NBPG, va += NBPG, size -= NBPG) {
                        if (size == 0)
                                panic("_bus_dmamem_map: size botch");
                        pmap_enter(pmap_kernel(), va, addr,
                            VM_PROT_READ | VM_PROT_WRITE,
                            VM_PROT_READ | VM_PROT_WRITE | PMAP_WIRED);
                }
        }
	pmap_update(pmap_kernel());

        return (0);
}

/*
 * Common function for unmapping DMA-safe memory.  May be called by
 * bus-specific DMA memory unmapping functions.
 */
void
_bus_dmamem_unmap(t, kva, size)
        bus_dma_tag_t t;
        caddr_t kva;
        size_t size;
{

#ifdef DIAGNOSTIC
        if ((u_long)kva & PGOFSET)
                panic("_bus_dmamem_unmap");
#endif

        size = round_page(size);

        uvm_km_free(kernel_map, (vaddr_t)kva, size);
}

/*
 * Common functin for mmap(2)'ing DMA-safe memory.  May be called by
 * bus-specific DMA mmap(2)'ing functions.
 */
paddr_t
_bus_dmamem_mmap(t, segs, nsegs, off, prot, flags)
        bus_dma_tag_t t;
        bus_dma_segment_t *segs;
        int nsegs;
        off_t off;
        int prot, flags;
{
        int i;

        for (i = 0; i < nsegs; i++) {
#ifdef DIAGNOSTIC
                if (off & PGOFSET)
                        panic("_bus_dmamem_mmap: offset unaligned");
                if (PCI_MEM_TO_PHYS(segs[i].ds_addr) & PGOFSET)
                        panic("_bus_dmamem_mmap: segment unaligned");
                if (segs[i].ds_len & PGOFSET)
                        panic("_bus_dmamem_mmap: segment size not multiple"
                            " of page size");
#endif
                if (off >= segs[i].ds_len) {
                        off -= segs[i].ds_len;
                        continue;
                }

                return (powerpc_btop(PCI_MEM_TO_PHYS(segs[i].ds_addr) + off));
        }

        /* Page not found. */
        return (-1);
}

/*
 * Allocate physical memory from the given physical address range.
 * Called by DMA-safe memory allocation methods.
 */
int
_bus_dmamem_alloc_range(t, size, alignment, boundary, segs, nsegs, rsegs,
    flags, low, high)
        bus_dma_tag_t t;
        bus_size_t size, alignment, boundary;
        bus_dma_segment_t *segs;
        int nsegs;
        int *rsegs;
        int flags;
        paddr_t low;
        paddr_t high;
{
        paddr_t curaddr, lastaddr;
        struct vm_page *m;
        struct pglist mlist;
        int curseg, error;

        /* Always round the size. */
        size = round_page(size);

        /*
         * Allocate pages from the VM system.
         */
        TAILQ_INIT(&mlist);
        error = uvm_pglistalloc(size, low, high, alignment, boundary,
            &mlist, nsegs, (flags & BUS_DMA_NOWAIT) == 0);
        if (error)
                return (error);

        /*
         * Compute the location, size, and number of segments actually
         * returned by the VM code.
         */
        m = mlist.tqh_first;
        curseg = 0;
        lastaddr = VM_PAGE_TO_PHYS(m);
        segs[curseg].ds_addr = PHYS_TO_PCI_MEM(lastaddr);
        segs[curseg].ds_len = PAGE_SIZE;
        m = m->pageq.tqe_next;

        for (; m != NULL; m = m->pageq.tqe_next) {
                curaddr = VM_PAGE_TO_PHYS(m);
#ifdef DIAGNOSTIC
                if (curaddr < low || curaddr >= high) {
                        printf("vm_page_alloc_memory returned non-sensical"
                            " address 0x%lx\n", curaddr);
                        panic("_bus_dmamem_alloc_range");
                }
#endif
                if (curaddr == (lastaddr + PAGE_SIZE))
                        segs[curseg].ds_len += PAGE_SIZE;
                else {
                        curseg++;
                        segs[curseg].ds_addr = PHYS_TO_PCI_MEM(curaddr);
                        segs[curseg].ds_len = PAGE_SIZE;
                }
                lastaddr = curaddr;
        }

        *rsegs = curseg + 1;

        return (0);
}

