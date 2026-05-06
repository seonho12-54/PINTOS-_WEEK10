#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/tss.h"
#include "userprog/process.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/mmu.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef VM
#include "vm/vm.h"
#endif


static void process_cleanup (void);
static void close_running_file (struct thread *t);
static bool load (const char *file_name, struct intr_frame *if_);
static void initd (void *f_name);
static void __do_fork (void *);

struct fork_args {
	struct thread *parent;
	struct child_status *child_status;
	struct intr_frame parent_if;
};

struct initd_args {
	char *file_name;
	struct child_status *child_status;
};



static void
child_status_release(struct child_status *cs)
{
	bool should_free = false;

	lock_acquire(&cs->ref_lock);
	cs->ref_count--;
	if (cs->ref_count == 0)
		should_free = true;
	lock_release(&cs->ref_lock);

	if (should_free)
		palloc_free_page(cs);
}

static struct child_status *
child_status_create(void)
{
	struct child_status *cs = palloc_get_page(0);
	if (cs == NULL)
		return NULL;

	cs->tid = TID_ERROR;
	cs->exit_status = -1;
	cs->waited = false;
	cs->exited = false;
	cs->fork_success = false;
	sema_init(&cs->fork_sema, 0);
	sema_init(&cs->wait_sema, 0);
	cs->ref_count = 2;
	lock_init(&cs->ref_lock);
	return cs;
}

static bool
duplicate_fd_table(struct thread *parent, struct thread *child)
{
	child->next_fd = parent->next_fd;
	child->capacity = parent->capacity;
	child->fd_table = NULL;

	if (parent->fd_table == NULL) {
		return true;
	}

	child->fd_table = calloc(child->capacity, sizeof(struct file *));
	if (child->fd_table == NULL) {
		child->capacity = 0;
		return false;
	}

	for (int fd = 2; fd < parent->capacity; fd++) {
		struct file *parent_file = parent->fd_table[fd];

		if (parent_file == NULL)
			continue;

		child->fd_table[fd] = file_duplicate(parent_file);

		if (child->fd_table[fd] == NULL) {
			for (int used_fd = 2; used_fd < fd; used_fd++) {
				if (child->fd_table[used_fd] != NULL) {
					file_close(child->fd_table[used_fd]);
					child->fd_table[used_fd] = NULL;
				}
			}

			free(child->fd_table);
			child->fd_table = NULL;
			child->capacity = 0;
			return false;
		}
	}

	return true;
}


static bool
duplicate_running_file(struct thread *parent, struct thread *child)
{
	if (parent->running_file == NULL)
		return true;

	child->running_file = file_duplicate(parent->running_file);
	return child->running_file != NULL;
}

static void
close_running_file(struct thread *t)
{
	if (t->running_file == NULL)
		return;

	file_close(t->running_file);
	t->running_file = NULL;
}


/* General process initializer for initd and other process. */
static void
process_init (void) {
	struct thread *current = thread_current ();
}




bool parse_command_line_args(char *cmd_line, int *argc, char **argv){
	// strtok_r() 첫 호출로 시작 토큰을 가져온다.
	char *save_ptr = NULL;
	char *token = strtok_r(cmd_line, " ", &save_ptr);
	*argc = 0;
	if (token == NULL){
		return false;
	}
	// 
 	if(*argc >= ARG_MAX){
		return false;
	}
	// 토큰을 argv[argc]에 저장한 뒤 argc를 증가시킨다.
	argv[*argc] = token;
	(*argc)++;

	// 매 반복마다 최대 토큰 수/길이 경계를 검사한다.
	while(token != NULL){
		token = strtok_r(NULL, " ", &save_ptr);
		if (token == NULL){
			break;
		}
		if(*argc >= ARG_MAX){
			return false;
		}
		argv[*argc] = token;
		(*argc)++;
	}

	return true;
}

