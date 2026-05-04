#include "userprog/syscall.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "intrinsic.h"
#include "lib/kernel/console.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/loader.h"
#include "threads/mmu.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include "threads/palloc.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

struct lock filesys_lock;

static int sys_exec (const char *cmd_line); // 실행파일 선언
static void sys_halt (void);
void sys_exit (int status);
static bool sys_create (const char *file, unsigned initial_size);
static bool sys_remove (const char *file);
static int sys_open (const char *file_name);
static int sys_filesize (int fd);
static int sys_read (int fd, void *buffer, unsigned size);
static int sys_write (int fd, const void *buffer, unsigned size);
static void sys_seek (int fd, unsigned position);
static unsigned sys_tell (int fd);
static void sys_close (int fd);

static int fd_alloc (struct file *file);
static struct file *find_file_by_fd (int fd);
static bool file_name_is_empty (const char *file);
static bool file_name_is_too_long (const char *file);

static void fail_invalid_user_memory (void);
static bool is_valid_user_ptr (const void *uaddr);
static void validate_user_ptr (const void *uaddr);
static void validate_user_buffer (const void *buffer, size_t size);
static void validate_user_string (const char *str);

#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_SYSCALL_MASK 0xc0000084


static int //시스템 실행 시스템 콜 핸들러에서 호출되는 sys_exec() 
//함수를 구현한다.
sys_exec (const char *cmd_line) {
    validate_user_string (cmd_line);

    char *cmd_copy = palloc_get_page (0);
    if (cmd_copy == NULL)
        return -1;

    strlcpy (cmd_copy, cmd_line, PGSIZE);

    return process_exec (cmd_copy);
}



