# 03 — `priority-fifo` 검증

## 1. 이 테스트가 검증하는 것
동일 priority 스레드들 사이에서 실행 순서가 일관되게 유지되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `cmp_priority()`
- 위치: `pintos/threads/thread.c`
- 규칙 1: 기본 비교 기준은 priority이며, 동점 정책(FIFO 유지)을 코드와 일치시킨다.

### `thread_unblock()`, `thread_yield()`
- 위치: `pintos/threads/thread.c`
- 규칙 1: 두 경로 모두 동일한 정렬/삽입 정책을 사용한다.

## 3. 실패 시 바로 확인할 것
- 동점 스레드가 역순으로 실행되는지
- 삽입 경로마다 정책이 달라지는지

## 4. 재검증
- `priority-fifo` 단일 실행 후 통과 확인
