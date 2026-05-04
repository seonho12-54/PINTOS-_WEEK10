# System Call/User Memory 4인 분업 기준

이 문서는 System Calls와 User Memory Access를 4명이 병렬로 구현할 때의 책임 경계, 내부 테스트 기준, 병합 순서를 정리합니다.

핵심 원칙은 `테스트 파일 하나씩 분배`가 아니라 `테스트 묶음 + 기능 소유권`으로 나누는 것입니다. User Memory Access는 모든 syscall의 공통 전제이므로, 공용 검증 helper 계약을 먼저 고정한 뒤 각 syscall 담당자가 그 helper를 사용합니다.

이 방식은 구현 내용을 전부 공부한 뒤 시작하자는 뜻이 아닙니다. 먼저 모두가 syscall 흐름과 helper 사용법만 얕게 맞추고, 각 담당자는 자기 영역만 깊게 파는 방식입니다.

---

## 1. 공통 학습 범위

병렬 개발 전에 전원이 깊게 공부해야 하는 범위는 작게 유지합니다. 공통으로는 다음 흐름만 맞추면 됩니다.

```text
syscall_handler
-> User Memory Access helper
-> syscall별 구현 함수
-> RAX 반환 또는 exit(-1)
```

### 모두가 알아야 할 것

- syscall이 `syscall_handler()`로 들어오고 `struct intr_frame`의 레지스터에서 인자를 읽는다는 것
- 사용자 포인터는 syscall 담당자가 직접 역참조하지 않고 공용 helper로만 다룬다는 것
- helper가 실패하면 해당 프로세스는 `exit(-1)`로 종료된다는 것
- 각 syscall은 성공/실패 결과를 테스트 기대값에 맞게 `RAX`로 반환한다는 것
- 자기 담당 테스트를 돌리는 방법과 실패 로그를 확인하는 방법

### 깊게 공부할 영역

- A: User Memory Access 내부 구현, bad pointer, boundary, page fault 처리
- B: syscall dispatch, syscall number, 인자 개수, `halt`, `exit`, `read`, `write`
- C: fd table, file syscall, 파일 객체 수명
- D: `fork`, `exec`, `wait`, `multi-*`, `rox-*`

즉, 전원이 모든 구현 세부사항을 먼저 공부하지 않습니다. 공통 계약만 먼저 고정하고, 나머지는 담당 영역별로 학습합니다.

---

## 2. User Memory Access 계약

담당 A는 syscall 구현자가 직접 사용자 포인터를 역참조하지 않도록 공용 helper의 이름, 반환 규칙, 실패 정책을 먼저 확정합니다.

### 담당 범위

- 사용자 주소 범위 검증: kernel address, NULL, unmapped page 차단
- syscall 인자 읽기: syscall number와 정수/포인터 인자를 안전하게 읽기
- 문자열 복사: NUL 종료까지 page boundary를 넘어서도 안전하게 확인
- 버퍼 검증/복사: 시작 주소뿐 아니라 `[addr, addr + size)` 전체 범위 확인
- 실패 정책: 잘못된 사용자 메모리 접근은 kernel panic이 아니라 현재 프로세스 `exit(-1)`

### 필요한 계약

계약은 syscall 담당자와 User Memory Access 담당자 사이에서 결과가 달라지면 안 되는 부분만 정합니다. 내부 구현용 helper는 담당 A가 자유롭게 나눌 수 있습니다.

#### 계약 1: 잘못된 사용자 메모리 접근은 `exit(-1)`이다

- 적용 대상: 모든 syscall
- 규칙: 사용자 포인터가 NULL, kernel address, unmapped page, boundary 중간의 invalid page를 가리키면 값을 반환하지 않고 현재 프로세스를 `exit(-1)`로 종료한다.
- 이유: syscall마다 `return -1`, `return 0`, kernel panic으로 갈라지는 것을 막기 위함이다.

#### 계약 2: 문자열 인자는 NUL 종료까지 검증한다

- 적용 대상: `create`, `remove`, `open`, `exec`
- 규칙: 문자열 시작 주소만 검사하지 않고, NUL 문자를 만날 때까지 모든 페이지가 사용자 영역이고 매핑되어 있는지 확인한다.
- 규칙: 검증에 성공하면 syscall 구현은 안전한 커널 문자열을 받거나, 최소한 NUL까지 안전하다고 보장된 문자열만 사용한다.
- 규칙: 빈 문자열 자체의 성공/실패 정책은 User Memory Access가 아니라 각 syscall 정책에서 정한다.
- 이유: `open-boundary`, `exec-boundary`, `*-bad-ptr`, `open-empty`, `create-empty`의 책임 경계를 분리하기 위함이다.

#### 계약 3: 버퍼 인자는 전체 범위를 검증한다