bool finalize_parsed_args(int argc, char **argv){
	// 첫 토큰(실행 파일명) 존재 여부를 최종 확인한다.
	if(argv == NULL){
		return false;
	}
	
	if(argc <= 0 || argv[0] == NULL){
		return false;
	}

	return true;
}


static bool build_user_stack_args(struct intr_frame *user_if, int argc, char **argv, uintptr_t *argv_user_addr){
	uintptr_t sp;
	char **arg_addrs;
	int i;
	// 입력 검증
	if (argc <= 0 || argc > ARG_MAX || argv == NULL || user_if == NULL || argv_user_addr == NULL)
		return false;

	arg_addrs = palloc_get_page (0);
	if (arg_addrs == NULL)
		return false;

	// 문자열을 역순으로 push하고 시작 주소를 arg_addrs[]에 기록한다.
	sp = (uintptr_t) user_if->rsp;
	
	for(i = argc - 1; i >= 0; i--){
		size_t len = strlen(argv[i]) + 1;
		sp -= len;

		if(sp < USER_STACK - PGSIZE){
			palloc_free_page (arg_addrs);
			return false;
		}

		memcpy((void *) sp, argv[i], len);
		arg_addrs[i] = (char *) sp;
	}
	// 8바이트 정렬을 맞춘 뒤 NULL 센티널을 push한다.
	sp &= ~0x7;

	if(sp < USER_STACK - PGSIZE){
		palloc_free_page (arg_addrs);
		return false;
	}
	
	sp -= sizeof(char *);
	
	if(sp < USER_STACK - PGSIZE){
		palloc_free_page (arg_addrs);
		return false;
	}

	*(char **) sp = NULL;
	
	// arg_addrs[]를 역순 순회해 argv 포인터들을 push한다.
	for(i = argc - 1; i >= 0; i--){
		sp -= sizeof(char *);

		if(sp < USER_STACK - PGSIZE){
			palloc_free_page (arg_addrs);
			return false;
		}

		*(char **) sp = arg_addrs[i];
	}
	
	// 마지막에 fake return address(0)를 push해 프레임을 마무리한다.
	*argv_user_addr = sp;

	sp -= sizeof(char *);

	if(sp < USER_STACK - PGSIZE){
		palloc_free_page (arg_addrs);
		return false;
	}

	*(void **) sp = NULL;

	user_if->rsp = sp;
	palloc_free_page (arg_addrs);
	return true;
}

bool set_user_entry_registers(struct intr_frame *user_if, int argc, uintptr_t argv_user_addr){
	// 입력 검증하기
	if(user_if == NULL || argc <= 0 || argv_user_addr == 0)
		return false;
	//argv_user_addr가 사용자 가상 주소인지 점검한다.
	if(!is_user_vaddr((void *) argv_user_addr)){
		return false;
	}
	// if_.R.rdi = argc를 먼저 설정한다.
	user_if->R.rdi = argc;
	// if_.R.rsi = argv_user_addr를 설정한다.
	user_if->R.rsi = argv_user_addr;
	return true;
}

bool
validate_user_entry_frame (struct intr_frame *user_if)
{
	if (user_if == NULL)
		return false;

	if (!is_user_vaddr ((void *) user_if->rsp))
		return false;

	/* `set_user_entry_registers` 계약: RDI = argc (정수), RSI = argv (사용자 주소). */
	if (user_if->R.rdi == 0 || user_if->R.rdi > ARG_MAX)
		return false;

	if (!is_user_vaddr ((void *) user_if->R.rsi))
		return false;
	if ((user_if->R.rsi % sizeof (void *)) != 0)
		return false;
	if (pml4_get_page (thread_current ()->pml4, (void *) user_if->R.rsi) == NULL)
		return false;

	return true;
}

/* Starts the first userland program, called "initd", loaded from FILE_NAME.
 * The new thread may be scheduled (and may even exit)
 * before process_create_initd() returns. Returns the initd's
 * thread id, or TID_ERROR if the thread cannot be created.
 * Notice that THIS SHOULD BE CALLED ONCE. */
