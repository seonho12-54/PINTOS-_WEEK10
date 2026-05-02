# 01 — `bad-read` / `bad-write` 검증

## 1. 이 테스트가 검증하는 것
사용자 프로그램이 직접 잘못된 주소를 읽거나 쓸 때 커널이 panic 나지 않고, 해당 프로세스만 비정상 종료되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `page_fault()`
- 위치: `pintos/userprog/exception.c`
- 규칙 1: 사용자 모드 invalid read/write는 현재 프로세스 종료로 처리한다.
- 규칙 2: 커널 전체 panic으로 이어지지 않게 한다.

### 사용자 메모리 접근 helper
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: syscall 중 사용자 주소 접근은 helper를 통해 수행한다.
- 규칙 2: helper 실패는 `exit(-1)`과 동일한 정책으로 처리한다.

## 3. 실패 시 바로 확인할 것
- `bad-read`, `bad-write` 실행 시 kernel panic이 나는지 확인
- `page_fault()`가 user fault를 프로세스 종료로 분기하는지 확인
- 잘못된 주소를 예외 없이 직접 역참조하는 코드가 남아 있는지 확인

## 4. 재검증
- `bad-read`, `bad-write` 단일 실행 통과 확인
- 필요 시 `bad-read2`, `bad-write2` 재실행
