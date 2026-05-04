# 05 — User Memory Helper 계약 정리

## 1. 목적

이 문서는 System Calls 구현 담당자들이 공통으로 사용할 User Memory Access helper의 이름, 위치, 호출 규칙을 정리합니다.

핵심 원칙은 syscall 구현 함수가 사용자 포인터를 신뢰하지 않고, 커널이 해당 주소를 읽거나 쓰기 전에 반드시 공통 helper를 호출하는 것입니다.

---

## 2. 파일 위치

구현 위치는 다음 파일로 통일합니다.

```text
pintos/userprog/syscall.c
```

현재 단계에서는 별도 레이어나 새 모듈을 만들지 않고, `syscall.c` 내부의 `static` helper 함수로 둡니다.

---

## 3. 필요한 include

`syscall.c` 상단 include 영역에 다음 헤더가 필요합니다.

```c
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "threads/vaddr.h"
#include "threads/mmu.h"
```

- `<stdbool.h>`: `bool` 반환 helper 사용
- `<stdint.h>`: `uint8_t`, 정수형 주소 계산
- `<stddef.h>`: `size_t`
- `"threads/vaddr.h"`: `is_user_vaddr()`, `PGSIZE`, `pg_round_down()`
- `"threads/mmu.h"`: `pml4_get_page()`

---

## 4. 함수 선언

`sys_exit()`는 User Memory helper가 실패 경로에서 호출하므로 helper 선언보다 먼저 prototype을 둡니다.

```c
static void sys_exit(int status);

static void fail_invalid_user_memory(void);
static bool is_valid_user_ptr(const void *uaddr);
static void validate_user_ptr(const void *uaddr);
static void validate_user_buffer(const void *buffer, size_t size);
static void validate_user_string(const char *str);
```

---

## 5. 함수별 역할

### 5.1 `fail_invalid_user_memory()`

잘못된 사용자 메모리 접근을 만났을 때 공통 실패 경로로 보냅니다.
이 함수는 반환하지 않는다고 생각하고 사용합니다. 실패 정책이 바뀌면 이 함수만 수정합니다.

### 5.2 `is_valid_user_ptr()`

사용자 주소 하나가 유효한지 판별만 합니다.
이 함수는 `bool`만 반환하고 프로세스를 종료하지 않습니다.

### 5.3 `validate_user_ptr()`

사용자 주소 하나를 검증합니다. 실패하면 현재 프로세스를 `exit(-1)` 경로로 종료합니다.
syscall 구현부에서는 대부분 이 함수나 아래의 `validate_user_buffer()`, `validate_user_string()`을 호출합니다.

### 5.4 `validate_user_buffer()`

`buffer + size` 범위 전체가 사용자 영역에 있고 page table에 매핑되어 있는지 확인합니다.
시작 주소만 검사하면 page boundary를 넘어가는 bad buffer를 놓칠 수 있으므로, 범위가 걸치는 모든 page를 확인합니다.

### 5.5 `validate_user_string()`

NUL 종료 문자열 전체가 사용자 영역에 있고 매핑되어 있는지 확인합니다.
문자열은 길이를 syscall 인자로 받지 않으므로 `'\0'`을 만날 때까지 한 바이트씩 검증합니다.

---

## 6. 호출 계약

syscall 구현 함수는 사용자 포인터를 실제로 사용하기 전에 다음 규칙으로 helper를 호출합니다.

- 포인터 하나만 받는 경우: `validate_user_ptr(ptr)`
- 버퍼와 크기를 받는 경우: `validate_user_buffer(buffer, size)`
- 문자열 포인터를 받는 경우: `validate_user_string(str)`
- `validate_*()` 함수는 실패 시 반환하지 않고 `exit(-1)`로 현재 프로세스를 종료합니다.
- syscall 구현부에서 직접 `is_valid_user_ptr()`만 호출하고 실패 처리를 생략하지 않습니다.

---

## 7. syscall별 사용 예

### `write(fd, buffer, size)`

```c
validate_user_buffer(buffer, size);
```

`putbuf()`는 커널이 사용자 버퍼를 읽는 지점이므로 호출 전에 반드시 `validate_user_buffer()`를 통과해야 합니다.

### `read(fd, buffer, size)`

```c
validate_user_buffer(buffer, size);
```

`read()`는 사용자 버퍼에 데이터를 써야 하므로, 이후 writable 검사가 필요해지면 `validate_user_buffer_writable()`로 확장할 수 있습니다.

### `create(file)`, `open(file)`, `exec(cmd_line)`

```c
validate_user_string(file);
validate_user_string(cmd_line);
```

파일 이름과 command line은 문자열 포인터이므로 NUL 종료까지 검증합니다.

---

## 8. 팀 공유 기준

System Calls 담당자는 사용자 포인터를 받는 syscall을 구현할 때 이 helper 계약을 따른다.

- bad pointer는 syscall별 임시 반환값으로 처리하지 않고 `exit(-1)`로 종료한다.
- 포인터 검증 코드를 syscall마다 중복 작성하지 않는다.
- page boundary 테스트를 위해 시작 주소뿐 아니라 전체 범위를 검증한다.
- 검증 전에는 사용자 포인터를 직접 역참조하지 않는다.

관련 테스트는 `*-bad-ptr`, `*-boundary`, `bad-read`, `bad-write` 계열입니다.
