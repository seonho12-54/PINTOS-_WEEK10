# 03 — `read` / `write` 검증

## 1. 이 테스트가 검증하는 것
`read`와 `write`가 stdin/stdout과 일반 파일 fd를 구분하고, 실제 처리한 바이트 수를 올바르게 반환하는지 검증합니다.

## 2. 구현 필요 함수와 규칙
### `read` syscall
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: fd 0은 stdin 입력으로 처리한다.
- 규칙 2: 일반 file fd는 `file_read()` 결과를 반환한다.
- 규칙 3: stdout 또는 잘못된 fd에서 read하면 실패 정책을 따른다.

### `write` syscall
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: fd 1은 stdout 출력으로 처리한다.
- 규칙 2: 일반 file fd는 `file_write()` 결과를 반환한다.
- 규칙 3: stdin 또는 잘못된 fd에서 write하면 실패 정책을 따른다.

## 3. 실패 시 바로 확인할 것
- fd 0/1 특수 처리가 뒤바뀌었는지 확인
- `read-zero`, `write-zero`에서 size 0 처리 반환값이 맞는지 확인
- `read-bad-fd`, `write-bad-fd`에서 fd lookup 실패 처리가 맞는지 확인

## 4. 재검증
- `read-normal`, `read-zero`, `read-stdout`, `read-bad-fd` 실행
- `write-normal`, `write-zero`, `write-stdin`, `write-bad-fd` 실행
- User Memory Access 수정 후 `read-boundary`, `write-boundary`도 함께 재확인
