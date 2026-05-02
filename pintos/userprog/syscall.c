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
#include "threads/init.h" // power_off 함수
#include "filesys/file.h" // file_write 함수 
void syscall_entry(void);
void syscall_handler(struct intr_frame *);

// 시스템콜 함수
static int sys_write(int fd, const void *buffer, unsigned size);

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

void syscall_init(void)
{
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

static int sys_write(int fd, const void *buffer, unsigned size)
{
	//struct file * file;
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
	//file = find_file_from_fd(fd, )
	// if (file == NULL)
	// 	return -1;
	
	// return file_write(file, buffer, size);
}

static void
sys_exit(int status)
{
	printf("%s: exit(%d)\n", thread_current()->name, status);
	thread_exit();
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