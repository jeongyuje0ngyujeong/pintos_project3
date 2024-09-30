/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "hash.h"

/* 태현 추가 */
#include "threads/mmu.h"
#include "userprog/process.h"
#include <string.h>

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		/* 태현 추가 */
		struct page *new_page = malloc (sizeof (struct page));
		if (new_page == NULL) {
			goto err;
		}

		switch (VM_TYPE(type)) {
			case VM_ANON :
				uninit_new (new_page, pg_round_down(upage), init, VM_ANON, aux, anon_initializer);
				break;
			case VM_FILE :
				uninit_new (new_page, pg_round_down(upage), init, VM_FILE, aux, file_backed_initializer);
				break;
			default :
				goto err;
		}

		/* TODO: Insert the page into the spt. */
		new_page->writable = writable;
		/* 태현 추가 */
		return spt_insert_page(spt, new_page);
		// if (spt_find_page (spt, new_page->va) == NULL) {
		// 	free(new_page);
		// 	goto err;
		// }
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = malloc(sizeof(struct page));
	/* TODO: Fill this function. */
	/* 태현 추가 */
	page->va = pg_round_down(va);
	struct hash_elem *e;
	e = hash_find (&spt->hash, &page->hash_elem);
	free(page);
	if (e == NULL) {
		return NULL;
	}
	return hash_entry (e, struct page, hash_elem);
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED, struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	/* 태현 추가 */
	struct hash_elem *e;
	e = hash_find (&spt->hash, &page->hash_elem);
	if (e != NULL) {
		return succ;
	}
	hash_insert (&spt->hash, &page->hash_elem);
	return succ = true;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	/* 태현 추가 */
	void *kva = palloc_get_page (PAL_USER|PAL_ZERO);
	if (kva == NULL) {
		PANIC ("todo");
		// frame = vm_evict_frame ();
	}
	else {
		frame = malloc (sizeof (struct frame));
		frame->kva = kva;
		frame->page = NULL;
	}
	ASSERT (frame != NULL);
	ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	vm_alloc_page(VM_ANON | VM_MARKER_0, pg_round_down(addr), 1);
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	/* 태현 추가 */
	if (addr == NULL){
        return false;
	}
	if (is_kernel_vaddr(addr)) {
        return false;
	}
	if (not_present) {
		void *rsp = f->rsp;
		if (!user) {
			rsp = thread_current()->rsp;
		}
		if ((addr >= (uint8_t *)rsp - PGSIZE) && 
            (addr < USER_STACK) && 
            (addr >= USER_STACK - (1 << 20))) 
                vm_stack_growth(addr);
		page = spt_find_page(spt, addr);
		if (page == NULL) {
			return false;
		}
		if (write == 1 && page->writable == 0) {
            return false;
		}
		return vm_do_claim_page (page);
	}
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
	/* 태현 추가 */
	page = spt_find_page (&thread_current ()->spt, va);
	if (page == NULL) {
		return false;
	}
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* 태현 추가 */
	pml4_set_page (thread_current()->pml4, page->va, frame->kva, page->writable);

	return swap_in (page, frame->kva);
}

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	/* 태현 추가 */
	hash_init (&spt->hash, page_hash, page_less, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	/* 태현 추가 */
	struct hash_iterator i;
	hash_first(&i, &src->hash);
	while(hash_next(&i)) {
		struct page *page = hash_entry(i.elem, struct page, hash_elem);
		if (VM_TYPE(page->operations->type) == VM_UNINIT) {
			struct aux *aux = malloc(sizeof(struct aux));
			memcpy(aux, page->uninit.aux, sizeof(struct aux));
			if (!vm_alloc_page_with_initializer(page_get_type(page), page->va, page->writable, page->uninit.init, aux)) {
				goto err;
			}
		}
		else {
			if (!vm_alloc_page_with_initializer(page_get_type(page), page->va, page->writable, NULL, NULL)) {
				goto err;
			}
			struct page *new_page = spt_find_page(dst, page->va);
			if (!vm_do_claim_page(new_page)) {
				goto err;
			}
			memcpy(new_page->frame->kva, page->frame->kva, PGSIZE);
		}
	}
	return true;
err:
	supplemental_page_table_kill(dst);
	return false;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
	/* 태현 추가 */
	hash_clear (&spt->hash, spt_remove_func);
}

void
spt_remove_func (struct hash_elem *elem, void *aux UNUSED) {
	struct page *page = hash_entry(elem, struct page, hash_elem);
	free(page);
	return true;
}

/* 태현 추가 */
unsigned
page_hash (struct hash_elem *p, void *aux UNUSED) {
	struct page *page = hash_entry (p, struct page, hash_elem);
	return hash_bytes (&page->va, sizeof page->va);
}

bool
page_less (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	struct page *page_a = hash_entry (a, struct page, hash_elem);
	struct page *page_b = hash_entry (b, struct page, hash_elem);
	return page_a->va < page_b->va;
}