# 05 — `priority-donate-*` 검증

## 1. 이 테스트가 검증하는 것
락 대기 상황에서 donation이 적용/전파/회수되어 priority inversion이 완화되는지 검증합니다.

대상 테스트:
- `priority-donate-one`
- `priority-donate-multiple`, `priority-donate-multiple2`
- `priority-donate-nest`, `priority-donate-chain`
- `priority-donate-sema`, `priority-donate-lower`

## 2. 구현 필요 함수와 규칙
### `lock_acquire()`
- 위치: `pintos/threads/synch.c`
- 규칙 1: lock owner에게 donation을 전달한다.
- 규칙 2: 체인 대기에서는 상위 owner까지 전파한다.

### `lock_release()`
- 위치: `pintos/threads/synch.c`
- 규칙 1: 해제한 lock 관련 donation을 회수하고 effective priority를 재계산한다.

### `thread_set_priority()`
- 위치: `pintos/threads/thread.c`
- 규칙 1: base priority 변경과 donation 적용 상태를 분리해 관리한다.

## 3. 실패 시 바로 확인할 것
- donation 체인이 중간에서 끊기는지
- lock 해제 후 priority 복원이 잘못되는지
- base/effective priority가 같은 필드에 섞여 있는지

## 4. 재검증
- `priority-donate-one` -> `priority-donate-multiple` -> `priority-donate-chain` 순으로 단일 확인
- 마지막에 `priority-donate-*` 묶음 재실행
