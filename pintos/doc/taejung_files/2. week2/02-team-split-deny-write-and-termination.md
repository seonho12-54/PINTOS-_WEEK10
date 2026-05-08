# Denying Writes/Process Termination 4인 분업 기준

이 문서는 `denying_writes_to_executables`와 `process_termination_messages` 구현을 4명이 병렬로 진행할 때의 책임 경계, 공통 계약, 병합 순서를 정리합니다.

핵심 원칙은 테스트 파일 단위 분배가 아니라, 구현 책임 단위를 `A/B/C/D`로 고정하는 것입니다. 각 담당자는 자기 구현 축을 끝까지 소유하고, 공유 인터페이스는 초기에 합의해 충돌을 줄입니다.

---

## 1. 공통 학습 범위

병렬 개발 전 전원이 공통으로 맞춰야 하는 범위는 아래 흐름입니다.

```text
sys_exec/process_exec
-> 실행 파일 열기
-> file_deny_write 적용
-> 실행 중 보호 유지
-> process_exit에서 allow/close 정리
```

```text
sys_exit/예외 종료
-> 종료 코드 저장
-> "name: exit(status)" 출력
-> wait에서 1회 회수
```

### 모두가 알아야 할 것

- 실행 파일 쓰기 금지는 `exec 성공 직후` 시작하고 `process_exit()`에서 해제된다는 것
- 종료 상태(exit status)는 부모가 `wait()`에서 정확히 한 번 회수한다는 것
- 잘못된 포인터/예외 종료 경로와 `exit(-1)` 정책이 충돌하지 않도록 종료 정책을 통일한다는 것
- 출력 메시지 형식은 `"name: exit(status)"`로 고정한다는 것

### 깊게 공부할 영역

- A: 실행 파일 포인터 수명과 deny/allow 호출 경계
- B: 종료 코드 저장, 종료 메시지, 예외 종료 정책
- C: 부모-자식 종료 동기화와 wait 회수 구조
- D: 통합 테스트, 회귀 테스트, 경계 조건 검증

---

## 2. A/B/C/D 구현 소유권

| 담당 | 구현 소유권 | 핵심 책임 | 주요 수정 파일 |
| --- | --- | --- | --- |
| A | 실행 파일 추적/deny-write | 실행 파일 포인터 보관, `file_deny_write()`, 종료 시 `file_allow_write()/file_close()` | `userprog/process.c`, `include/threads/thread.h` |
| B | 종료 코드/종료 메시지 | `sys_exit`-`process_exit` 상태 저장, `exit(status)` 출력 규칙, 비정상 종료 `-1` 통일 | `userprog/syscall.c`, `userprog/process.c`, `userprog/exception.c` |
| C | wait 회수/부모-자식 동기화 | wait 1회 회수 보장, 중복 wait/없는 자식 처리, 종료 상태 전달 | `userprog/process.c`, `include/threads/thread.h`, `threads/thread.c` |
| D | 통합/검증/회귀 | `exec`-`rox`-`wait` 연동 검증, 경계 케이스 정리, 최종 통합 테스트 | `tests/userprog/*`, 필요 시 `userprog/*` 보완 |

분배가 애매할 때는 다음 기준을 사용합니다.

- deny-write 시작/해제 시점 문제는 A가 먼저 본다.
- 출력 형식/exit code 정책 문제는 B가 먼저 본다.
- wait 반환값/중복 회수 문제는 C가 먼저 본다.
- 개별 기능은 맞는데 통합에서 깨지는 경우는 D가 먼저 본다.

---

## 3. 구현 4분할 작업 단위

역할별 소유권을 실제 구현 작업으로 바로 옮길 때는 아래 4개 단위로 쪼개서 진행합니다.

### A: Executable write deny 수명 구현

- `exec` 성공 직후 실행 파일에 `file_deny_write()`를 적용한다.
- 실행 중 보호를 유지하고 `process_exit()`에서 `file_allow_write()`와 `file_close()`를 정리한다.
- 실패/롤백 경로에서 실행 파일 포인터 누수가 없도록 한다.

### B: 종료 상태/종료 메시지 구현

