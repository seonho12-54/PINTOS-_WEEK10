# 04 — `priority-sema` 검증

## 1. 이 테스트가 검증하는 것
세마포어 대기열(waiters)에서 높은 priority 스레드가 먼저 깨어나는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `sema_down()`, `sema_up()`
- 위치: `pintos/threads/synch.c`
- 규칙 1: waiters에서 깨울 대상을 priority 기준으로 선택한다.
- 규칙 2: 깨운 뒤 preemption 판단 경로를 연계한다.

### `thread_unblock()`
- 위치: `pintos/threads/thread.c`
- 규칙 1: 세마포어 경로에서 넘어온 스레드도 READY 삽입 정책을 동일하게 적용한다.

## 3. 실패 시 바로 확인할 것
- `sema_up()`이 단순 FIFO pop만 수행하는지
- unblock 이후 선점 연결이 누락됐는지

## 4. 재검증
- `priority-sema` 단일 실행 후 통과 확인
