# System Calls 테스트 노트 템플릿/양식/생성 가이드

이 문서는 System Calls 학습 문서를 새로 만들거나 확장할 때,
같은 형식으로 빠르게 작성하기 위한 통합 가이드입니다.

---

## 1. 폴더 구조 기준

- 기능 문서: `system_calls/1. feature`
- 테스트 문서: `system_calls/2. testing`
- 공용 템플릿: `../../0. references/templates/test-notes`

---

## 2. 현재 표준 문서 세트

### feature 문서
- `01-feature-overview-system-call-flow.md`
- `02-feature-syscall-dispatch-and-args.md`
- `03-feature-process-syscalls.md`
- `04-feature-file-syscalls-and-fd-table.md`
- `05-feature-executable-write-deny.md`

### testing 문서
- `00-README.md`
- `01-halt-exit.md`
- `02-create-open-close.md`
- `03-read-write.md`
- `04-exec-wait.md`
- `05-multi-and-rox.md`

---

## 3. 템플릿 사용 규칙

### 3.1 feature 문서 템플릿
- 기준: `../../0. references/templates/test-notes/TEMPLATE-feature-deep-dive.md`
- 용도: syscall 기능 단위 학습/구현 문서 작성
- 기능별 가이드는 `개념 설명 → 시퀀스 및 흐름 → 구현 주석 (보면 되는 함수/구조체)` 순서로 작성

### 3.2 testing 문서 템플릿
- 기준: `../../0. references/templates/test-notes/TEMPLATE-testing-note.md`
- 용도: syscall 테스트 묶음 기준 검증 문서 작성

---

## 4. User Memory Access와의 경계

System Calls 문서는 **검증된 인자를 받은 뒤 syscall 본래 정책을 수행하는 부분**에 집중합니다.

- User Memory Access에서 다룰 것:
  - bad pointer 검증
  - 문자열/버퍼 안전 복사
  - boundary를 넘는 사용자 메모리 접근

- System Calls에서 다룰 것:
  - syscall number dispatch
  - 반환값과 exit status 정책
  - fd table과 file 객체 수명
  - process syscall 동기화
  - 실행 중인 파일 쓰기 금지

---

## 5. 문서 생성 절차 (권장 워크플로우)

1. **문서 범위 확정**
   - dispatcher/process/file/fd/rox 중 무엇인지 먼저 결정
2. **템플릿 복사**
   - 공용 템플릿을 새 파일 이름으로 복사
3. **syscall 정책 기준으로 섹션 채우기**
   - 인자 검증 자체는 User Memory Access로 링크하고 중복 설명하지 않음
4. **테스트 링크 동기화**
   - `2. testing/00-README.md`
   - `1. feature/01-feature-overview-system-call-flow.md`
5. **테스트 결과 반영**
   - 관련 `halt`, `exit`, `open/read/write`, `exec/wait`, `rox-*` 결과를 테스팅 문서에 반영

---

## 6. 빠른 체크리스트

- [ ] syscall number dispatch가 안전한가?
- [ ] syscall별 반환값과 실패 정책이 테스트 기대와 맞는가?
- [ ] fd table이 프로세스별로 관리되는가?
- [ ] `read`/`write`가 stdin/stdout/file을 구분하는가?
- [ ] `exec`/`wait` 동기화와 exit status 전달이 일관적인가?
- [ ] 실행 중인 파일에 deny-write가 적용되는가?
