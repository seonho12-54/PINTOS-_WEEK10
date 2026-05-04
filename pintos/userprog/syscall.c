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
#include "threads/flags.h"
#include "lib/kernel/console.h"
#include "intrinsic.h"
#include "lib/kernel/stdio.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "filesys/file.h"

// 평소에는 꺼두기
#define USER_MEM_DEBUG 0
#if USER_MEM_DEBUG
#define user_mem_debug(...) printf(__VA_ARGS__)
#else
#define user_mem_debug(...) ((void) 0)
#endif

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

// 시스템콜 함수
static int sys_write(int fd, const void *buffer, unsigned size);
static void sys_exit(int status);

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

// 헬퍼 함수 구현부 
static void 
fail_invalid_user_memory(void) {
	sys_exit(-1); 
}


static bool
is_valid_user_ptr(const void *uaddr) {

	// NULL 포인터를 실패 처리한다.
	if(uaddr == NULL){
		user_mem_debug("invalid user ptr: NULL\n");
		return false;
	}

	// is_user_vaddr()로 커널 주소를 차단한다.
	if(!is_user_vaddr((void *) uaddr)){
		user_mem_debug("invalid user ptr: kernel addr %p\n", uaddr);
		return false;
	}


	// 현재 thread의 page table에서 매핑 여부를 확인한다.
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



static int sys_write(int fd, const void *buffer, unsigned size)
{
	struct file *file;
	if (fd == 1)
	{
		putbuf(buffer, size);
		return size;
	}

	// fd가 2이상일때, 현재 프로세스의 fd table에서 fd에 해당하는 struct file *를 찾음 find_file_from_fd()
	// 없으면 return -1, 있으면 파일에 입력하는 함수 호출하고, 그 함수가 입력한 사이즈를 반환.
	// fd에 해당하는 file* 찾는 함수 호출해서 파일 가져옴
	// 있는지 없는지 검사
	// 있다면 파일에 입력하는 함수 호출
	// 함수가 반환하는 사이즈 그대로 반환

	if (fd >= 2)  {			
		// file = find_file_by_fd(fd);
		
		if (file == NULL) {
			return -1;
		}

		return file_write(file, buffer, size);		
	}
	if (fd == 0) {
		return -1;
	}
	if (fd < 0){
		return -1;
	}
}

static void
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

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.

	// 10번이 SYS_WRITE
	int sys_call = f->R.rax; 

	switch (sys_call) {
		case SYS_WRITE: 
			f->R.rax = sys_write(f->R.rdi, f->R.rsi, f->R.rdx);	
			break; 			
		case SYS_EXIT:
			sys_exit(f->R.rdi);		
			break; 
		case SYS_CREATE: 
			f->R.rax = sys_create(f->R.rdi, f->R.rsi); 		
			break; 
	}


}