tid_t
process_create_initd (const char *file_name) {
	char *fn_copy; // file_name 문자열을 복사해서 담아두는 별도의 메모리 페이지 
	tid_t tid; // 고유한 쓰레드 ID 
	struct child_status *cs;
	struct initd_args *args;
	
	/* Make a copy of FILE_NAME.
	 * Otherwise there's a race between the caller and load(). */
	fn_copy = palloc_get_page (0);
	if (fn_copy == NULL) {
		return TID_ERROR;
	}
	strlcpy (fn_copy, file_name, PGSIZE);

    // char *token = strtok_r(file_name, " ", &save_ptr);
	/* Create a new thread to execute FILE_NAME. */
	char *prog_copy; 
	prog_copy = palloc_get_page (0); 

	if (prog_copy == NULL) {
		palloc_free_page (fn_copy);
		return TID_ERROR; 
	}
	
	strlcpy(prog_copy, file_name, PGSIZE); 
	prog_copy = strtok_r(prog_copy, " ", &file_name); 

	cs = child_status_create();
	if (cs == NULL) {
		palloc_free_page (fn_copy);
		palloc_free_page (prog_copy);
		return TID_ERROR;
	}

	args = palloc_get_page (0);
	if (args == NULL) {
		palloc_free_page (fn_copy);
		palloc_free_page (prog_copy);
		palloc_free_page (cs);
		return TID_ERROR;
	}

	args->file_name = fn_copy;
	args->child_status = cs;
	list_push_back(&thread_current()->children, &cs->elem);

	tid = thread_create (prog_copy, PRI_DEFAULT, initd, args);
	palloc_free_page(prog_copy); 

	if (tid == TID_ERROR) {
		list_remove(&cs->elem);
		palloc_free_page (fn_copy);
		palloc_free_page (args);
		palloc_free_page (cs);
		return TID_ERROR;
	}

	cs->tid = tid;
	return tid;
}

/* A thread function that launches first user process. */
static void
initd (void *f_name) {
	struct initd_args *args = f_name;
	char *file_name = args->file_name;
	thread_current()->my_status = args->child_status;
	palloc_free_page(args);

#ifdef VM
	supplemental_page_table_init (&thread_current ()->spt);
#endif

	process_init ();
	
	if (process_exec (file_name) < 0)
		PANIC("Fail to launch initd\n");
	NOT_REACHED ();
}

/* Clones the current process as `name`. Returns the new process's thread id, or
 * TID_ERROR if the thread cannot be created. */
tid_t
process_fork (const char *name, struct intr_frame *if_ UNUSED) {
	struct thread *parent = thread_current();
	struct child_status *cs = child_status_create();
	if (cs == NULL)
		return TID_ERROR;

	struct fork_args *args = palloc_get_page(0);
	if (args == NULL)
	{
		palloc_free_page(cs);
		return TID_ERROR;
	}

	args->parent = parent;
	args->child_status = cs;
	memcpy(&args->parent_if, if_, sizeof *if_);

	list_push_back(&parent->children, &cs->elem);

	tid_t tid = thread_create(name, PRI_DEFAULT, __do_fork, args);
	if (tid == TID_ERROR)
	{
		list_remove(&cs->elem);
		palloc_free_page(args);
		palloc_free_page(cs);
		return TID_ERROR;
	}

	cs->tid = tid;
	sema_down(&cs->fork_sema);

	if (!cs->fork_success)
	{
		list_remove(&cs->elem);
		child_status_release(cs);
		return TID_ERROR;
	}

	return tid;
}

#ifndef VM
/* Duplicate the parent's address space by passing this function to the
 * pml4_for_each. This is only for the project 2. */
