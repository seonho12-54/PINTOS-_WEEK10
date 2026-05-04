# 01 기능 2: 다음 단계는 `exit` 시스템 콜

GitHub 이슈: [#15 `[WEEK10] 테스트 - exit`](https://github.com/seonho12-54/PINTOS-_WEEK10/issues/15)

## 1. 지금 상태에서 왜 `exit`가 다음인가?

Argument Passing까지 끝나면 사용자 프로그램은 `main(argc, argv)`까지 들어갈 수 있다.

그 다음 사용자 프로그램이 정상 종료하려면 `exit(status)`를 호출하고, 커널이 그 시스템 콜을 받아 현재 프로세스를 종료해야 한다.

즉 다음 목표는:

**사용자 프로그램의 `exit(57)` 호출을 커널의 `syscall_handler()`에서 받아 `exit: exit(57)`을 출력하고 종료시키는 것**이다.

## 2. 구현 목표

### 기능이 하는 일

`tests/userprog/exit.c`는 아래처럼 동작한다.

```c
exit (57);
fail ("should have called exit(57)");
```

정상이라면 `exit(57)`에서 프로세스가 종료되어야 하므로 `fail()`까지 가면 안 된다.

기대 출력은:

```text
(exit) begin
exit: exit(57)
```

## 3. 수정해야 할 핵심 위치

### 3.1 `pintos/userprog/syscall.c`

역할:

- `syscall_handler()`에서 시스템 콜 번호를 읽는다.
- `SYS_EXIT`이면 첫 번째 인자 `status`를 읽는다.
- `exit(status)` 처리 함수로 넘긴다.

확인할 레지스터:

- `f->R.rax`: 시스템 콜 번호
- `f->R.rdi`: 첫 번째 인자, `exit(status)`의 `status`

### 3.2 `pintos/userprog/process.c`

역할:

- `process_exit()`에서 현재 프로세스 이름과 종료 상태를 출력한다.
- 출력 형식은 테스트가 기대하는 형식과 정확히 맞춘다.

출력 형식:

```c
printf ("%s: exit(%d)\n", thread_current ()->name, status);
```

### 3.3 `pintos/include/threads/thread.h`

역할:

- 현재 thread가 자기 종료 상태를 기억할 수 있도록 `exit_status` 같은 필드를 둔다.
- 나중에 `wait` 구현 때 부모가 이 값을 회수하게 된다.

## 4. 구현 순서

1. `struct thread`에 `int exit_status` 필드를 추가한다.
2. thread 생성/초기화 경로에서 기본값을 정한다.
3. `syscall_handler()`에서 `SYS_EXIT` 분기를 만든다.
4. `SYS_EXIT` 처리 시 `f->R.rdi`를 종료 상태로 읽는다.
5. 현재 thread의 `exit_status`에 저장한다.
6. `thread_exit()`을 호출해 프로세스를 종료한다.
7. `process_exit()`에서 `현재스레드이름: exit(status)` 형식으로 출력한다.

## 5. 주의할 점

- `exit`는 반환하면 안 된다. 처리 후 반드시 종료 경로로 들어가야 한다.
- `SYS_EXIT` 번호는 `pintos/include/lib/syscall-nr.h`에 이미 정의되어 있다.
- 출력 형식이 조금만 달라도 `exit.ck`가 실패한다.
- `wait`는 나중 범위다. 이번 feature에서는 부모가 exit status를 회수하는 것까지 깊게 들어가지 않는다.
- 다만 `exit_status` 저장은 나중 `wait`를 위해 미리 해두는 편이 좋다.

## 6. 테스트 기준

우선 목표 테스트:

```text
tests/userprog/exit
```

기대 체크 파일:

```text
pintos/tests/userprog/exit.ck
```

통과 기준:

- `(exit) begin`이 출력된다.
- `exit: exit(57)`이 출력된다.
- `fail("should have called exit(57)")` 경로로 가지 않는다.

## 7. 다음 feature로 이어지는 흐름

`exit`가 되면 그 다음은 보통 아래 순서가 자연스럽다.

1. `wait`: 부모가 자식 종료 상태를 기다리고 회수한다.
2. `write`: `msg()` 출력과 일반 stdout 테스트를 안정화한다.
3. `halt`: 커널 종료 시스템 콜을 처리한다.
4. 파일 관련 syscall: `create`, `open`, `read`, `write`, `close` 순서로 확장한다.

현재 바로 다음 이슈 기준으로는 `exit`부터 잡는 게 맞다.
