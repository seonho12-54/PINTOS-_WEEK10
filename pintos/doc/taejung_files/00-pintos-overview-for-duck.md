# Pintos 전체 개념 설명

이 문서는 Pintos 운영체제를 처음 접하는 러버덕에게도 이해되도록, 전체 구조와 주요 개념을 자세히 설명합니다.

---

## 1. Pintos란 무엇인가요?

Pintos는 **교육용 운영체제(OS) 프로젝트**입니다. 학생들이 운영체제의 핵심 개념을 직접 구현하면서 배우도록 설계되었습니다.

Pintos 학습에서 주로 다루는 영역은 다음과 같습니다.

1. **Threads**: 스레드 생성, 스케줄링, 문맥 전환, 동기화
2. **User Programs**: 사용자 프로세스 실행, 주소 공간 분리, 시스템 콜
3. **File Systems**: 파일 저장, 디렉터리 관리, 블록 할당
4. **Virtual Memory**: 가상 메모리, 페이지 폴트, demand paging, swap

Pintos는 **x86-64** 기반이며, 교육 목적상 단일 CPU 환경을 가정합니다. 따라서 멀티코어가 아닌 한 CPU에서 동작하고, 복잡도를 줄이기 위해 설계된 부분이 있습니다.

---

## 2. Pintos 전체 코드 구조

Pintos 소스 코드는 역할별로 폴더가 나뉩니다. 각 폴더가 무슨 일을 하는지 알고 있으면 어디를 봐야 할지 빨리 찾을 수 있습니다.

- `threads/`
  - 스케줄러, 스레드 관리, 동기화, 인터럽트 처리
  - `thread.c`, `synch.c`, `interrupt.c`, `timer.c` 등
- `userprog/`
  - 사용자 프로그램 실행, 프로세스 관리, 시스템 콜, 예외 처리
  - `process.c`, `syscall.c`, `exception.c`, `gdt.c` 등
- `filesys/`
  - 디스크 기반 파일 시스템 구현
  - `file.c`, `inode.c`, `directory.c`, `free-map.c`, `filesys.c`
- `vm/`
  - 가상 메모리와 페이지 교체, supplemental page table
  - `vm.c`, `page.c`, `swap.c`, `frame.c`, `anon.c`, `file.c`
- `lib/`
  - 기본 라이브러리 구현체, 문자열, 입출력 등
- `devices/`
  - 하드웨어 장치 드라이버 추상화, 타이머, 콘솔, 디스크
- `tests/`
  - 자동화된 테스트 케이스와 스크립트

### 2.1 폴더 간 연결

Pintos는 단순히 독립적인 모듈이 아닙니다. 예를 들어:

- 사용자 프로그램을 실행하려면 `userprog/process.c`에서 새로운 스레드를 만들고
- 그 스레드는 `threads/thread.c`에 정의된 스케줄링 규칙에 따라 실행되고
- 파일 입출력은 `filesys/file.c`를 통해 수행되며
- 메모리 매핑은 `vm/` 또는 `threads/palloc.c`에 의해 관리됩니다.

즉, Pintos를 이해하려면 각 파트가 어떻게 상호작용하는지 함께 봐야 합니다.

---

## 3. 초기 부팅과 커널 초기화

Pintos가 부팅되면 다음 순서로 시스템이 준비됩니다.

### 3.1 부트 로더와 초기 진입

1. BIOS 또는 QEMU가 Pintos 커널 이미지를 메모리에 로드
2. 초기 어셈블리 코드(`threads/start.S`)가 실행
3. 커널 스택과 페이지 테이블의 기본 환경을 설정

### 3.2 `init.c`에서의 초기화

`init.c`는 Pintos 커널의 시작점입니다.

- **메모리 초기화**: 물리 페이지 풀을 만든다
- **스레드 시스템 초기화**: 기본 스레드 데이터 구조 생성
- **인터럽트 초기화**: IDT(Interrupt Descriptor Table) 설정
- **파일 시스템 초기화**: 파일 시스템 루트와 블록 장치 초기화
- **타이머 초기화**: 시간 기반 스케줄링 준비

