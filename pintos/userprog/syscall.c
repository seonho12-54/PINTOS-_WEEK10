#include "userprog/syscall.h"
#include <stdio.h>
#include <stdbool.h> 
#include <stddef.h> 
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include "threads/flags.h"
#include "lib/kernel/console.h"
#include "intrinsic.h"
#include "lib/kernel/stdio.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "threads/init.h"  // power_off 함수
#include "filesys/file.h"  // file_write 함수
#include "devices/input.h" //sys_read

// 평소에는 꺼두기
#define USER_MEM_DEBUG 0
#if USER_MEM_DEBUG
#define user_mem_debug(...) printf(__VA_ARGS__)
#else
#define user_mem_debug(...) ((void) 0)
#endif


void syscall_entry(void);
void syscall_handler(struct intr_frame *);

// 시스템콜 함수
static int sys_write(int fd, const void *buffer, unsigned size);
static int sys_open(const char *file);
void sys_exit(int status);

// 기본 헬퍼 함수 
static struct file* find_file_by_fd(int fd); 

// 유저 메모리 유효성 검사 함수
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

#define MSR_STAR 0xc0000081			/* Segment selector msr */
#define MSR_LSTAR 0xc0000082		/* Long mode SYSCALL target */
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

static int sys_write(int fd, const void *buffer, unsigned size)
{
	struct file *file;
	//표준출력, 버퍼에서 size만큼 읽어서 터미널에 출력
	if (fd == 1)
	{
		putbuf(buffer, size);
		return size;
	}

	//fd_table[fd]의 file*를 가져옴
	file = find_file_by_fd(fd);
	if (file == NULL)
		return -1;
	
	//fd가 2이상이면 일반파일, 찾아온 file에 버퍼 내용을 size만큼 쓰기
	if (fd >= 2)
		return file_write(file, buffer, size);

	if (fd==0)
		return -1;
	if (fd < 0)
		return -1;
}

static int sys_read(int fd, void *buffer, unsigned size)
{
	struct file *file;

	if (size == 0)
		return 0;

	if (fd == 0) // 표준입력,  size만큼 반복, 문자 하나를 읽어서 버퍼에 저장후, size반환
	{
		for (int i = 0; i<size; i++)
		{
			((uint8_t *) buffer)[i]=input_getc();
		}
		return size;
	}

	if (fd == 1)
		return -1;

	if (fd >= 2)
	{
		file = find_file_by_fd(fd);
		if (file == NULL)
			return -1;
		return file_read(file, buffer, size);
	}

	return -1;   //나머지 경우 처리
}

static int 
sys_open(const char *file_name) {
	if (!is_valid_user_ptr(file_name)) {
		sys_exit(-1);
	}

	// sys_open이 filesys_open(file_name)을 호출 
	// filesys_open이 디렉터리에서 파일 이름을 찾고 inode를 얻음
	// filesys_open 내부에서 file_open(inode) 호출
	// file_open이 struct file * 객체를 만들어 반환
	struct file* file = filesys_open(file_name); 
	struct thread *curr_thread = thread_current(); 

	curr_thread->fd_table[curr_thread->next_fd] = file; 
	
	return curr_thread->next_fd++; 
}

void
sys_exit(int status) {
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_exit();
}

static bool 
file_name_is_empty(const char *file) {
	return strlen(file) == 0; 
}

static bool 
file_name_is_too_long(const char *file) {
	return strlen(file) > NAME_MAX; 
}

static bool 
sys_create(const char *file, unsigned initial_size) {
	if (!is_valid_user_ptr(file)) {
		sys_exit(-1);
	}
	 
	validate_user_string(file); 

	if (file_name_is_empty(file)) {
    	return false;
	}

	if (file_name_is_too_long(file)) {
		return false; 
	}
	
	bool is_file_created = filesys_create(file, initial_size);

	if (!is_file_created) {
		return false; 
	}

	return true; 
}

static void
sys_halt(void)
{
	power_off();
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f UNUSED)
{
	// TODO: Your implementation goes here.

	// 10번이 SYS_WRITE
	int sys_call = f->R.rax; 

	switch (sys_call) {
		case SYS_WRITE:
			f->R.rax = sys_write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_OPEN: 
			f->R.rax = sys_open(f->R.rdi); 
			break; 
		case SYS_EXIT:
			sys_exit(f->R.rdi);		
			break; 
		case SYS_CREATE: 
			f->R.rax = sys_create(f->R.rdi, f->R.rsi); 		
			break; 
	}

	int sys_call = f->R.rax;

	switch (sys_call)
	{
	case SYS_WRITE:
		f->R.rax = sys_write(f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	case SYS_EXIT:
		sys_exit(f->R.rdi);
		break;
	case SYS_HALT:
		sys_halt();
		break;
	default:
		sys_exit(-1);
		break;
	}
}