static bool
duplicate_pte (uint64_t *pte, void *va, void *aux) {
	struct thread *current = thread_current ();
	struct thread *parent = (struct thread *) aux;
	void *parent_page;
	void *newpage;
	bool writable;

	if (is_kernel_vaddr(va))
		return true;

	parent_page = pml4_get_page (parent->pml4, va);
	if (parent_page == NULL)
		return false;

	newpage = palloc_get_page(PAL_USER);
	if (newpage == NULL)
		return false;

	memcpy(newpage, parent_page, PGSIZE);
	writable = is_writable(pte);

	if (!pml4_set_page (current->pml4, va, newpage, writable)) {
		palloc_free_page(newpage);
		return false;
	}
	return true;
}
#endif

/* A thread function that copies parent's execution context.
 * Hint) parent->tf does not hold the userland context of the process.
 *       That is, you are required to pass second argument of process_fork to
 *       this function. */
static void
__do_fork (void *aux) {
	struct intr_frame if_;
	struct fork_args *args = aux;
	struct thread *parent = args->parent;
	struct child_status *cs = args->child_status;
	struct thread *current = thread_current ();

	memcpy (&if_, &args->parent_if, sizeof if_);
	if_.R.rax = 0;
	current->my_status = cs;

	/* 2. Duplicate PT */
	current->pml4 = pml4_create();
	if (current->pml4 == NULL)
		goto error;

	process_activate (current);
#ifdef VM
	supplemental_page_table_init (&current->spt);
	if (!supplemental_page_table_copy (&current->spt, &parent->spt))
		goto error;
#else
	if (!pml4_for_each (parent->pml4, duplicate_pte, parent))
		goto error;
#endif

	if (!duplicate_fd_table(parent, current))
		goto error;
	if (!duplicate_running_file(parent, current))
		goto error;

	process_init ();
	cs->fork_success = true;
	sema_up(&cs->fork_sema);
	palloc_free_page(args);

	/* Finally, switch to the newly created process. */
	do_iret (&if_);
error:
	cs->fork_success = false;
	sema_up(&cs->fork_sema);
	palloc_free_page(args);
	thread_exit ();
}


/* Switch the current execution context to the f_name.
 * Returns -1 on fail. */
