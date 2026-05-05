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
#include "threads/palloc.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include "threads/flags.h"
#include "lib/kernel/console.h"
#include "intrinsic.h"
#include "lib/kernel/stdio.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "threads/init.h"  // power_off 함수
#include "filesys/file.h"  // file_write 함수
#include "devices/input.h" //sys_read

// 평소에는 꺼두기
#define USER_MEM_DEBUG 0
#if USER_MEM_DEBUG
#define user_mem_debug(...) printf(__VA_ARGS__)
#else
#define user_mem_debug(...) ((void)0)
#endif

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

// 시스템콜 함수
static int sys_write(int fd, const void *buffer, unsigned size);
static int sys_open(const char *file);
static void sys_close(int fd);
static int sys_read(int fd, void *buffer, unsigned size);
static bool sys_create(const char *file, unsigned initial_size);
static bool sys_remove(const char *file);
static int sys_filesize(int fd);
static void sys_seek(int fd, unsigned position);
static unsigned sys_tell(int fd);
static int sys_exec(const char *cmd_line);
static void sys_halt(void);
void sys_exit(int status);

// 기본 헬퍼 함수
static int fd_alloc(struct file *file);
static struct file *find_file_by_fd(int fd);
static bool file_name_is_empty(const char *file);
static bool file_name_is_too_long(const char *file);

