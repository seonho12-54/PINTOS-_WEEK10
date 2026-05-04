# 01 — `rox-*` 테스트 검증

## 1. 이 테스트가 검증하는 것
실행 중인 executable file에 대한 쓰기가 거부되고, 프로세스 종료 후 file 상태가 정상 복구되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `load()`
- 위치: `pintos/userprog/process.c`
- 규칙 1: executable file open 성공 직후 `file_deny_write()`를 호출한다.
- 규칙 2: 실행 중에는 해당 file 객체를 닫지 않고 thread field에 보관한다.

### `sys_write()`
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: 일반 file fd에는 `file_write()`를 호출한다.
- 규칙 2: deny-write로 인한 실패 반환을 syscall 반환값에 반영한다.

### `process_exit()`
- 위치: `pintos/userprog/process.c`
- 규칙 1: executable field가 있으면 `file_allow_write()` 후 `file_close()`한다.
- 규칙 2: field를 NULL로 정리한다.

## 3. 실패 시 바로 확인할 것
- `load()` 성공 경로에서 실행 파일을 닫아버리는지 확인
- `file_deny_write()`를 호출한 file과 fd table에서 write하는 file의 관계 확인
- cleanup에서 allow/close 순서가 빠졌는지 확인
- fork 자식이 부모 file pointer를 얕은 복사해 중복 close하는지 확인

## 4. 재검증
- `rox-simple` 단일 실행 통과 확인
- `rox-child`, `rox-multichild`로 fork/exit 경계 확인