int
process_exec (void *f_name) {
	// 커맨드 라인 받기
	char *file_name = f_name;
	bool success;
	
	// file_name NULL 여부를 먼저 검사한다.
	if (file_name == NULL){
		return -1;
	}
	
	// 길이가 너무 길지 않은지 확인한다. 
	if(strnlen(file_name, PGSIZE) == PGSIZE){
		palloc_free_page (file_name);
		return -1;
	}

	// 공백만 있는 입력인지 확인한다.
	if(strspn(file_name, " ") == strlen(file_name)){
		palloc_free_page (file_name);
		return -1;
	}

	// 기능 1: 커맨드라인 파싱 (토큰화) 
	// process_exec() 파싱 시작부

	// 쓰기 가능한 복사 버퍼를 할당한다.
	char *cmd_line = palloc_get_page (0);
	if (cmd_line == NULL){
		palloc_free_page (file_name);
		return -1;
	}

	// 복사 버퍼에 복사한다.
	strlcpy(cmd_line, file_name, PGSIZE);

	// parse_command_line_args() 토큰화 루프
	int argc = 0;
	char **argv = palloc_get_page (0);
	if (argv == NULL){
		palloc_free_page (file_name);
		palloc_free_page (cmd_line);
		return -1;
	}

	success = parse_command_line_args(cmd_line, &argc, argv);
	if (!success){ 
		palloc_free_page (file_name);
		palloc_free_page (cmd_line);
		palloc_free_page (argv);
		return -1;
	}
	
	// finalize_parsed_args() 파싱 결과 반환
	success = finalize_parsed_args(argc, argv);
	if (!success){
		palloc_free_page (file_name);
		palloc_free_page (cmd_line);
		palloc_free_page (argv);
		return -1;
	}

	/* We cannot use the intr_frame in the thread structure.
	 * This is because when current thread rescheduled,
	 * it stores the execution information to the member. */
	struct intr_frame _if;
	_if.ds = _if.es = _if.ss = SEL_UDSEG;
	_if.cs = SEL_UCSEG;
	_if.eflags = FLAG_IF | FLAG_MBS;

	/* We first kill the current context */
	close_running_file(thread_current());
	process_cleanup ();
	
	// TODO: Argument 분리해서 파일명만 load()로 넘기기 

	// 기능 2: 사용자 스택 레이아웃 구성 부분 시작 (ABI 계약)
	uintptr_t argv_user_addr = 0;
	// load() 기본 매핑
	success = load(argv[0], &_if);
	if (!success){
		palloc_free_page (file_name);
		palloc_free_page (cmd_line);
		palloc_free_page (argv);
		return -1;
	}
	else{
		// build_user_stack_args() 인자 배치 루프
		success = build_user_stack_args(&_if, argc, argv, &argv_user_addr);
		if (!success){
			palloc_free_page (file_name);
			palloc_free_page (cmd_line);
			palloc_free_page (argv);
			return -1;
		}
	}

	// 기능 3: 레지스터 설정과 유저 진입 경계
	// set_user_entry_registers() intr_frame 인자 필드
	success = set_user_entry_registers(&_if, argc, argv_user_addr);
	if (!success){
		palloc_free_page (file_name);
		palloc_free_page (cmd_line);
		palloc_free_page (argv);
		return -1;
	}
	// validate_user_entry_frame() 유저 전환 직전 점검
	success = validate_user_entry_frame(&_if);
	if (!success){
		palloc_free_page (file_name);
		palloc_free_page (cmd_line);
		palloc_free_page (argv);
		return -1;
	}
	// do_iret() 유저 진입
	palloc_free_page (file_name);
	palloc_free_page (cmd_line);
	palloc_free_page (argv);
	do_iret (&_if);
	NOT_REACHED ();
}


/* Waits for thread TID to die and returns its exit status.  If
 * it was terminated by the kernel (i.e. killed due to an
 * exception), returns -1.  If TID is invalid or if it was not a
 * child of the calling process, or if process_wait() has already
 * been successfully called for the given TID, returns -1
 * immediately, without waiting.
 *
 * This function will be implemented in problem 2-2.  For now, it
 * does nothing. */
int
process_wait (tid_t child_tid UNUSED) {
	struct thread *curr = thread_current();
	struct child_status *target = NULL;

	// 현재 프로세스가 가지고 있는 자식 목록 curr->children을 처음부터 끝까지 순회한다 
	for (struct list_elem *e = list_begin(&curr->children);
		 e != list_end(&curr->children);
		 e = list_next(e))
	{
		// 리스트 원소 e를 child_status 구조체 포인터로 변환한다 (list_elem)
		struct child_status *cs = list_entry(e, struct child_status, elem);

		// 현재 확인 중인 자식의 tid가 기다리려는 child_tid와 같은지 확인한다 
		if (cs->tid == child_tid)
		{
			target = cs;
			break;
		}
	}

	// target이 NULL이면 child_tid가 현재 프로세스의 자식이 아니라는 의미이다 
	// target->waited가 true이면 이미 wait한 자식이라는 의미이다 
	// 두 경우 모두 wait 실패이므로 -1을 반환한다 
	if (target == NULL || target->waited)
		return -1;

	// 이 자식 프로세스에 대해 wait을 호출했음을 표시한다
	// 같은 자식에 대해 중복 wait하는 것을 막기 위해 사용한다 
	target->waited = true;

	// 자식 프로세스가 아직 종료되지 않았다면
	// 부모 프로세스는 자식이 종료될 때까지 wait_sema에서 잠든다 
	if (!target->exited)
		sema_down(&target->wait_sema);

	// 자식 프로세스가 종료되면서 저장해 둔 exit status를 가져온다 
	int status = target->exit_status;

	// 현재 부모 프로세스의 children 리스트에서 해당 자식 정보를 제거한다
	// wait이 끝난 자식이 더 이상 부모의 자식 관리 목록에 남겨 둘 필요가 없다 
	list_remove(&target->elem);
	
	// 부모가 child_status를 더 이상 사용하지 않으므로 참조를 해제한다 
	// child_status_release 내부에서 참조 카운트가 0이 되면 메모리를 free할 수 있다 
	child_status_release(target);

	// 자식 프로세스의 종료 상태를 반환한다
	return status;
}

