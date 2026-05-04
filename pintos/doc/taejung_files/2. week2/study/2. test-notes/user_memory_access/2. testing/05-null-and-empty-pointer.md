# 05 — NULL 포인터와 빈 문자열 경계 검증

## 1. 이 테스트가 검증하는 것
syscall 인자로 NULL 포인터나 빈 문자열이 들어올 때, 잘못된 포인터는 종료하고 정책상 허용/거부해야 하는 빈 문자열은 일관되게 처리하는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### 사용자 포인터 검증 helper
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: NULL 포인터는 user memory access 실패로 처리한다.
- 규칙 2: NULL을 문자열 helper에 넘겨 직접 읽지 않는다.

### 문자열 syscall 처리
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: `open-null`, `create-null`은 NULL 포인터를 실패 처리한다.
- 규칙 2: `open-empty`처럼 빈 문자열은 syscall 정책에 맞게 실패하되 커널 panic 없이 처리한다.

## 3. 실패 시 바로 확인할 것
- NULL 포인터를 `strlen`, `strcmp`, `filesys_open` 등에 그대로 넘기는지 확인
- 빈 문자열과 NULL 포인터를 같은 케이스로 섞어 처리하는지 확인
- 실패 반환과 프로세스 종료 정책이 테스트 기대와 맞는지 확인

## 4. 재검증
- `create-null`, `open-null` 단일 실행 통과 확인
- `open-empty`, `create-empty` 재실행
- 필요 시 `open-bad-ptr`와 함께 포인터 검증 경계 확인
