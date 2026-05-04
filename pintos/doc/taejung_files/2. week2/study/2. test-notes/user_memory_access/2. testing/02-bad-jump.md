# 02 — `bad-jump` 검증

## 1. 이 테스트가 검증하는 것
사용자 프로그램이 잘못된 instruction pointer 또는 jump 대상에 접근할 때 커널이 아니라 해당 프로세스만 종료되는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `page_fault()`
- 위치: `pintos/userprog/exception.c`
- 규칙 1: 사용자 모드에서 발생한 invalid instruction fetch를 프로세스 종료로 처리한다.
- 규칙 2: fault 원인이 사용자 주소 접근 실패인지 확인한다.

### 프로세스 종료 경로
- 위치: `pintos/userprog/syscall.c` 또는 `pintos/userprog/process.c`
- 규칙 1: 비정상 사용자 접근은 exit status `-1`로 관측되어야 한다.
- 규칙 2: 종료 중 부모/자식 상태 정리가 깨지지 않아야 한다.

## 3. 실패 시 바로 확인할 것
- invalid jump가 kernel panic으로 이어지는지 확인
- `page_fault()`에서 user/kernel fault 구분이 반대로 되어 있지 않은지 확인
- 비정상 종료 상태가 `wait-killed`류 테스트와 충돌하지 않는지 확인

## 4. 재검증
- `bad-jump`, `bad-jump2` 단일 실행 통과 확인
- 필요 시 `wait-killed`와 함께 재실행
