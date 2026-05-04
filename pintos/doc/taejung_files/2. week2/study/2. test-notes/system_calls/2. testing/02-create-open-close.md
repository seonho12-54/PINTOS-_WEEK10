# 02 — `create` / `open` / `close` 검증

## 1. 이 테스트가 검증하는 것
파일 생성, 열기, 닫기 syscall이 파일 시스템 API와 fd table을 올바르게 연결하는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### 파일 syscall 구현
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: `create`는 성공/실패를 bool로 반환한다.
- 규칙 2: `open`은 성공 시 fd, 실패 시 `-1`을 반환한다.
- 규칙 3: `close`는 유효한 fd만 닫고 fd slot을 해제한다.

### fd table helper
- 위치: `pintos/userprog/syscall.c`, `pintos/threads/thread.h`
- 규칙 1: fd 0/1을 예약하고 일반 파일 fd는 2 이상으로 할당한다.
- 규칙 2: 닫힌 fd나 잘못된 fd는 실패 처리한다.

## 3. 실패 시 바로 확인할 것
- `open` 성공 후 fd table에 file 객체가 저장되는지 확인
- `close-twice`, `close-bad-fd`에서 잘못된 fd를 무시하거나 panic 내는지 확인
- `create-empty`, `open-empty`의 실패 정책이 맞는지 확인

## 4. 재검증
- `create-normal`, `create-empty`, `create-long`, `create-exists` 실행
- `open-normal`, `open-missing`, `open-empty`, `open-twice` 실행
- `close-normal`, `close-twice`, `close-bad-fd` 실행