### 3.3 첫 사용자 프로세스 생성

부팅이 끝나면 `process_create_initd()` 또는 `start_process()`가 호출되어 첫 사용자 프로그램을 실행합니다. 이 단계에서 사용자 주소 공간을 준비하고, 사용자 모드로 진입할 준비를 마칩니다.

---

## 4. 스레드(Thread)의 핵심

Pintos의 기본 실행 단위는 `thread`입니다.

### 4.1 `struct thread` 구조

`threads/thread.h`에는 스레드의 핵심 구조가 정의되어 있습니다. 주요 필드는 다음과 같습니다.

- `tid_t tid`: 스레드 식별 번호
- `enum thread_status status`: 현재 상태 (RUNNING, READY, BLOCKED, DYING)
- `int priority`: 스레드 우선순위
- `void *stack`: 커널 스택 주소
- `uint64_t *pml4`: 프로세스 페이지 테이블 포인터
- `struct list_elem allelem`: 스레드 전체 리스트용 링크
- `struct list_elem elem`: 준비 큐 및 동기화 객체용 링크

### 4.2 스레드 상태 전환

주요 상태는 다음과 같습니다.

- `THREAD_RUNNING`: 현재 CPU를 실행 중
- `THREAD_READY`: 실행 준비 완료, 스케줄러 대기
- `THREAD_BLOCKED`: 이벤트 대기 중
- `THREAD_DYING`: 종료 예정

상태 전환 함수:

- `thread_block()`: 현재 스레드를 블록 상태로 전환
- `thread_unblock()`: 블록된 스레드를 준비 큐로 이동
- `thread_yield()`: 현재 스레드가 CPU를 자발적으로 포기

### 4.3 문맥 전환(context switch)

`thread_launch()`와 `schedule()`는 스레드 간 문맥 전환을 수행합니다.

- 현재 스레드의 CPU 레지스터와 스택 포인터를 저장
- 다음에 실행할 스레드의 레지스터와 스택 포인터를 복원
- `switch_threads()`가 실제 어셈블리 수준 문맥 전환을 수행

### 4.4 스케줄러 알고리즘

Pintos는 기본적으로 **Round-Robin** 스케줄링을 사용합니다.

- `thread_tick()`에서 시간 할당량 타이머를 체크
- 타임 슬라이스가 끝나면 `intr_yield_on_return()`으로 `thread_yield()` 호출

그 외, `priority`가 있는 경우 우선순위 기반 스케줄링도 섞어서 동작합니다.

### 4.5 동기화 primitives

`threads/synch.c`는 주요 동기화 객체를 구현합니다.

- `semaphore`: 카운트 기반 동기화
- `lock`: 상호배제를 위한 락
- `condition`: 조건 변수와 이벤트 대기

`lock_acquire()` / `lock_release()`는 스레드 간 자원 경쟁을 방지합니다.

### 4.6 우선순위 기부(priority donation)

Pintos에서는 우선순위 역전 문제를 해결하기 위해 우선순위 기부가 구현될 수 있습니다.

- 높은 우선순위 스레드가 낮은 우선순위 스레드가 가진 락을 기다릴 때
- 낮은 우선순위 스레드에게 잠시 높은 우선순위가 기부됩니다

이 메커니즘은 `thread_current()->priority`와 `donations` 리스트를 관리하는 형태로 동작합니다.

---

## 5. 프로세스와 사용자 프로그램

`userprog/`는 Pintos가 OS처럼 동작하기 위한 핵심 부분입니다.

### 5.1 프로세스란 무엇인가?

Pintos에서 프로세스는 **주소 공간을 가진 실행 단위**입니다. 스레드는 실행 흐름을 나타내고, 프로세스는 그 실행이 소비할 주소 공간과 자원을 포함합니다.

- `thread`는 실행 상태를 가진 객체
- `process`는 주소 공간, 페이지 테이블, 파일 디스크립터 등을 가진 객체

