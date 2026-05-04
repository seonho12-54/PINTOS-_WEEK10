# 01 — `halt` / `exit` 검증

## 1. 이 테스트가 검증하는 것
가장 기본적인 syscall 진입, dispatch, 종료 상태 기록, 종료 메시지 출력이 동작하는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `syscall_handler()`
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: syscall number를 읽고 `halt`, `exit` 구현으로 분기한다.
- 규칙 2: 지원 syscall별 필요한 인자를 올바르게 해석한다.

### `exit` syscall
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: 전달받은 status를 현재 프로세스 종료 상태로 기록한다.
- 규칙 2: 테스트가 기대하는 종료 메시지와 프로세스 정리를 수행한다.

## 3. 실패 시 바로 확인할 것
- syscall number dispatch가 잘못되어 다른 syscall로 들어가는지 확인
- `exit(status)`의 status가 저장되지 않는지 확인
- `halt`가 power off 경로로 연결되는지 확인

## 4. 재검증
- `halt`, `exit` 단일 실행 통과 확인
- 필요 시 `wait-killed`로 종료 상태 경계 재확인