- 적용 대상: `read`, `write`
- 규칙: 버퍼 시작 주소만 검사하지 않고 `[buffer, buffer + size)` 전체 범위가 사용자 영역이고 매핑되어 있는지 확인한다.
- 규칙: `size == 0`이면 실제 메모리 접근이 없으므로 버퍼 검증 없이 syscall 정책에 따라 `0`을 반환할 수 있다.
- 규칙: `write`는 사용자 버퍼를 커널이 읽는 경로이고, `read`는 커널이 사용자 버퍼에 쓰는 경로라는 방향을 구분한다.
- 이유: `read-boundary`, `write-boundary`, `read-bad-ptr`, `write-bad-ptr`, `write-zero`, `read-zero`의 결과를 일관되게 맞추기 위함이다.

#### 계약 4: syscall 구현자는 사용자 포인터를 직접 역참조하지 않는다

- 적용 대상: 포인터 인자를 받는 모든 syscall
- 규칙: syscall 구현자는 `char *file`, `void *buffer` 같은 사용자 주소를 받은 뒤 바로 `strlen`, `memcpy`, `file_read`, `putbuf` 등에 넘기지 않는다.
- 규칙: 먼저 User Memory Access helper를 통해 검증하거나 안전한 커널 값으로 변환한다.
- 이유: syscall별 구현 안에 포인터 검증이 흩어지면 bad pointer와 boundary 테스트를 고치기 어려워지기 때문이다.

### 구현 이름 예시

아래 이름은 예시입니다. 최종 이름은 팀이 코드에 맞춰 정하되, 위 계약의 동작은 유지합니다.

- `fail_invalid_user_memory(void)`
- `copy_in_string(const char *user_str)`
- `validate_user_buffer(const void *user_buffer, size_t size)`
- `validate_user_buffer_writable(void *user_buffer, size_t size)`

### AI 활용 기준

User Memory Access 계약은 AI를 사용하기 좋은 부분입니다. 구현 전체를 맡기기보다는 다음 용도로 사용합니다.

- helper 함수 이름과 역할 초안 만들기
- bad pointer, NULL, empty string, page boundary 케이스 누락 검토
- `open`, `read`, `write`, `exec`가 같은 helper 계약으로 처리되는지 점검
- 테스트 목록을 기준으로 계약에서 빠진 실패 정책 찾기

최종 함수명, 실패 정책, 반환 규칙은 팀이 코드에 맞춰 확정합니다. AI 초안을 그대로 쓰기보다는 팀의 공통 인터페이스로 고정하는 것이 중요합니다.

### 담당 테스트

- `bad-read`, `bad-read2`
- `bad-write`, `bad-write2`
- `bad-jump`, `bad-jump2`
- `create-bad-ptr`, `exec-bad-ptr`, `open-bad-ptr`, `read-bad-ptr`, `write-bad-ptr`
- `create-bound`, `exec-boundary`, `fork-boundary`, `open-boundary`, `read-boundary`, `write-boundary`
- `create-null`, `open-null`, `open-empty`, `create-empty`

---

## 3. System Call 테스트 묶음 분배

| 담당 | 기능 소유권 | 주요 테스트 | 주요 수정 파일 |
| --- | --- | --- | --- |
| A | User Memory Access + syscall 인자 검증 계약 | `bad-*`, `*-bad-ptr`, `*-boundary`, null/empty pointer | `userprog/syscall.c`, `userprog/exception.c` |
| B | syscall dispatch + 기본 종료 계열 + read/write | `halt`, `exit`, `read-*`, `write-*`, unsupported syscall 실패 정책 | `userprog/syscall.c` |
| C | file syscall + fd table | `create-*`, `open-*`, `close-*`, `filesize`, `seek`, `tell` | `userprog/syscall.c`, `threads/thread.h` |
| D | process syscall + 통합 | `exec-*`, `wait-*`, `fork-*`, `multi-*`, `rox-*` | `userprog/syscall.c`, `userprog/process.c` |

분배가 애매한 테스트는 다음 기준으로 소유자를 정합니다.

- pointer 또는 boundary 때문에 실패하면 A가 먼저 본다.
- fd 조회/수명 문제면 C가 먼저 본다.
- stdin/stdout 또는 `read`/`write` 반환 byte 수 문제면 B가 먼저 본다.
- 부모-자식 동기화, exit status 전달, 실행 파일 write deny 문제면 D가 먼저 본다.
- syscall number dispatch 또는 `RAX` 반환 누락이면 B가 먼저 본다.

---

## 4. 공유 파일 수정 경계

`syscall.c`는 모든 담당자가 만지는 중심 파일이므로 병합 충돌을 줄이기 위해 아래 순서를 지킵니다.

1. A: User Memory Access helper와 bad pointer 실패 경로를 먼저 추가
2. B: `syscall_handler()`의 switch/case 틀, 반환값 기록 규칙, `halt`, `exit`, stdout `write` 기본 흐름 추가
3. C: fd table helper, file syscall 구현, `thread.h` fd 필드 추가
4. B: C의 fd table helper를 사용해 일반 파일 `read`/`write` 마무리
5. D: process syscall, rox/multi 통합 로직 추가

