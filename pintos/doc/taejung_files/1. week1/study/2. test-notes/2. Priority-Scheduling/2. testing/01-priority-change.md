# 01 — `priority-change` 검증

## 1. 이 테스트가 검증하는 것
현재 실행 스레드의 priority를 변경했을 때 스케줄러가 즉시 재평가되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `thread_set_priority()`
- 위치: `pintos/threads/thread.c`
- 규칙 1: 현재 스레드의 base priority를 갱신한다.
- 규칙 2: 변경 후 더 높은 READY 스레드가 있으면 `thread_yield()`로 양보한다.

### `thread_get_priority()`
- 위치: `pintos/threads/thread.c`
- 규칙 1: 테스트가 기대하는 effective priority 값을 일관되게 반환한다.

## 3. 실패 시 바로 확인할 것
- priority 값은 바뀌었는데 양보가 일어나지 않는지
- donation 상태에서 base/effective 구분이 섞여 있는지

## 4. 재검증
- `priority-change` 단일 실행 후 통과 확인