### 5.2 프로세스 생성 흐름

`process_create_initd()`는 첫 번째 사용자 프로세스를 만듭니다.

1. 커널에서 `palloc_get_page()`로 메모리를 할당
2. `thread_create()`로 새 스레드 생성
3. 새 스레드에서 `initd()` 호출
4. `process_exec()`로 실제 사용자 프로그램 로드

### 5.3 `process_exec()` 동작

`process_exec()`는 새 사용자 프로그램을 실행할 때 핵심 역할을 합니다.

- 현재 프로세스의 주소 공간 정리(`process_cleanup()`)
- 새로운 페이지 테이블 생성 및 활성화
- ELF 실행 파일 로딩
- 스택 설정 및 인자 전달
- 사용자 모드로 전환

### 5.4 ELF 실행 파일 로딩

`load()`는 ELF 파일을 읽고 실행 가능한 메모리 맵을 구성합니다.

#### ELF 헤더 검사

- `file_read()`로 ELF 헤더를 읽음
- `e_ident` 검증: `0x7F 'E' 'L' 'F'`
- `e_type`, `e_machine`, `e_version`, `e_phentsize` 등 확인

#### 프로그램 헤더 반복 처리

`e_phoff` 위치부터 `e_phnum`만큼 반복하며 각 `Phdr`를 읽습니다.

- `PT_LOAD`인 세그먼트만 로드
- `validate_segment()`에서 유효성 검사
- `load_segment()`로 실제 메모리 매핑

#### `validate_segment()`가 확인하는 항목

- 파일 오프셋과 가상 주소의 페이지 오프셋 일치
- 파일 내부 범위 여부
- `p_memsz >= p_filesz`
- 영역이 사용자 공간 범위인지
- 페이지 0을 매핑하지 않는지

#### `load_segment()`의 동작

- `palloc_get_page(PAL_USER)`로 커널 페이지 할당
- 파일에서 필요한 만큼 읽고 나머지는 0으로 채움
- `install_page(upage, kpage, writable)`로 페이지 테이블에 맵핑

### 5.5 스택 초기화

`setup_stack()`은 사용자 스택 페이지를 할당합니다.

- `USER_STACK` 바로 아래 한 페이지를 매핑
- `if_->rsp = USER_STACK`로 초기 스택 포인터 설정

이 스택은 사용자 모드 진입 직전까지 커널이 채웁니다.

### 5.6 인자 전달(Argument Passing)

`process_exec()`는 커맨드라인을 파싱하고 사용자 스택에 배치합니다.

#### 커맨드라인 파싱

- `strlcpy()`로 입력을 복사
- `strtok_r()`로 공백 단위 토큰 분리
- `argv[]` 배열에 각 인자 저장
- `argc` 계산

#### 사용자 스택 레이아웃

스택은 높은 주소에서 낮은 주소로 자랍니다. 실제 배치는 다음과 같이 구성됩니다.

```
[높은 주소]
  ---
  fake return address (0)
  argv[argc-1] pointer
  ...
  argv[0] pointer
  argv NULL terminator
  padding to 8-byte align
  "argN\0"
  ...
  "arg0\0"
[낮은 주소]
```

왜 역순으로 저장하는가?

- 스택은 `rsp`를 감소시키며 내려감
- `argv[0]`을 가장 낮은 주소에 두려면 인자를 역순으로 스택에 쌓아야 함

#### 정렬과 패딩

- x86-64 ABI는 스택을 16바이트 정렬할 것을 요구
- `sp &= ~0x7`로 8바이트 정렬 후, 필요 시 더 큰 정렬을 맞춤
- `argv` 포인터 배열과 문자열 사이에 padding을 추가

#### 레지스터 설정

- `user_if->R.rdi = argc`
- `user_if->R.rsi = argv_user_addr`
- `user_if->R.rsp = final_stack_pointer`
- `user_if->R.rip = entry_point`

이제 `do_iret()`로 사용자 모드로 전환합니다.

### 5.7 프로세스 종료와 정리