/* Exit the process. This function is called by thread_exit (). */
void
process_exit (void) {
	struct thread *curr = thread_current ();
	close_running_file(curr);

	if (curr->my_status != NULL)
	{
		curr->my_status->exit_status = curr->exit_status;
		curr->my_status->exited = true;
		sema_up(&curr->my_status->wait_sema);
		child_status_release(curr->my_status);
		curr->my_status = NULL;
	}

	while (!list_empty(&curr->children))
	{
		struct list_elem *e = list_pop_front(&curr->children);
		struct child_status *cs = list_entry(e, struct child_status, elem);
		child_status_release(cs);
	}

	if (curr->fd_table != NULL) {
		for (int fd = 2; fd < thread_current()->capacity; fd++)
		{
			if (curr->fd_table[fd] != NULL)
			{
				file_close(curr->fd_table[fd]);
				curr->fd_table[fd] = NULL;
			}
		}

		free(curr->fd_table); 
		curr->fd_table = NULL; 
		curr->capacity = 16;
	}

	process_cleanup ();
}

/* Free the current process's resources. */
static void
process_cleanup (void) {
	struct thread *curr = thread_current ();

#ifdef VM
	supplemental_page_table_kill (&curr->spt);
#endif

	uint64_t *pml4;
	/* Destroy the current process's page directory and switch back
	 * to the kernel-only page directory. */
	pml4 = curr->pml4;
	if (pml4 != NULL) {
		/* Correct ordering here is crucial.  We must set
		 * cur->pagedir to NULL before switching page directories,
		 * so that a timer interrupt can't switch back to the
		 * process page directory.  We must activate the base page
		 * directory before destroying the process's page
		 * directory, or our active page directory will be one
		 * that's been freed (and cleared). */
		curr->pml4 = NULL;
		pml4_activate (NULL);
		pml4_destroy (pml4);
	}
}

/* Sets up the CPU for running user code in the nest thread.
 * This function is called on every context switch. */
void
process_activate (struct thread *next) {
	/* Activate thread's page tables. */
	pml4_activate (next->pml4);

	/* Set thread's kernel stack for use in processing interrupts. */
	tss_update (next);
}

/* We load ELF binaries.  The following definitions are taken
 * from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
#define EI_NIDENT 16

#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
 * This appears at the very beginning of an ELF binary. */
struct ELF64_hdr {
	unsigned char e_ident[EI_NIDENT];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct ELF64_PHDR {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};
/* Abbreviations */
#define ELF ELF64_hdr
#define Phdr ELF64_PHDR


static bool setup_stack (struct intr_frame *if_);
static bool validate_segment (const struct Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes,
		bool writable);


/* Loads an ELF executable from FILE_NAME into the current thread.
 * Stores the executable's entry point into *RIP
 * and its initial stack pointer into *RSP.
 * Returns true if successful, false otherwise. */
static bool
load (const char *file_name, struct intr_frame *if_) {
	struct thread *t = thread_current ();
	struct ELF ehdr;
	struct file *file = NULL;
	off_t file_ofs;
	bool success = false;
	int i;

	/* Allocate and activate page directory. */
	t->pml4 = pml4_create ();
	if (t->pml4 == NULL)
		goto done;
	process_activate (thread_current ());

	/* Open executable file. */	
	file = filesys_open (file_name);
	if (file == NULL) {
		printf ("load: %s: open failed\n", file_name);
		goto done;
	}
	file_deny_write(file);

	/* Read and verify executable header. */
	if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
			|| memcmp (ehdr.e_ident, "\177ELF\2\1\1", 7)
			|| ehdr.e_type != 2
			|| ehdr.e_machine != 0x3E // amd64
			|| ehdr.e_version != 1
			|| ehdr.e_phentsize != sizeof (struct Phdr)
			|| ehdr.e_phnum > 1024) {
		printf ("load: %s: error loading executable\n", file_name);
		goto done;
	}