파일별 책임 경계는 다음과 같습니다.

- `userprog/syscall.c`: dispatch, user memory helper, syscall별 구현 함수, fd helper
- `threads/thread.h`: 현재 프로세스의 fd table, next fd, 실행 파일 포인터 등 상태 필드
- `userprog/process.c`: `fork`, `exec`, `wait`, 프로세스 종료 정리, 실행 파일 write deny 연결
- `userprog/exception.c`: user/kernel page fault 처리 경계, bad memory 접근 시 현재 프로세스 종료

공통 규칙:

- 사용자 포인터를 syscall 담당자가 직접 역참조하지 않는다.
- syscall별 구현 함수는 검증된 값 또는 안전 복사된 커널 값을 받는다.
- 파일 시스템 접근은 팀에서 정한 하나의 전역 lock 규칙을 따른다.
- 실패 반환값은 syscall별 테스트 기대값에 맞춘다. bad pointer는 반환값이 아니라 `exit(-1)`로 처리한다.

---

## 5. 테스트 기반 구현 프로세스

이 문서는 이미 User Memory Access 계약, 테스트 소유권, 공유 파일 수정 경계를 정해 둔 상태입니다. 따라서 초반에 전원이 모든 구현 세부사항을 오래 같이 공부할 필요는 없습니다. 문서 계약을 기준으로 각자 담당 영역을 바로 파고들되, 문서 계약이 실제 코드 인터페이스로 옮겨지는 첫 병합은 빠르게 진행합니다.

### 기본 원칙

- 테스트 기반 개발은 각자 담당 테스트 묶음 단위로 진행한다.
- 초반 공동 학습은 문서 계약 확인과 인터페이스 확정에만 집중한다.
- 각자 전체 구현을 끝낸 뒤 마지막에 한 번에 합치지는 않는다.
- 문서 계약이 실제 코드에 반영되는 지점, 즉 helper 시그니처, dispatch 틀, fd table 구조, exit status 저장 방식은 먼저 병합한다.
- 본인 테스트만 통과해도 공통 기능을 건드렸다면 의존 테스트까지 같이 확인한다.

### 초반 킥오프 범위

초반 30분~1시간은 전원이 아래 항목만 같이 맞춥니다.

- 문서의 User Memory Access 계약을 읽고 실패 정책을 `exit(-1)`로 고정한다.
- helper 함수명과 시그니처를 확정한다.
- syscall dispatch 함수 구조와 syscall별 구현 함수 이름을 확정한다.
- fd table 구조 초안과 fd helper 이름을 확정한다.
- exit status 저장 위치와 `wait`가 읽을 자료구조 방향을 확정한다.
- 브랜치/병합 순서와 각자 돌릴 테스트 묶음을 확정한다.

### 권장 진행 순서

1. 1차 병합: 문서 계약을 코드 인터페이스로 반영한다. A helper 선언, B syscall 틀, `exit` 기본 흐름, C fd 구조 초안을 포함한다.
2. 2차 병합: A bad pointer/boundary 계열 테스트 통과
3. 3차 병합: C file syscall 계열 테스트 통과
4. 4차 병합: B `read`/`write` 계열 테스트 통과
5. 5차 병합: D `exec`/`wait`/`fork`/`rox` 계열 테스트 통과
6. 최종 병합: `userprog` 전체 테스트 통과

### 병합 시 추가 확인

- A가 user memory helper를 바꾸면 B/C/D의 포인터 관련 테스트도 확인한다.
- B가 `exit` 또는 syscall dispatch를 바꾸면 D의 `wait`와 process syscall 흐름도 확인한다.
- C가 fd table 또는 file 객체 수명 관리를 바꾸면 B의 `read`/`write`, D의 `exec`/`rox-*`도 확인한다.
- D가 process lifecycle을 바꾸면 `multi-*`, `wait-*`, `rox-*`뿐 아니라 종료 메시지와 exit status도 확인한다.

---

## 6. 통합 기준

병합은 담당자 단위로 한 번에 끝내기보다, 공유 인터페이스와 기능 단위로 나누어 진행합니다. 병합 전에는 최소한 다음 순서로 회귀 테스트를 확인합니다.

1. A 병합 후: bad pointer/boundary 계열 테스트
2. B 1차 병합 후: `halt`, `exit`, stdout `write` 기본 흐름
3. C 병합 후: `create-*`, `open-*`, `close-*`
4. B 2차 병합 후: `read-*`, `write-*`
5. D 병합 후: `exec-*`, `wait-*`, `fork-*`, `multi-*`, `rox-*`
6. 최종 병합 전: `userprog` 전체 테스트

최종 완료 기준은 각자 담당 테스트 통과가 아니라, 병합된 브랜치에서 `userprog` 전체 테스트가 통과하는 것입니다.