`process_exit()`와 `process_cleanup()`는 프로세스 자원을 정리합니다.

- `supplemental_page_table_kill()`로 SPT 해제
- `pml4_activate(NULL)`로 커널 페이지 테이블로 전환
- `pml4_destroy()`로 프로세스 페이지 테이블 해제

`process_wait()`는 자식 프로세스 종료를 기다리고 종료 상태를 반환합니다.

---

## 6. 시스템 콜(System Call)

시스템 콜은 사용자 프로그램이 커널 서비스를 요청하는 통로입니다.

### 6.1 시스템 콜 흐름

1. 사용자 코드에서 `syscall` 또는 라이브러리 호출
2. CPU가 트랩을 발생시키며 커널 모드로 전환
3. `intr_frame`에 사용자 레지스터 상태 저장
4. 시스템 콜 번호와 인자를 읽음
5. 적절한 핸들러로 dispatch
6. 결과를 `rax`에 저장
7. 커널에서 사용자 모드로 복귀

### 6.2 시스템 콜 번호와 dispatch

`syscall.c`는 번호별로 함수 호출을 분기합니다.

예:

```c
switch (syscall_number) {
  case SYS_HALT:
    sys_halt();
    break;
  case SYS_EXIT:
    sys_exit((int) f->R.rdi);
    break;
  case SYS_WRITE:
    f->R.rax = sys_write((int) f->R.rdi, (const void *) f->R.rsi, (unsigned) f->R.rdx);
    break;
}
```

### 6.3 인자 전달 방식

x86-64 ABI에 따라 사용자 프로그램은 다음 레지스터를 사용합니다.

- `RDI`, `RSI`, `RDX`, `R10`, `R8`, `R9`

커널은 `intr_frame`에서 레지스터 값을 읽어 인자를 얻습니다.

### 6.4 사용자 메모리 검증

사용자 포인터는 반드시 검증해야 합니다.

- `is_user_vaddr(ptr)`로 범위 확인
- `pml4_get_page()`로 페이지 매핑 확인
- 읽기/쓰기 권한 체크
- `copy_in()`, `copy_out()`으로 안전하게 메모리 이동

잘못된 포인터를 그냥 사용하면 커널이 페이지 폴트를 일으켜 치명적인 오류가 날 수 있습니다.

### 6.5 안전한 복사 함수

`copy_in()`과 `copy_out()`은 사용자 메모리를 커널 버퍼로 옮깁니다.

- 페이지 폴트가 발생하면 예외를 잡아 프로세스만 종료
- `memcpy()`를 직접 사용하지 않음

사용 예:

```c
if (!copy_in(kernel_buffer, user_buffer, size))
  sys_exit(-1);
```

### 6.6 주요 시스템 콜 예시

- `sys_halt()`: 시스템 종료
- `sys_exit(status)`: 프로세스 종료
- `sys_exec(cmd_line)`: 새 프로세스 실행
- `sys_wait(tid)`: 자식 프로세스 대기
- `sys_open()`, `sys_read()`, `sys_write()`, `sys_close()`

---

## 7. 예외와 인터럽트

Pintos는 사용자 실행 중 발생하는 여러 예외를 처리합니다.

### 7.1 인터럽트 벡터

`intr-stubs.S`와 `interrupt.c`는 인터럽트 벡터를 구성합니다.

- 각 인터럽트/예외에 대해 핸들러 등록
- `intr_register_ext()`로 외부 인터럽트 처리

### 7.2 페이지 폴트 처리

페이지 폴트는 가상 주소가 물리 페이지에 매핑되어 있지 않을 때 발생합니다.

- 사용자 페이지 폴트는 프로세스를 종료하거나 `vm`에서 페이징을 처리
- 커널 페이지 폴트는 일반적으로 치명적 오류

### 7.3 예외 처리 흐름

1. 예외 발생
2. `exception.c` 또는 `userprog/exception.c`로 분기
3. 현재 스레드의 `intr_frame` 상태 저장
4. 예외 원인 분석
5. 적절한 조치 수행 (kill, handle, retry)

