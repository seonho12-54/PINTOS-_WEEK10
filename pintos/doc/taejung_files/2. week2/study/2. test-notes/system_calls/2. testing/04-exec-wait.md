# 04 — `exec` / `wait` 검증

## 1. 이 테스트가 검증하는 것
프로세스 실행, 실행 실패 전달, 자식 종료 상태 회수, 중복 wait/비자식 wait 실패 처리가 올바른지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `exec` syscall
- 위치: `pintos/userprog/syscall.c`, `pintos/userprog/process.c`
- 규칙 1: 명령 문자열은 User Memory Access에서 안전하게 복사된 값을 사용한다.
- 규칙 2: load 실패 시 부모가 실패를 관측할 수 있어야 한다.

### `wait` syscall
- 위치: `pintos/userprog/syscall.c`, `pintos/userprog/process.c`
- 규칙 1: 현재 프로세스의 자식 pid만 wait 가능하다.
- 규칙 2: 같은 자식은 한 번만 wait 가능하다.
- 규칙 3: 자식 종료 상태를 정확히 반환한다.

## 3. 실패 시 바로 확인할 것
- `exec-missing`에서 실패를 성공 pid처럼 반환하는지 확인
- `wait-twice`에서 중복 wait가 허용되는지 확인
- `wait-bad-pid`에서 비자식 pid를 기다리는지 확인
- `wait-killed`에서 비정상 종료 status가 맞는지 확인

## 4. 재검증
- `exec-once`, `exec-arg`, `exec-missing` 실행
- `wait-simple`, `wait-twice`, `wait-bad-pid`, `wait-killed` 실행
- 필요 시 `fork-once`, `fork-multiple`, `fork-recursive` 함께 확인
