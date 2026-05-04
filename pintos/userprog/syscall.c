#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "lib/kernel/console.h"
#include "intrinsic.h"
#include "lib/kernel/stdio.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "threads/palloc.h"
#include "userprog/process.h"

#define USER_MEM_DEBUG 0
#if USER_MEM_DEBUG
#define user_mem_debug(...) printf(__VA_ARGS__)
#else
#define user_mem_debug(...) ((void) 0)
#endif

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

static int sys_write(int fd, const void *buffer, unsigned size);
void sys_exit(int status);

static void fail_invalid_user_memory(void);
static bool is_valid_user_ptr(const void *uaddr);
static void validate_user_ptr(const void *uaddr);
static void validate_user_buffer(const void *buffer, size_t size);
static void validate_user_string(const char *str);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

static void
fail_invalid_user_memory(void) {
	sys_exit(-1);
}

static bool
is_valid_user_ptr(const void *uaddr) {

	if(uaddr == NULL){
		user_mem_debug("invalid user ptr: NULL\n");
		return false;
	}

	if(!is_user_vaddr((void *) uaddr)){
		user_mem_debug("invalid user ptr: kernel addr %p\n", uaddr);
		return false;
	}

	if(pml4_get_page(thread_current()->pml4, (void *) uaddr) == NULL){
		user_mem_debug("invalid user ptr: unmapped %p\n", uaddr);
		return false;
	}

	return true;
}

static void
validate_user_ptr(const void *uaddr) {

	if (!is_valid_user_ptr(uaddr)) {
		fail_invalid_user_memory();
	}
}

static void
validate_user_buffer(const void *buffer, size_t size) {
	if (size == 0) {
		return;
	}
	validate_user_ptr(buffer);
	validate_user_ptr((const uint8_t *) buffer + size - 1);

	for (const uint8_t *page = pg_round_down(buffer);
		 page <= (const uint8_t *) pg_round_down((const uint8_t *) buffer + size - 1);
		 page += PGSIZE) {
		validate_user_ptr(page);
	}
}

static void
validate_user_string(const char *str) {
	validate_user_ptr(str);

	while (true) {
		validate_user_ptr(str);
		if (*str == '\0') {
			return;
		}
		str++;
	}
}

static int sys_write(int fd, const void *buffer, unsigned size) {
	if (fd == 1) {
		putbuf(buffer, size);
		return size;
	}
	return 0;
}

void
sys_exit(int status) {
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_exit();
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	int sys_call = f->R.rax;

	switch (sys_call) {
		case SYS_WRITE:
			f->R.rax = sys_write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_EXIT:
			sys_exit(f->R.rdi);
			break;
		case SYS_EXEC: {
			char *copied_cmd;

			validate_user_string((const char *) f->R.rdi);
			copied_cmd = palloc_get_page(0);
			if (copied_cmd == NULL) {
				f->R.rax = -1;
				break;
			}
			strlcpy (copied_cmd, (const char *) f->R.rdi, PGSIZE);
			f->R.rax = process_exec (copied_cmd);
			break;
		}
		default:
			break;
	}
}