---

## 8. 파일 시스템(File System)

Pintos의 파일 시스템은 디스크 블록을 관리하고 파일 데이터를 저장합니다.

### 8.1 파일 시스템 계층

- `filesys.c`: 파일 시스템 전역 초기화, 루트 디렉터리 관리
- `file.c`: 파일 읽기/쓰기, 파일 포인터 이동, 크기 관리
- `inode.c`: 파일의 메타데이터, 블록 목록 관리
- `directory.c`: 디렉터리 구조, 이름 검색, 삭제
- `free-map.c`: 빈 데이터 블록 배치 관리

### 8.2 디스크와 블록

Pintos 디스크는 섹터 단위로 동작합니다.

- 하나의 섹터는 512바이트
- `cache` 또는 직접 I/O로 블록을 읽고 씀
- `free-map`은 블록 사용 여부를 비트맵으로 관리

### 8.3 파일 읽기/쓰기 동작

`file_read()`와 `file_write()`는 현재 파일 위치(`file_offset`)에서 동작합니다.

- 읽기: `inode_read_at()`으로 디스크 블록에서 데이터 복사
- 쓰기: `inode_write_at()`으로 디스크 블록에 데이터 기록
- 파일 포인터는 작업 후 이동

### 8.4 디렉터리 구조

`directory.c`는 디렉터리 엔트리를 관리합니다.

- 디렉터리는 파일 이름과 inode 번호 매핑
- `dir_lookup()`으로 이름 검색
- `dir_add()`, `dir_remove()`로 추가/삭제

### 8.5 파일 시스템의 동기화

파일 시스템은 여러 스레드에서 동시에 접근할 수 있어 동기화가 필요합니다.

- `filesys_lock` 또는 `lock`을 사용하여 파일 시스템 연산을 보호
- 파일 디스크립터 테이블도 스레드별로 관리

---

## 9. 가상 메모리(Virtual Memory)

`vm/` 모듈은 Pintos를 실제 OS에 더 가깝게 만듭니다.

### 9.1 가상 메모리가 필요한 이유

- 물리 메모리 부족 시 효율적인 관리
- 프로세스 간 메모리 분리
- demand paging으로 메모리 절약

### 9.2 Supplemental Page Table (SPT)

SPT는 프로세스가 가진 가상 페이지의 상태를 관리하는 구조입니다.

- 각 가상 주소에 대해 페이지 정보 저장
- `VM_ANON`, `VM_FILE`, `VM_STACK` 같은 타입 구분
- lazy load와 swap 처리를 지원

### 9.3 lazy loading

파일 기반 페이지는 실제로 접근될 때까지 로드하지 않습니다.

- `vm_alloc_page_with_initializer()`로 페이지를 예약
- 페이지 폴트 발생 시 `lazy_load_segment()` 호출
- 필요한 데이터를 디스크에서 읽어서 메모리에 매핑

### 9.4 swap과 페이지 교체

물리 메모리가 부족하면 일부 페이지를 swap 영역으로 내보냅니다.

- `swap_out()`은 물리 페이지를 swap 슬롯으로 기록
- `swap_in()`은 다시 메모리로 복원
- `frame` 테이블과 `page` 구조가 함께 작동

### 9.5 익명 페이지

익명 페이지는 파일이 아닌 메모리 영역입니다.

- `malloc()`으로 할당된 힙 메모리
- 사용자 스택
- 할당 시점에 초기화되지 않은 페이지

### 9.6 스택 확장

사용자 스택은 필요할 때만 확장될 수 있습니다.

- 스택 페이지 경계 아래에서 페이지 폴트 발생 시
- `vm` 모듈이 새로운 스택 페이지를 할당하고 매핑

---

## 10. 메모리와 주소 공간

Pintos는 가상 주소 공간과 물리 주소 공간을 분리합니다.

### 10.1 사용자 주소 vs 커널 주소

