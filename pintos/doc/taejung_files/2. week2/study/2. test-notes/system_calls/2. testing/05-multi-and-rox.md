# 05 — `multi-*` / `rox-*` 검증

## 1. 이 테스트가 검증하는 것
여러 프로세스가 동시에 실행될 때 fd table과 프로세스 자원이 분리되는지, 실행 중인 파일에 대한 쓰기가 금지되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### 프로세스별 fd table
- 위치: `pintos/threads/thread.h`, `pintos/userprog/syscall.c`
- 규칙 1: fd table은 프로세스별로 분리한다.
- 규칙 2: fork/exec/exit 경계에서 file 객체 수명을 일관되게 관리한다.

### 실행 파일 deny-write
- 위치: `pintos/userprog/process.c`, `pintos/userprog/syscall.c`
- 규칙 1: load 성공 후 executable file에 `file_deny_write()`를 적용한다.
- 규칙 2: 프로세스 종료 시 `file_allow_write()`와 close를 수행한다.

## 3. 실패 시 바로 확인할 것
- `multi-child-fd`에서 자식과 부모 fd table이 섞이는지 확인
- `multi-recurse`에서 자원 누수 또는 wait 동기화 실패가 있는지 확인
- `rox-simple`, `rox-child`, `rox-multichild`에서 실행 파일 write가 허용되는지 확인

## 4. 재검증
- `multi-recurse`, `multi-child-fd` 실행
- `rox-simple`, `rox-child`, `rox-multichild` 실행
- 필요 시 `exec-*`, `wait-*`, `write-normal`과 함께 회귀 확인