void
syscall_init (void) {
	lock_init (&filesys_lock);
	write_msr (MSR_STAR, ((uint64_t) SEL_UCSEG - 0x10) << 48 |
			((uint64_t) SEL_KCSEG) << 32);
	write_msr (MSR_LSTAR, (uint64_t) syscall_entry);
	write_msr (MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

static void
fail_invalid_user_memory (void) {
	sys_exit (-1);
}

static bool
is_valid_user_ptr (const void *uaddr) {
	if (uaddr == NULL)
		return false;
	if (!is_user_vaddr ((void *) uaddr))
		return false;
	if (pml4_get_page (thread_current ()->pml4, (void *) uaddr) == NULL)
		return false;
	return true;
}

static void
validate_user_ptr (const void *uaddr) {
	if (!is_valid_user_ptr (uaddr))
		fail_invalid_user_memory ();
}

static void
validate_user_buffer (const void *buffer, size_t size) {
	if (size == 0)
		return;

	validate_user_ptr (buffer);
	validate_user_ptr ((const uint8_t *) buffer + size - 1);

	for (const uint8_t *page = pg_round_down (buffer);
		 page <= (const uint8_t *) pg_round_down ((const uint8_t *) buffer + size - 1);
		 page += PGSIZE) {
		validate_user_ptr (page);
	}
}

static void
validate_user_string (const char *str) {
	validate_user_ptr (str);
	while (true) {
		validate_user_ptr (str);
		if (*str == '\0')
			return;
		str++;
	}
}

static bool
file_name_is_empty (const char *file) {
	return strlen (file) == 0;
}

static bool
file_name_is_too_long (const char *file) {
	return strlen (file) > NAME_MAX;
}

static struct file *
find_file_by_fd (int fd) {
	if (fd < 2 || fd >= ARG_MAX)
		return NULL;
	return thread_current ()->fd_table[fd];
}

static int
fd_alloc (struct file *file) {
	struct thread *curr = thread_current ();

	if (file == NULL)
		return -1;

	for (int fd = 2; fd < ARG_MAX; fd++) {
		if (curr->fd_table[fd] == NULL) {
			curr->fd_table[fd] = file;
			if (fd >= curr->next_fd)
				curr->next_fd = fd + 1;
			return fd;
		}
	}

	return -1;
}

static void
sys_halt (void) {
	power_off ();
}

void
sys_exit (int status) {
	struct thread *curr = thread_current ();
	curr->exit_status = status;
	thread_exit ();
}

static bool
sys_create (const char *file, unsigned initial_size) {
	bool success;

	validate_user_string (file);

	if (file_name_is_empty (file))
		return false;
	if (file_name_is_too_long (file))
		return false;

	lock_acquire (&filesys_lock);
	success = filesys_create (file, initial_size);
	lock_release (&filesys_lock);
	return success;
}

static bool
sys_remove (const char *file) {
	bool success;

	validate_user_string (file);

	if (file_name_is_empty (file))
		return false;
	if (file_name_is_too_long (file))
		return false;

	lock_acquire (&filesys_lock);
	success = filesys_remove (file);
	lock_release (&filesys_lock);
	return success;
}

static int
sys_open (const char *file_name) {
	struct file *file;
	int fd;

	validate_user_string (file_name);
	if (file_name_is_empty (file_name))
		return -1;

	lock_acquire (&filesys_lock);
	file = filesys_open (file_name);
	lock_release (&filesys_lock);
	if (file == NULL)
		return -1;

	fd = fd_alloc (file);
	if (fd == -1) {
		lock_acquire (&filesys_lock);
		file_close (file);
		lock_release (&filesys_lock);
	}
	return fd;
}

static int
sys_filesize (int fd) {
	struct file *file = find_file_by_fd (fd);
	if (file == NULL)
		return -1;

	lock_acquire (&filesys_lock);
	int length = file_length (file);
	lock_release (&filesys_lock);
	return length;
}

static int
sys_read (int fd, void *buffer, unsigned size) {
	validate_user_buffer (buffer, size);

	if (size == 0)
		return 0;
	if (fd == 1 || fd < 0 || fd >= ARG_MAX)
		return -1;

	if (fd == 0) {
		for (unsigned i = 0; i < size; i++)
			((uint8_t *) buffer)[i] = input_getc ();
		return size;
	}

	struct file *file = find_file_by_fd (fd);
	if (file == NULL)
		return -1;

	lock_acquire (&filesys_lock);
	int bytes_read = file_read (file, buffer, size);
	lock_release (&filesys_lock);
	return bytes_read;
}

static int
sys_write (int fd, const void *buffer, unsigned size) {
	validate_user_buffer (buffer, size);

	if (fd == 1) {
		putbuf (buffer, size);
		return size;
	}
	if (fd <= 0 || fd >= ARG_MAX)
		return -1;

	struct file *file = find_file_by_fd (fd);
	if (file == NULL)
		return -1;

	lock_acquire (&filesys_lock);
	int bytes_written = file_write (file, buffer, size);
	lock_release (&filesys_lock);
	return bytes_written;
}

static void
sys_seek (int fd, unsigned position) {
	struct file *file = find_file_by_fd (fd);
	if (file == NULL)
		return;

	lock_acquire (&filesys_lock);
	file_seek (file, position);
	lock_release (&filesys_lock);
}

static unsigned
sys_tell (int fd) {
	struct file *file = find_file_by_fd (fd);
	if (file == NULL)
		return (unsigned) -1;

	lock_acquire (&filesys_lock);
	unsigned position = file_tell (file);
	lock_release (&filesys_lock);
	return position;
}

static void
sys_close (int fd) {
	struct thread *curr = thread_current ();
	struct file *file = find_file_by_fd (fd);

	if (file == NULL)
		return;

	lock_acquire (&filesys_lock);
	file_close (file);
	lock_release (&filesys_lock);
	curr->fd_table[fd] = NULL;
}

void
syscall_handler (struct intr_frame *f UNUSED) {
	int sys_call = f->R.rax;

	switch (sys_call) {
		case SYS_HALT:
			sys_halt ();
			break;
		case SYS_EXIT:
			sys_exit (f->R.rdi);
			break;
		case SYS_FORK:
			f->R.rax = process_fork ((const char *) f->R.rdi, f);
			break;
		case SYS_WAIT:
			f->R.rax = process_wait ((tid_t) f->R.rdi);
			break;
		case SYS_CREATE:
			f->R.rax = sys_create ((const char *) f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			f->R.rax = sys_remove ((const char *) f->R.rdi);
			break;
		case SYS_OPEN:
			f->R.rax = sys_open ((const char *) f->R.rdi);
			break;
		case SYS_FILESIZE:
			f->R.rax = sys_filesize (f->R.rdi);
			break;
		case SYS_READ:
			f->R.rax = sys_read (f->R.rdi, (void *) f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			f->R.rax = sys_write (f->R.rdi, (const void *) f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			sys_seek (f->R.rdi, f->R.rsi);
			break;
		case SYS_TELL:
			f->R.rax = sys_tell (f->R.rdi);
			break;
		case SYS_CLOSE:
			sys_close (f->R.rdi);
			break;
		case SYS_EXEC:
			f->R.rax = sys_exec ((const char *) f->R.rdi);
			break;
		default:
			sys_exit (-1);
			break;
	}
}