- `sys_exit()`와 예외 종료를 하나의 종료 코드 정책으로 정리한다.
- `"name: exit(status)"` 출력 형식을 고정하고 중복 출력을 막는다.
- 비정상 종료 코드를 `-1`로 통일한다.

### C: wait 회수/동기화 구현

- 부모가 자식 종료 상태를 정확히 한 번만 회수하도록 한다.
- 중복 wait, 비자식 tid, 없는 tid는 `-1` 처리한다.
- 자식 선종료/부모 후대기 케이스에서 상태 손실이 없도록 한다.

### D: 통합 경계 처리 구현

- A/B/C를 연결하는 경계 조건(실패 경로, 순서 의존, 동시성)을 보완한다.
- `exec-*`, `wait-*`, `rox-*` 중심 회귀에서 깨지는 연결부를 마무리한다.

---

## 4. 구현 계약

### 계약 1: deny-write 수명

- `process_exec()`에서 실행 파일 로드가 성공한 뒤 `file_deny_write()`를 적용한다.
- 실행 중에는 해당 실행 파일에 대한 쓰기를 막는다.
- `process_exit()`에서 `file_allow_write()` 후 `file_close()`로 정리한다.
- 실패 경로에서도 열린 실행 파일 포인터 누수가 없어야 한다.

### 계약 2: 종료 코드 정책

- 정상 `sys_exit(status)`는 전달받은 status를 저장한다.
- 비정상 종료(예외/잘못된 사용자 메모리)는 `-1`로 저장한다.
- 부모가 `wait()`로 회수하는 값은 저장된 종료 코드와 일치해야 한다.

### 계약 3: 종료 메시지 출력

- 출력 형식은 `"name: exit(status)"`로 고정한다.
- 출력은 프로세스 종료 시점에 1회만 발생한다.
- 커널 스레드/특수 스레드 출력 정책은 팀 합의대로 일관되게 유지한다.

### 계약 4: wait 1회 회수

- 부모는 동일 자식에 대해 wait를 한 번만 성공할 수 있다.
- 이미 회수한 자식, 부모의 자식이 아닌 tid, 없는 tid는 `-1`을 반환한다.
- 자식이 이미 종료했어도 상태는 손실 없이 회수되어야 한다.

---

## 5. 병합 순서

공유 파일 충돌을 줄이기 위해 아래 순서를 권장합니다.

1. A 1차 병합: 실행 파일 포인터 필드/deny-write 기본 뼈대
2. B 1차 병합: 종료 코드 저장 위치, 종료 메시지 출력 경로 고정
3. C 1차 병합: wait 회수 구조(1회 보장, 자식 식별) 고정
4. A 2차 병합: deny/allow/close 실패 경로 정리
5. B 2차 병합: 예외 종료 `-1` 통일
6. C 2차 병합: 중복 wait/없는 자식 경계 처리
7. D 통합 병합: `exec-*`, `wait-*`, `rox-*`, 종료 메시지 회귀 확인

---

## 6. 테스트 기반 진행

### 담당 테스트 묶음

- A: `rox-simple`, `rox-child`, `rox-multichild`, `exec-read`
- B: `exit`, `wait-killed`, bad pointer/예외 종료 연계 케이스
- C: `wait-simple`, `wait-twice`, `wait-bad-pid`, `multi-*` 일부
- D: `exec-*`, `wait-*`, `rox-*` 전체 회귀 + 통합 시나리오

### 진행 원칙

- 각 담당자는 자기 테스트 묶음을 먼저 통과시킨다.
- 공유 구조를 바꾸면 인접 담당 테스트를 반드시 함께 돌린다.
- 통합 검증은 D 단독 책임이 아니라 전원 공동으로 수행한다.
- 최종 완료 기준은 담당 테스트 통과가 아니라 `userprog` 회귀 통과다.

---

## 7. 완료 기준

다음 조건을 모두 만족하면 완료로 본다.

1. 실행 중인 실행 파일에 write가 차단되고 종료 시 정상 해제된다.
2. 종료 코드가 정상/비정상 경로에서 일관되게 저장된다.
3. 종료 메시지가 형식과 횟수를 만족한다.
4. `wait()`가 1회 회수, 중복/비자식/없는 tid 경계를 만족한다.
5. 통합 브랜치에서 `exec-*`, `wait-*`, `rox-*` 회귀가 통과한다.