- 사용자 주소: `0x0000000000000000` ~ `0x8000000000000000` (가상)
- 커널 주소: `0x8000000000000000` 이상
- `PHYS_BASE`는 커널 주소의 시작을 나타냄

### 10.2 페이지 테이블과 매핑

x86-64는 4단계 페이지 테이블을 사용합니다.

- `pml4` 최상위 페이지 테이블
- 각각 512개 엔트리
- `pml4_get_page()`로 매핑 확인
- `pml4_set_page()`로 새 매핑 추가

### 10.3 물리 메모리와 커널 주소

커널은 물리 메모리에 직접 접근하지 않습니다. 물리 메모리는 커널 가상 주소로 매핑되어 사용됩니다.

- 커널은 `palloc_get_page()`로 커널 페이지 할당
- 페이지는 커널 가상 주소 `PHYS_BASE` 위쪽에 매핑됨

### 10.4 스택, 힙, 코드/데이터 영역

가상 주소 공간은 크게 세 부분으로 나뉩니다.

- **코드/데이터**: ELF 프로그램이 로드되는 낮은 주소 영역
- **힙**: 동적 메모리 할당이 성장하는 중간 영역
- **스택**: 높은 주소에서 시작해 아래로 성장

스택과 힙은 서로 만날 수 있으므로, 둘 다 페이지 단위로 관리해야 합니다.

---

## 11. 스택 레이아웃과 인자 전달 심화

스택은 고정된 구조가 아니라, 함수 호출마다 변화합니다. 하지만 프로그램 시작 시에는 매우 구체적인 형식이 필요합니다.

### 11.1 왜 스택이 아래쪽으로 성장하나요?

x86 아키텍처에서 `push` 명령은 `rsp`를 감소시키고 값을 저장합니다. 따라서 스택은 높은 주소에서 낮은 주소로 쌓입니다.

### 11.2 프로세스 시작 시 스택 구성

사용자 프로그램 시작 시 스택은 다음과 같은 형태입니다.

```
[높은 주소]
  0 (fake return address)
  argv[argc-1]
  ...
  argv[0]
  argv NULL terminator
  padding (alignment)
  "argN\0"
  ...
  "arg0\0"
[낮은 주소]
```

### 11.3 역순으로 넣는 이유

문자열 데이터를 스택에 역순으로 넣어야, 포인터 배열을 올바른 순서로 만들 수 있습니다.

- 먼저 `"world\0"`를 넣고 주소를 저장
- 다음에 `"hello\0"`를 넣고 주소를 저장
- 최종적으로 `argv[0]`이 가장 낮은 주소에 위치

### 11.4 8/16바이트 정렬

- x86-64는 스택 포인터가 16바이트 정렬 상태여야 함
- 시스템 콜 진입 시 `RSP`가 16바이트 정렬된 상태인지 확인해야 함
- `sp &= ~0x7` 같은 방식으로 정렬을 맞춤

### 11.5 `argv` 배열과 `argc`

`argv`는 문자열 주소의 배열입니다.

- `argv[0]`는 프로그램 이름
- `argv[argc]`는 NULL terminator
- 사용자 프로그램은 `main(int argc, char **argv)`를 통해 접근

---

## 12. 중요한 데이터 구조와 함수

### 12.1 `struct thread`

- 스레드 id, 상태, 우선순위
- 스택 포인터, 페이지 테이블 포인터
- 파일 디스크립터 테이블 참조

### 12.2 `struct file` / `struct inode`

- `struct file`은 열려 있는 파일의 상태를 저장
- `struct inode`는 파일의 디스크 블록 연결과 메타데이터를 저장

### 12.3 `struct page` (VM)

- 각 가상 페이지의 상태를 저장
- 파일 기반 또는 익명 페이지 구분
- swap slot 및 물리 프레임 참조

### 12.4 주요 함수

- `thread_create()`: 새로운 스레드 생성
- `process_exec()`: ELF 프로그램 로드 및 실행
- `load_segment()`: 파일 데이터를 페이지에 로딩
- `setup_stack()`: 사용자 스택 초기화
- `syscall_handler()`: 시스템 콜 처리
- `copy_in()`, `copy_out()`: 사용자 메모리 안전 복사
- `pml4_get_page()`, `pml4_set_page()`: 페이지 매핑 관리

