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
// 추가된 헤더파일
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"

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
void sys_exit(int status);


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

// 유저 메모리 유효성 검사 함수들
// 실패 종료 경로 함수
static void
fail_invalid_user_memory(void) {
	// 잘못된 사용자 메모리는 현재 프로세스를 exit(-1)로 종료한다.
	// validate_*() 계열 helper의 공통 실패 경로로 사용한다.
	// 호출 이후 정상 syscall 반환값을 만들지 않는다.
	sys_exit(-1);
}

// 접근 가능한 사용자 주소인지 판별하는 함수
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

//단일 포인터 검증 함수
static void
validate_user_ptr(const void *uaddr) {

	// 규칙 1: 내부 판별은 is_valid_user_ptr()에 위임한다.
	if (!is_valid_user_ptr(uaddr)) {
		// 실패 시 fail_invalid_user_memory()를 호출한다.
		fail_invalid_user_memory();
	}
}

static void
validate_user_buffer(const void *buffer, size_t size) {
	// size == 0은 빈 범위로 처리한다.
	if (size == 0) {
		return;
	}
	// 시작 주소를 검증한다.
	validate_user_ptr(buffer);
	// buffer가 가리키는 메모리 범위의 마지막 바이트도 유효한 사용자 주소인지 확인한다
	validate_user_ptr((const uint8_t *) buffer + size - 1);

	for (const uint8_t *page = pg_round_down(buffer);
		 page <= (const uint8_t *) pg_round_down((const uint8_t *) buffer + size - 1);
		 page += PGSIZE) {
		validate_user_ptr(page);
	}
}

static void
validate_user_string(const char *str) {
	// 규칙 1: 시작 주소뿐 아니라 각 문자 위치를 검증한다.
	validate_user_ptr(str);

	// 규칙 2: NUL 종료를 발견하면 검증을 종료한다.
	while (true) {
		validate_user_ptr(str);
		if (*str == '\0') {
			return;
		}
		str++;
	}
}

// 시스템 콜 함수들

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
	// TODO: Your implementation goes here.

	// 10번이 SYS_WRITE
	int sys_call = f->R.rax;

	switch (sys_call) {
		case SYS_WRITE:
			f->R.rax = sys_write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_EXIT:
			sys_exit(f->R.rdi);
	}

}