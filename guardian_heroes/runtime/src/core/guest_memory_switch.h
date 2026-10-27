/*
 * Horizon guest-memory core from nebadasSwifty/masseffect-nx, commit a85b0b419ec3e332165ae1617c980bd399856a44.
 * Copyright (c) 2026 Tom Clay; portions Copyright (c) 2022 Ben Vanik and
 * Xenia contributors. BSD-3-Clause; see LICENSE.switch-memory in this directory.
 * Guardian Heroes changes: explicit loader failure and strict RW/no-access
 * protection. Prototype's logical-read-only / writable-memory bypass is unused.
 */
/*
 * Sparse Horizon guest arena. Reserve 4.5 GiB of virtual address space, then
 * allocate physical backing in 2 MiB chunks only when committed. Heap backing
 * is borrowed by svcMapProcessCodeMemory, made RW through a private shadow,
 * and aliased into guest views by svcMapProcessMemory. The original heap
 * pointer is inaccessible until the code alias is unmapped.
 *
 * Views are mapped explicitly with RexGmFaultIn, not by an installed exception
 * handler. Only RW and NONE are supported. Read-only and write-watching require
 * an exception/emulation path not provided by this core. NONE is real unmapping.
 * The loader must supply a real process handle; pseudo handles are rejected.
 */
#pragma once

#if defined(__SWITCH__)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Logical permissions, with the same values as rex::memory::PageAccess so that
 * the conversion is the identity.
 */
typedef enum {
    REX_GM_NONE  = 0,
    REX_GM_READ  = 1 << 0,
    REX_GM_WRITE = (1 << 0) | (1 << 1),  /* read + write */
} RexGmAccess;

/* Reserves the window. size is usually 0x120000000. Returns the base or NULL. */
uint8_t* RexGmInit(size_t size);

/* Releases everything. On syscall failure returns false and retains borrowed backing. */
bool RexGmShutdown(void);

/* Base of the window, or NULL if not initialized. */
uint8_t* RexGmBase(void);

/* Size of the reserved window, or 0 if not initialized. */
size_t RexGmSize(void);

/**
 * Translates a window address into the mapping offset it reflects, according to
 * the view that contains it. Returns false if it falls outside every view.
 *
 * A range [addr, addr+len) is translated by its first address, so the caller
 * must guarantee that it does not cross from one view into another. The SDK heaps
 * satisfy this by construction: each heap lives inside a single view.
 */
bool RexGmWindowToOffset(uint64_t window_address, size_t* offset_out);

/**
 * Registers a view: the part of the window that starts at `base` and is
 * `length` long reflects the mapping from offset `offset` onwards.
 *
 * Several views can share the same `offset`: that is exactly what creates the
 * 360 mirrors. Committing once fills them all.
 *
 * It does not map anything by itself; what is already committed is mapped on
 * the fly.
 */
bool RexGmAddView(uint8_t* base, size_t offset, size_t length);

/**
 * Commits real memory in [offset, offset+length) of the mapping. Each view that
 * covers it maps it on first access (RexGmFaultIn).
 *
 * Idempotent: what is already committed is left as it is.
 */
bool RexGmCommit(size_t offset, size_t length, RexGmAccess access);

/* The reverse. Frees the backing. */
bool RexGmDecommit(size_t offset, size_t length);

/*
 * Maps into the window the chunk that covers window_address, and only in the
 * view that address belongs to. Chunks are not mapped when they are committed:
 * the Xbox 360 sees the same memory from up to five addresses, and mapping every
 * chunk into all of them exhausts the process limit on mappable memory
 * (2001-0103, resource exhausted), which is the real ceiling on Horizon.
 *
 * Returns true if it mapped something, that is, if the faulting access can now
 * be retried. Returns false if there is no committed chunk there or it was
 * already mapped: then the fault is something else. Called by the exception
 * handler.
 */
bool RexGmFaultIn(uint64_t window_address);

/* Changes RW/NONE protection in one view. READ is rejected without mutation.
 * A failed syscall may leave a prefix changed; false must stop the caller.
 */
bool RexGmProtect(uint8_t* address, size_t length, RexGmAccess access, RexGmAccess* out_old);

/* Queries the logical permissions. length receives the size of the uniform range. */
bool RexGmQueryProtect(uint8_t* address, size_t* length, RexGmAccess* out_access);

/**
 * Equivalent address in the shadow alias, always accessible even where the
 * window is unmapped. It is what the exception handler emulates through.
 *
 * Returns NULL if that address is not committed.
 */
void* RexGmShadowFor(uint64_t window_address);

/* Bytes of backing committed right now. For diagnostics. */
size_t RexGmCommittedBytes(void);

/* Number of committed chunks (each is a code alias and up to one mapping per view). */
size_t RexGmChunkCount(void);

/**
 * Bytes mapped in the window, counting each mirror separately.
 *
 * This is the figure that hits Horizon's mapping ceiling (MarathonRecomp-NX puts
 * it at ~2.25 GB). With the ReXGlue memory map, physical memory alone appears in
 * five views, so this figure grows much faster than the backing one.
 */
size_t RexGmMappedBytes(void);

/* Always 2: protection is implemented by unmapping, never a writable RO alias. */
int RexGmProtectionMode(void);

/**
 * Result of the last system call that failed, or one of the REX_GM_ERR_* codes.
 * 0 if nothing has failed.
 *
 * Failure semantics of RexGmCommit: the failing range is undone entirely, so
 * nothing is left half committed. What earlier ranges of the same call
 * committed stays, because committing is idempotent and a retry resumes where
 * it stopped.
 */
uint32_t RexGmLastResult(void);

#define REX_GM_ERR_NOHANDLE 0xFFFF0003u /* loader omitted a real process handle */
#define REX_GM_ERR_NOVIRTUAL 0xFFFF0004u /* virtual reservation unavailable */
#define REX_GM_ERR_UNSUPPORTED 0xFFFF0005u /* requires exception emulation */
#define REX_GM_ERR_NOMEM    0xFFFF0001u  /* memalign of the backing store failed */
#define REX_GM_ERR_NOCODEVA 0xFFFF0002u  /* no free space left in the code region */

#ifdef __cplusplus
}
#endif

#endif /* __SWITCH__ */