---

## 13. 구현 팁과 자주 틀리는 부분

### 13.1 사용자 메모리 검증

- `is_user_vaddr()`만으로 충분하지 않음
- 포인터가 페이지 경계를 넘어가는지 확인해야 함
- `pml4_get_page()`로 실제 매핑이 있는지 확인

### 13.2 스택 정렬과 패딩

- 8바이트 정렬만 맞추면 안 됨
- `RSP`가 다시 16바이트 정렬 상태인지 확인
- padding을 잘못 계산하면 `main()` 진입 시 깨짐

### 13.3 ELF 로딩 오류

- `read_bytes`와 `zero_bytes` 계산 실수
- `validate_segment()` 검증 누락
- `install_page()` 실패 시 누수

### 13.4 페이지 폴트 처리

- 사용자 페이지 폴트는 `process_exit()` 또는 VM 핸들링
- 커널 페이지 폴트는 일반적으로 치명적
- `copy_in()`에서 페이지 폴트를 안전하게 처리해야 함

---

## 14. 테스트 전략

Pintos는 많은 자동화된 테스트를 제공합니다. 테스트를 이해하면 구현 범위와 검증 기준이 명확해집니다.

### 14.1 `tests/` 폴더 구조

- `threads/`: 스레드와 동기화 테스트
- `userprog/`: 프로세스, 시스템 콜 테스트
- `filesys/`: 파일 시스템 테스트
- `vm/`: 가상 메모리 테스트

### 14.2 테스트 작성 방식

- `make check` 또는 `pintos` 테스트 스크립트 사용
- 각 테스트는 `run` 스크립트로 실행
- 실패 시 로그와 결과를 분석

### 14.3 테스트 핵심 포인트

- 정상 경로와 오류 경로 모두 검증
- 경계 케이스(Null 포인터, 최대 크기, 페이지 경계)
- 프로세스 간 자원 정리 여부
- 시스템 콜 후 상태 복구

---

## 15. 러버덕 설명: Pintos를 쉽게 이해하려면

Pintos를 한 문장으로 설명하면 "작은 운영체제 학교"입니다.

- **스레드는 학생**: 각각 다른 일을 하는 인물
- **스케줄러는 선생님**: 누가 다음에 실행할지 정함
- **커널은 학교 운영진**: 모든 자원과 권한을 관리
- **사용자 프로그램은 과제**: 직접 자원을 쓰지 못하고 요청해야 함
- **메모리는 책상**: 잘못된 책상에 앉으면 사고가 남

### 15.1 학교 비유 확장

- **파일 시스템**은 도서관
  - 책을 빌리고 반납하는 과정이 파일 열기/닫기
- **시스템 콜**은 학생의 요청서
  - 학생이 운영진에게 요청서를 제출하면, 운영진이 처리
- **가상 메모리**는 각 학생에게 배정된 교실
  - 실제 건물(물리 메모리)은 제한되어 있고, 필요에 따라 교실을 바꿔 사용

---

## 16. 꼭 읽어야 할 파일과 위치

### 스레드 & 동기화
- `threads/thread.c`
- `threads/synch.c`
- `threads/interrupt.c`

### 프로세스 & 유저 프로그램
- `userprog/process.c`
- `userprog/syscall.c`
- `userprog/exception.c`
- `userprog/gdt.c`

### 파일 시스템
- `filesys/filesys.c`
- `filesys/file.c`
- `filesys/inode.c`
- `filesys/directory.c`
- `filesys/free-map.c`

### 가상 메모리
- `vm/vm.c`
- `vm/page.c`
- `vm/anon.c`
- `vm/file.c`
- `vm/swap.c`

이제 이 파일을 중심으로 코드를 읽으면, Pintos의 전체 구조를 더 빠르게 파악할 수 있습니다.