	/* Read program headers. */
	file_ofs = ehdr.e_phoff;
	for (i = 0; i < ehdr.e_phnum; i++) {
		struct Phdr phdr;

		if (file_ofs < 0 || file_ofs > file_length (file))
			goto done;
		file_seek (file, file_ofs);

		if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
			goto done;
		file_ofs += sizeof phdr;
		switch (phdr.p_type) {
			case PT_NULL:
			case PT_NOTE:
			case PT_PHDR:
			case PT_STACK:
			default:
				/* Ignore this segment. */
				break;
			case PT_DYNAMIC:
			case PT_INTERP:
			case PT_SHLIB:
				goto done;
			case PT_LOAD:
				if (validate_segment (&phdr, file)) {
					bool writable = (phdr.p_flags & PF_W) != 0;
					uint64_t file_page = phdr.p_offset & ~PGMASK;
					uint64_t mem_page = phdr.p_vaddr & ~PGMASK;
					uint64_t page_offset = phdr.p_vaddr & PGMASK;
					uint32_t read_bytes, zero_bytes;
					if (phdr.p_filesz > 0) {
						/* Normal segment.
						 * Read initial part from disk and zero the rest. */
						read_bytes = page_offset + phdr.p_filesz;
						zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
								- read_bytes);
					} else {
						/* Entirely zero.
						 * Don't read anything from disk. */
						read_bytes = 0;
						zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
					}
					if (!load_segment (file, file_page, (void *) mem_page,
								read_bytes, zero_bytes, writable))
						goto done;
				}
				else
					goto done;
				break;
		}
	}

	
	/* Set up stack. */
	if (!setup_stack (if_))
		goto done;

	/* Start address. */
	if_->rip = ehdr.e_entry;

	/* TODO: Your code goes here.
	 * TODO: Implement argument passing (see project2/argument_passing.html). */
	
	success = true;

done:
	if (success)
		thread_current()->running_file = file;
	else if (file != NULL)
		file_close(file);
	return success;
}


/* Checks whether PHDR describes a valid, loadable segment in
 * FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Phdr *phdr, struct file *file) {
	/* p_offset and p_vaddr must have the same page offset. */
	if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK))
		return false;

	/* p_offset must point within FILE. */
	if (phdr->p_offset > (uint64_t) file_length (file))
		return false;

	/* p_memsz must be at least as big as p_filesz. */
	if (phdr->p_memsz < phdr->p_filesz)
		return false;

	/* The segment must not be empty. */
	if (phdr->p_memsz == 0)
		return false;

	/* The virtual memory region must both start and end within the
	   user address space range. */
	if (!is_user_vaddr ((void *) phdr->p_vaddr))
		return false;
	if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
		return false;

	/* The region cannot "wrap around" across the kernel virtual
	   address space. */
	if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
		return false;

	/* Disallow mapping page 0.
	   Not only is it a bad idea to map page 0, but if we allowed
	   it then user code that passed a null pointer to system calls
	   could quite likely panic the kernel by way of null pointer
	   assertions in memcpy(), etc. */
	if (phdr->p_vaddr < PGSIZE)
		return false;

	/* It's okay. */
	return true;
}

