# 03 — `*-bad-ptr` syscall 포인터 검증

## 1. 이 테스트가 검증하는 것
`create`, `open`, `exec`, `read`, `write` 등 syscall 인자로 잘못된 사용자 포인터가 들어왔을 때 syscall 구현이 이를 차단하는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### syscall 인자 검증 helper
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: syscall 인자를 읽기 전에 사용자 주소를 검증한다.
- 규칙 2: NULL, 커널 주소, unmapped 주소를 실패 처리한다.

### 문자열/버퍼 복사 helper
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: 문자열은 NUL 종료까지 각 주소를 검증한다.
- 규칙 2: 버퍼는 `[buffer, buffer + size)` 전체 범위를 검증한다.

## 3. 실패 시 바로 확인할 것
- `open-bad-ptr`, `create-bad-ptr`, `exec-bad-ptr`에서 문자열 시작 주소만 검사하는지 확인
- `read-bad-ptr`, `write-bad-ptr`에서 buffer 전체 범위를 검사하는지 확인
- syscall handler가 `f->rsp`나 인자 주소를 검증 없이 읽는지 확인

## 4. 재검증
- `create-bad-ptr`, `open-bad-ptr`, `exec-bad-ptr` 단일 실행 통과 확인
- `read-bad-ptr`, `write-bad-ptr` 단일 실행 통과 확인
