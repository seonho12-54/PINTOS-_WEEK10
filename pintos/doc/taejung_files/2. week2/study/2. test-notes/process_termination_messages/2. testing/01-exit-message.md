# 01 — `exit` 메시지 검증

## 1. 이 테스트가 검증하는 것
사용자 프로세스 종료 시 `<process name>: exit(<status>)` 형식의 메시지가 정확히 출력되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `sys_exit()`
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: `exit(status)`의 status를 현재 프로세스 상태에 저장한다.
- 규칙 2: 저장 후 `thread_exit()`로 종료한다.

### `process_exit()`
- 위치: `pintos/userprog/process.c`
- 규칙 1: 사용자 프로세스 종료 시 정해진 형식으로 메시지를 출력한다.
- 규칙 2: 커널 스레드와 halt 경로에서는 출력하지 않는다.

## 3. 실패 시 바로 확인할 것
- 메시지 형식이 `%s: exit(%d)\n`와 정확히 일치하는지 확인
- `thread_current()->name`이 전체 프로세스 이름인지 확인
- `sys_exit()`와 `process_exit()`에서 중복 출력하지 않는지 확인
- 디버그 출력이 테스트 로그에 섞이지 않는지 확인

## 4. 재검증
- `exit` 단일 테스트 확인
- 필요 시 `wait-simple`, `wait-killed`로 status 전달 경계 재확인