#ifndef VM
/* Codes of this block will be ONLY USED DURING project 2.
 * If you want to implement the function for whole project 2, implement it
 * outside of #ifndef macro. */

/* load() helpers. */
static bool install_page (void *upage, void *kpage, bool writable);

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	file_seek (file, ofs);
	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* Get a page of memory. */
		uint8_t *kpage = palloc_get_page (PAL_USER);
		if (kpage == NULL)
			return false;

		/* Load this page. */
		if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes) {
			palloc_free_page (kpage);
			return false;
		}
		memset (kpage + page_read_bytes, 0, page_zero_bytes);

		/* Add the page to the process's address space. */
		if (!install_page (upage, kpage, writable)) {
			printf("fail\n");
			palloc_free_page (kpage);
			return false;
		}

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a minimal stack by mapping a zeroed page at the USER_STACK */
static bool
setup_stack (struct intr_frame *if_) {
	uint8_t *kpage;
	bool success = false;

	kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	if (kpage != NULL) {
		success = install_page (((uint8_t *) USER_STACK) - PGSIZE, kpage, true);
		if (success)
			if_->rsp = USER_STACK;
		else
			palloc_free_page (kpage);
	}
	return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
 * virtual address KPAGE to the page table.
 * If WRITABLE is true, the user process may modify the page;
 * otherwise, it is read-only.
 * UPAGE must not already be mapped.
 * KPAGE should probably be a page obtained from the user pool
 * with palloc_get_page().
 * Returns true on success, false if UPAGE is already mapped or
 * if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable) {
	struct thread *t = thread_current ();

	/* Verify that there's not already a page at that virtual
	 * address, then map our page there. */
	return (pml4_get_page (t->pml4, upage) == NULL
			&& pml4_set_page (t->pml4, upage, kpage, writable));
}
#else
/* From here, codes will be used after project 3.
 * If you want to implement the function for only project 2, implement it on the
 * upper block. */

static bool
lazy_load_segment (struct page *page, void *aux) {
	/* TODO: Load the segment from the file */
	/* TODO: This called when the first page fault occurs on address VA. */
	/* TODO: VA is available when calling this function. */
}

/* Loads a segment starting at offset OFS in FILE at address
 * UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
 * memory are initialized, as follows:
 *
 * - READ_BYTES bytes at UPAGE must be read from FILE
 * starting at offset OFS.
 *
 * - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.
 *
 * The pages initialized by this function must be writable by the
 * user process if WRITABLE is true, read-only otherwise.
 *
 * Return true if successful, false if a memory allocation error
 * or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
		uint32_t read_bytes, uint32_t zero_bytes, bool writable) {
	ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
	ASSERT (pg_ofs (upage) == 0);
	ASSERT (ofs % PGSIZE == 0);

	while (read_bytes > 0 || zero_bytes > 0) {
		/* Do calculate how to fill this page.
		 * We will read PAGE_READ_BYTES bytes from FILE
		 * and zero the final PAGE_ZERO_BYTES bytes. */
		size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;

		/* TODO: Set up aux to pass information to the lazy_load_segment. */
		void *aux = NULL;
		if (!vm_alloc_page_with_initializer (VM_ANON, upage,
					writable, lazy_load_segment, aux))
			return false;

		/* Advance. */
		read_bytes -= page_read_bytes;
		zero_bytes -= page_zero_bytes;
		upage += PGSIZE;
	}
	return true;
}

/* Create a PAGE of stack at the USER_STACK. Return true on success. */
static bool
setup_stack (struct intr_frame *if_) {
	bool success = false;
	void *stack_bottom = (void *) (((uint8_t *) USER_STACK) - PGSIZE);

	/* TODO: Map the stack on stack_bottom and claim the page immediately.
	 * TODO: If success, set the rsp accordingly.
	 * TODO: You should mark the page is stack. */
	/* TODO: Your code goes here */

	return success;
}
#endif /* VM */
