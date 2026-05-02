# User Memory Access 테스트 노트 인덱스

이 폴더는 User Memory Access 구현 후 확인해야 할 robustness 테스트를 묶어 정리합니다.

## 테스트 문서 목록

1. `01-bad-read-write.md` — 사용자 프로그램의 invalid read/write 직접 접근
2. `02-bad-jump.md` — invalid instruction pointer 또는 jump 대상 처리
3. `03-syscall-bad-ptr.md` — syscall 인자로 들어온 bad pointer 처리
4. `04-boundary-copy.md` — page boundary를 가로지르는 문자열/버퍼 복사
5. `05-null-and-empty-pointer.md` — NULL 포인터와 빈 문자열 경계

## 관련 feature 문서

- `../1. feature/01-feature-overview-user-memory-access.md`
- `../1. feature/02-feature-user-pointer-validation.md`
- `../1. feature/03-feature-safe-copy-in-out.md`
- `../1. feature/04-feature-page-fault-and-process-kill.md`
- `../1. feature/05-feature-user-memory-helper-contract.md`

## 우선 점검 순서

1. bad pointer가 kernel panic이 아니라 `exit(-1)`로 정리되는지 확인한다.
2. syscall 인자 읽기 전 사용자 주소 검증이 일어나는지 확인한다.
3. 문자열/버퍼가 page boundary를 넘어갈 때 전체 범위가 검증되는지 확인한다.
4. 실패 경로에서 파일 락이나 임시 자원이 남지 않는지 확인한다.

## 분업 기준

System Calls 담당자와 함께 작업할 때는 `../../01-team-split-system-call-user-memory.md`의 User Memory Access 계약을 먼저 확정한다.  
`*-bad-ptr`, `*-boundary`, `bad-read/write/jump`는 syscall별 임시 처리 대신 공용 user memory helper를 통해 해결한다.
