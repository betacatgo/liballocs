#ifndef LIBALLOCS_PRIVATE_H_
#define LIBALLOCS_PRIVATE_H_

#ifndef VIS
#define VIS(v) //__attribute__((visibility( #v )))
#endif

/* x86_64 only, for now */
#if !defined(__x86_64__) && !defined(X86_64)
#error Unsupported architecture.
#endif
/* FIXME: more portable */
#define PAGE_SIZE 4096
#define LOG_PAGE_SIZE 12
#define PAGE_MASK ~((PAGE_SIZE - 1))

#ifdef __cplusplus
extern "C" {
typedef bool _Bool;
#endif

#include <stddef.h>
#include <stdint.h>
#include <link.h>
#include "heap_index.h" /* includes memtable */
#include "allocsmt.h"
#include "systrap.h"

#include "liballocs.h"

#ifndef likely
#define likely(cond) \
  __builtin_expect( (cond), 1 )
#endif
#ifndef unlikely
#define unlikely(cond) \
  __builtin_expect( (cond), 0 )
#endif

const char *
dynobj_name_from_dlpi_name(const char *dlpi_name, void *dlpi_addr)
		__attribute__((visibility("hidden")));
char execfile_name[4096] __attribute__((visibility("hidden")));
char *realpath_quick(const char *arg) __attribute__((visibility("hidden")));
const char *format_symbolic_address(const void *addr) __attribute__((visibility("hidden")));

#include "pageindex.h"

extern struct big_allocation *executable_data_segment_mapping_bigalloc __attribute__((visibility("hidden")));

void mmap_replacement(struct generic_syscall *s, post_handler *post) __attribute__((visibility("hidden")));
void munmap_replacement(struct generic_syscall *s, post_handler *post) __attribute__((visibility("hidden")));
void mremap_replacement(struct generic_syscall *s, post_handler *post) __attribute__((visibility("hidden")));
void __liballocs_systrap_init(void);
void __systrap_brk_hack(void);
int load_types_for_one_object(struct dl_phdr_info *, size_t, void *data) __attribute__((visibility("hidden")));
int load_and_init_allocsites_for_one_object(struct dl_phdr_info *, size_t, void *data) __attribute__((visibility("hidden")));
int link_stackaddr_and_static_allocs_for_one_object(struct dl_phdr_info *, size_t, void *data) __attribute__((visibility("hidden")));
void *(*orig_dlopen)(const char *, int) __attribute__((visibility("hidden")));
void *(*orig_memmove)(void *, const void *, unsigned long) __attribute__((visibility("hidden")));
int dl_for_one_object_phdrs(void *handle,
	int (*callback) (struct dl_phdr_info *info, size_t size, void *data),
	void *data) __attribute__((visibility("hidden")));
const char *format_symbolic_address(const void *addr) __attribute__((visibility("hidden")));

/* We contain our own private malloc, and we wrap it using the linker 
 * to keep track of whether it's active on the current thread. */
extern _Bool __thread __private_malloc_active __attribute__((visibility("hidden")));
extern _Bool __thread __private_calloc_active __attribute__((visibility("hidden")));
extern _Bool __thread __private_free_active __attribute__((visibility("hidden")));
extern _Bool __thread __private_realloc_active __attribute__((visibility("hidden")));
extern _Bool __thread __private_memalign_active __attribute__((visibility("hidden")));
extern _Bool __thread __private_posix_memalign_active __attribute__((visibility("hidden")));
extern _Bool __thread __private_malloc_usable_size_active __attribute__((visibility("hidden")));
void *__private_malloc(size_t);
void __private_free(void *);

extern FILE *stream_err;
#define debug_printf(lvl, fmt, ...) do { \
    if ((lvl) <= __liballocs_debug_level) { \
      fprintf(stream_err, "%s: " fmt, get_exe_basename(), ## __VA_ARGS__ );  \
    } \
  } while (0)

#ifndef NO_TLS
extern __thread void *__current_allocsite __attribute__((weak)); // defined by heap_index_hooks
#else
extern void *__current_allocsite __attribute__((weak)); // defined by heap_index_hooks
#endif
struct addrlist
{
	unsigned count;
	unsigned allocsz;
	void **addrs;
};
struct liballocs_err
{
	const char *message;
};
struct frame_uniqtype_and_offset
{
	struct uniqtype *u;
	unsigned o;
};

struct frame_uniqtype_and_offset 
vaddr_to_stack_uniqtype(const void *vaddr)
		__attribute__((visibility("hidden")));
struct uniqtype *
static_addr_to_uniqtype(const void *static_addr, void **out_object_start)
		__attribute__((visibility("hidden")));
#define META_OBJ_SUFFIX "-meta.so"
_Bool is_meta_object_for_lib(struct link_map *maybe_types, struct link_map *l)
			__attribute__((visibility("hidden")));

/* Each allocation site has a metadata record generated for it
 * by our toolchain extensions.
 * These come out in one big array, so naturally a given record
 * has some index within that array.
 * To create a global identifier, we just remember a "start" value
 * for each loaded object. The global index of an allocation site
 * is its per-object index plus that object's "start" value.
 *
 * If we want to look up an alloc site's address by its index,
 * we get the base index for its object
 * and then add its offset.
 *
 * We can also look up an alloc site by its address.
 * Since each object's allocation site metadata records are also
 * sorted by the site address within that object, we simply
 * have to identify the object and then binary-search within
 * that object's sorted vector of allocation site records.
 *
 * FIXME: this is a bit too separate from the bigalloc stuff for my liking.
 * Is "allocation site" a central concept? Arguably so.
 *
 * FIXME: what if we want to redefine a given function / move its allocsites / insert a new one?
 * If we split the id into segment#, per-segment-id#, is that workable?
 * Suppose we use 10 bits for segment. Then we have 6 bits. So no. Even 8-8 is no-go.
 * (Arguably 16 bits for the id is not enough. But it's only for callers
 * of malloc/similar, remember.)
 * Maybe on updating some code, a global rewrite of allocsite ids could
 * be done. This is all academic for now. Much of the other static metadata stuff,
 * i.e. the sorted meta-vector, is also per-file or per-segment.
 *
 * FIXME: this should probably be part of the extrasyms or at least the
 * sorted meta-vector. Remember we have
 *
 * alloc sites (heap)   <-- we're thinking about these
 * static alloc sites (being replaced by sorted meta vec)
 * frame alloc sites  (like heap alloc sites but one extra field)
 *
 * and for the static alloc sites, we add type info *and* link back to
 * a symbol (or a spent reloc record? No; the meta-info is about the *target*
 * of the reloc, which may be the target of *many* reloc records; unclear
 * what it would mean to pick one... I think that field should just be 0).
 
	struct sym_or_reloc_rec
	{
		unsigned kind:2; // an instance of sym_or_reloc_kind
		unsigned idx:18; // i.e. at most 256K symbols of each kind, per file
		unsigned long uniqtype_ptr_bits:44; // i.e. the low-order 3 bits are 0
	} *sorted_meta_vec; // addr-sorted list of relevant dynsym/symtab/extrasym/reloc entries

 * Note that the sorted meta vec entry doesn't store the address of the
 * object it describes -- for that we have to indirect into the symtab
 * or to use the 'starts' bitmap.
 *
 * These patterns -- spines, sorted arrays, bitmaps and "next index" shortcut
 * arrays -- seem to be popping up in a few places. It would be good to have
 * one implementation of them.
 * Our plan for the starts bitmap was 
 * for that.
 * So in the case of relocs, we have to encode the section/symbol *and*
 * (perhaps) addend that gives us the target address.
 *
 *
 * We could make this *sorted_meta_vec[N_META_KINDS]
 * and allow additional per-file metadata kinds, of which heap allocsites
 * would naturally be one, and frame allocsites would naturally be another.

 * 
 */
/* This is effectively a "spine" linking all the per-file
 * allocsite metadata vectors. */
struct allocsites_by_id_entry
{
	unsigned short start_id;
	unsigned short count;
	struct allocsite_entry *ptr;
};
struct allocsites_by_object_base_address_entry
{
	const void *object_base_address;
	unsigned short start_id_plus_one; // so that zero~uninit == max index
	struct allocsites_by_id_entry *by_id; /* These don't move once created. */
};
#define ALLOCSITES_INDEX_SIZE 256 /* i.e. up to 256 objects with allocsite metadata */
extern struct allocsites_by_id_entry allocsites_by_id[ALLOCSITES_INDEX_SIZE];

#define MAX_EARLY_LIBS 128
extern struct link_map *early_lib_handles[MAX_EARLY_LIBS] __attribute((visibility("hidden")));

extern struct allocsites_by_object_base_address_entry allocsites_by_object_base_address[ALLOCSITES_INDEX_SIZE];
unsigned issue_allocsites_ids(unsigned count, struct allocsite_entry *first_entry,
	const void *object_base_address) __attribute__((visibility("hidden")));
struct allocsites_by_object_base_address_entry *__liballocs_find_allocsite_lookup_entry(
		const void *alloc_site_addr) __attribute__((visibility("hidden")));

/* avoid dependency on libc headers (in this header only) */
void __assert_fail(const char *assertion, 
	const char *file, unsigned int line, const char *function);
void warnx(const char *fmt, ...);
unsigned long malloc_usable_size (void *ptr);

/* counters */
extern unsigned long __liballocs_aborted_stack;
extern unsigned long __liballocs_aborted_static;
extern unsigned long __liballocs_aborted_unknown_storage;
extern unsigned long __liballocs_hit_heap_case;
extern unsigned long __liballocs_hit_stack_case;
extern unsigned long __liballocs_hit_static_case;
extern unsigned long __liballocs_aborted_unindexed_heap;
extern unsigned long __liballocs_aborted_unrecognised_allocsite;

/* We're allowed to malloc, thanks to __private_malloc(), but we 
 * we shouldn't call strdup because libc will do the malloc. */
char *__liballocs_private_strdup(const char *s) __attribute__((visibility("hidden")));
char *__liballocs_private_strndup(const char *s, size_t n) __attribute__((visibility("hidden")));

/* Our handling of mmap is in two phases: before systrapping enabled,
 * and after. */
extern _Bool __liballocs_systrap_is_initialized;
void __liballocs_post_systrap_init(void) __attribute__((visibility("hidden")));

void __generic_malloc_allocator_init(void) __attribute__((visibility("hidden")));

/* If this weak function is defined, it will be called when we've loaded
 * the metadata for one object. */
int __hook_loaded_one_object_meta(struct dl_phdr_info *info, size_t size, void *meta_object_handle) __attribute__((weak));
int load_and_init_all_metadata_for_one_object(struct dl_phdr_info *info, size_t size, void *out_meta_handle)
	__attribute__((visibility("hidden")));

void __notify_copy(void *dest, const void *src, unsigned long n);
void __notify_free(void *dest);

/* Some boilerplate helpers for use by allocators. */
#define DEFAULT_GET_TYPE \
static struct uniqtype *get_type(void *obj) \
{ \
	struct uniqtype *out; \
	struct liballocs_err *err = get_info(obj, NULL, &out, \
		NULL, NULL, NULL); \
	if (err) return NULL; \
	return out; \
}

extern struct uniqtype *pointer_to___uniqtype__void __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__unsigned_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____uninterpreted_byte __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____PTR_signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____PTR___PTR_signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__long_unsigned_int __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__long_int __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__Elf64_auxv_t __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype____ARR0_signed_char __attribute__((visibility("hidden")));
extern struct uniqtype *pointer_to___uniqtype__intptr_t __attribute__((visibility("hidden")));
#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif
