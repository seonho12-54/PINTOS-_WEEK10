# System Calls 테스트 노트 인덱스

이 폴더는 System Calls 구현 후 확인해야 할 userprog 테스트를 기능별로 묶어 정리합니다.

## 테스트 문서 목록

1. `01-halt-exit.md` — 최소 syscall 진입과 종료 상태
2. `02-create-open-close.md` — 파일 생성/열기/닫기와 fd 수명
3. `03-read-write.md` — 읽기/쓰기, stdin/stdout, 반환 바이트 수
4. `04-exec-wait.md` — 프로세스 실행/대기/종료 상태 전달
5. `05-multi-and-rox.md` — 다중 프로세스 fd와 실행 파일 쓰기 금지

## 관련 feature 문서

- `../1. feature/01-feature-overview-system-call-flow.md`
- `../1. feature/02-feature-syscall-dispatch-and-args.md`
- `../1. feature/03-feature-process-syscalls.md`
- `../1. feature/04-feature-file-syscalls-and-fd-table.md`
- `../1. feature/05-feature-executable-write-deny.md`

## User Memory Access와의 경계

`*-bad-ptr`, `*-boundary`, `bad-read/write/jump`는 User Memory Access 트랙에서 주로 검증합니다.  
System Calls 테스트 문서는 검증된 인자를 받은 뒤의 syscall 정책, fd table, 반환값, 동기화에 집중합니다.

## 우선 점검 순서

1. `halt`, `exit`로 syscall 진입과 반환/종료 경로를 확인한다.
2. `create/open/close`로 fd table 기본 수명을 확인한다.
3. `read/write`로 fd별 분기와 반환값을 확인한다.
4. `exec/wait`로 부모-자식 동기화와 exit status를 확인한다.
5. `multi-*`, `rox-*`로 자원 수명과 실행 파일 보호를 확인한다.