// 유저 메모리 유효성 검사 함수
static void fail_invalid_user_memory(void);
static bool is_valid_user_ptr(const void *uaddr);
static void validate_user_ptr(const void *uaddr);
static void validate_user_buffer(const void *buffer, size_t size);
static void validate_user_string(const char *str);
static struct lock filesys_lock;

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
	lock_init(&filesys_lock);
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |
							((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			  FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

static bool
file_name_is_empty(const char *file)
{
	return strlen(file) == 0;
}

static bool
file_name_is_too_long(const char *file)
{
	return strlen(file) > NAME_MAX;
}

// 유저 메모리 유효성 검사 함수들
// 실패 종료 경로 함수
static void
fail_invalid_user_memory(void)
{
	// 잘못된 사용자 메모리는 현재 프로세스를 exit(-1)로 종료한다.
	// validate_*() 계열 helper의 공통 실패 경로로 사용한다.
	// 호출 이후 정상 syscall 반환값을 만들지 않는다.
	sys_exit(-1);
}

// 접근 가능한 사용자 주소인지 판별하는 함수
static bool
is_valid_user_ptr(const void *uaddr)
{

	// NULL 포인터를 실패 처리한다.
	if (uaddr == NULL)
	{
		user_mem_debug("invalid user ptr: NULL\n");
		return false;
	}

	// is_user_vaddr()로 커널 주소를 차단한다.
	if (!is_user_vaddr((void *)uaddr))
	{
		user_mem_debug("invalid user ptr: kernel addr %p\n", uaddr);
		return false;
	}

	// 현재 thread의 page table에서 매핑 여부를 확인한다.
	if (pml4_get_page(thread_current()->pml4, (void *)uaddr) == NULL)
	{
		user_mem_debug("invalid user ptr: unmapped %p\n", uaddr);
		return false;
	}

	return true;
}

// 단일 포인터 검증 함수
static void
validate_user_ptr(const void *uaddr)
{

	// 규칙 1: 내부 판별은 is_valid_user_ptr()에 위임한다.
	if (!is_valid_user_ptr(uaddr))
	{
		// 실패 시 fail_invalid_user_memory()를 호출한다.
		fail_invalid_user_memory();
	}
}

static void
validate_user_buffer(const void *buffer, size_t size)
{
	// size == 0은 빈 범위로 처리한다.
	if (size == 0)
	{
		return;
	}
	// 시작 주소를 검증한다.
	validate_user_ptr(buffer);
	// buffer가 가리키는 메모리 범위의 마지막 바이트도 유효한 사용자 주소인지 확인한다
	validate_user_ptr((const uint8_t *)buffer + size - 1);

	for (const uint8_t *page = pg_round_down(buffer);
		 page <= (const uint8_t *)pg_round_down((const uint8_t *)buffer + size - 1);
		 page += PGSIZE)
	{
		validate_user_ptr(page);
	}
}

static void
validate_user_string(const char *str)
{
	// 규칙 1: 시작 주소뿐 아니라 각 문자 위치를 검증한다.
	validate_user_ptr(str);

	// 규칙 2: NUL 종료를 발견하면 검증을 종료한다.
	while (true)
	{
		validate_user_ptr(str);
		if (*str == '\0')
		{
			return;
		}
		str++;
	}
}

// 기본 헬퍼 함수
static struct file *find_file_by_fd(int fd)
{
	if (fd < 2 || fd >= ARG_MAX)
		return NULL;
	//유저 프로그램이 넘겨준 fd로 현재 프로세스의 fd_table에서 file*를 찾아 반환
	return thread_current()->fd_table[fd];
}

static int fd_alloc(struct file *file)
{
	struct thread *curr = thread_current();

	if (file == NULL)
		return -1;

	for (int fd = 2; fd < ARG_MAX; fd++)
	{
		if (curr->fd_table[fd] == NULL)
		{
			curr->fd_table[fd] = file;
			if (fd >= curr->next_fd)
				curr->next_fd = fd + 1;
			return fd;
		}
	}

	return -1;
}

static int sys_write(int fd, const void *buffer, unsigned size)
{
	struct file *file;
	//포인터 검사
	validate_user_buffer(buffer, size);
	// 표준출력, 버퍼에서 size만큼 읽어서 터미널에 출력
	if (fd == 1)
	{
		putbuf(buffer, size);
		return size;
	}
	if (fd <= 0)
	{
		return -1;
	}
	if (fd >= ARG_MAX)
	{
		return -1;
	}

	// fd_table[fd]의 file*를 가져옴
	file = find_file_by_fd(fd);
	if (file == NULL)
		return -1;

	// fd가 2이상이면 일반파일, 찾아온 file에 버퍼 내용을 size만큼 쓰기
	lock_acquire(&filesys_lock);
	int bytes_written = file_write(file, buffer, size);
	lock_release(&filesys_lock);
	return bytes_written;
}

// 파일 안에 데이터가 몇 바이트인지 확인
static int sys_filesize(int fd)
{
	struct file *file;
	if (fd < 2)
	{
		return -1;
	}
	if (fd >= ARG_MAX)
	{
		return -1;
	}
	file = find_file_by_fd(fd);
	if (file == NULL)
	{
		return -1;
	}
	lock_acquire(&filesys_lock);
	int length = file_length(file);
	lock_release(&filesys_lock);
	return length;
}

static int sys_read(int fd, void *buffer, unsigned size)
{
	struct file *file;
	validate_user_buffer(buffer, size);
	if (size == 0)
		return 0;
	if (fd == 1)
		return -1;
	if (fd < 0)
		return -1;
	if (fd >= ARG_MAX)
		return -1;

	if (fd == 0) // 표준입력
	{
		for (unsigned i = 0; i < size; i++)
		{
			//키보드 입력을 읽어서 유저 버퍼에 복사함
			((uint8_t *)buffer)[i] = input_getc(); 
		}
		return size;
	}

	if (fd >= 2)
	{
		file = find_file_by_fd(fd);
		if (file == NULL)
			return -1;
		lock_acquire(&filesys_lock);
		//파일에서 size만큼 읽어서 유저 버퍼에 복사
		int bytes = file_read(file, buffer, size);
		lock_release(&filesys_lock);
		return bytes;
	}
	return -1;
}

static int
sys_open(const char *file_name)
{
	if (!is_valid_user_ptr(file_name))
	{
		sys_exit(-1);
	}

	validate_user_string(file_name);

	if (file_name_is_empty(file_name))
	{
		return -1;
	}

	// sys_open이 filesys_open(file_name)을 호출
	// filesys_open이 디렉터리에서 파일 이름을 찾고 inode를 얻음
	// filesys_open 내부에서 file_open(inode) 호출
	// file_open이 struct file * 객체를 만들어 반환
	lock_acquire(&filesys_lock);
	struct file *file = filesys_open(file_name);
	lock_release(&filesys_lock);

	// open-missing 테스트
	if (file == NULL)
	{
		return -1;
	}

	int fd = fd_alloc(file);
	if (fd == -1)
	{
		lock_acquire(&filesys_lock);
		file_close(file);
		lock_release(&filesys_lock);
	}

	return fd;
}

static void sys_close(int fd){
	if (fd < 2) {
		return;
	}
	if (fd >= ARG_MAX){
		return;
	}

	struct thread * curr = thread_current();
	struct file * file;
	file = find_file_by_fd(fd);

	if (file == NULL) {
		return;
	}
	
	lock_acquire(&filesys_lock);
	file_close(file);
	lock_release(&filesys_lock);
	curr->fd_table[fd]=NULL;
}

void sys_exit(int status)
{
	struct thread *curr = thread_current();
	curr->exit_status = status;
	printf("%s: exit(%d)\n", curr->name, status);
	thread_exit();
}

static bool
sys_create(const char *file, unsigned initial_size)
{
	if (!is_valid_user_ptr(file))
	{
		sys_exit(-1);
	}

	validate_user_string(file);

	if (file_name_is_empty(file))
	{
		return false;
	}

	if (file_name_is_too_long(file))
	{
		return false;
	}

	lock_acquire(&filesys_lock);
	bool is_file_created = filesys_create(file, initial_size);
	lock_release(&filesys_lock);

	if (!is_file_created)
	{
		return false;
	}

	return true;
}

static bool
sys_remove(const char *file)
{
	if (!is_valid_user_ptr(file))
		sys_exit(-1);

	validate_user_string(file);

	if (file_name_is_empty(file) || file_name_is_too_long(file))
		return false;

	lock_acquire(&filesys_lock);
	bool removed = filesys_remove(file);
	lock_release(&filesys_lock);
	return removed;
}

static void
sys_seek(int fd, unsigned position)
{
	struct file *file = find_file_by_fd(fd);
	if (file == NULL)
		return;

	lock_acquire(&filesys_lock);
	file_seek(file, position);
	lock_release(&filesys_lock);
}

static unsigned
sys_tell(int fd)
{
	struct file *file = find_file_by_fd(fd);
	if (file == NULL)
		return (unsigned) -1;

	lock_acquire(&filesys_lock);
	unsigned position = file_tell(file);
	lock_release(&filesys_lock);
	return position;
}

static int
sys_exec(const char *cmd_line)
{
	validate_user_string(cmd_line);

	char *copied_cmd = palloc_get_page(0);
	if (copied_cmd == NULL)
		return -1;

	strlcpy(copied_cmd, cmd_line, PGSIZE);
	return process_exec(copied_cmd);
}

static void
sys_halt(void)
{
	power_off();
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f UNUSED)
{
	// 10번이 SYS_WRITE
	int sys_call = f->R.rax;

	switch (sys_call)
	{
	case SYS_WRITE:
		f->R.rax = sys_write((int) f->R.rdi, (const void *) f->R.rsi, (unsigned) f->R.rdx);
		break;
	case SYS_OPEN:
		f->R.rax = sys_open((const char *) f->R.rdi);
		break;
	case SYS_EXIT:
		sys_exit(f->R.rdi);
		break;
	case SYS_CREATE:
		f->R.rax = sys_create((const char *) f->R.rdi, (unsigned) f->R.rsi);
		break;
	case SYS_HALT:
		sys_halt();
		break;
	case SYS_READ:
		f->R.rax = sys_read((int) f->R.rdi, (void *) f->R.rsi, (unsigned) f->R.rdx);
		break;
	case SYS_FILESIZE:
		f->R.rax = sys_filesize((int) f->R.rdi);
		break;
	case SYS_CLOSE:
		sys_close((int) f->R.rdi);
		break;
	case SYS_REMOVE:
		f->R.rax = sys_remove((const char *) f->R.rdi);
		break;
	case SYS_SEEK:
		sys_seek((int) f->R.rdi, (unsigned) f->R.rsi);
		break;
	case SYS_TELL:
		f->R.rax = sys_tell((int) f->R.rdi);
		break;
	case SYS_FORK:
		validate_user_string((const char *) f->R.rdi);
		f->R.rax = process_fork((const char *) f->R.rdi, f);
		break;
	case SYS_WAIT:
		f->R.rax = process_wait((tid_t) f->R.rdi);
		break;
	case SYS_EXEC:
		f->R.rax = sys_exec((const char *) f->R.rdi);
		break;
	default:
		sys_exit(-1);
		break;
	}
}
