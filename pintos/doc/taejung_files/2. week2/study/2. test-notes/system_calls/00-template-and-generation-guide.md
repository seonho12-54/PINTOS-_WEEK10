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
- **`01-feature-overview-*.md`만 예외**: 전체 흐름 개요용이며 5장 생략 가능. 구체 구현은 `02`~`05`의 5장을 본다.
- **`02`~`05` feature 문서**: 5장에 문서 주제의 **모든 핵심 syscall 분기·helper·`process_*`·fd·load·cleanup·실패 경로**를 빠짐없이 둔다.
- 한 소기능이 여러 구현 지점으로 나뉘면 `### 5.N <기능 묶음>` 아래에 `#### 5.N.1`, `#### 5.N.2`처럼 함수/분기 단위로 쪼갠다.

### 3.2 testing 문서 템플릿
- 기준: `../../0. references/templates/test-notes/TEMPLATE-testing-note.md`
- 용도: syscall 테스트 묶음 기준 검증 문서 작성

---

## 4. 구현 주석 작성 양식 (핵심 규칙)

구현 주석은 "바로 코드에 반영 가능한 정보"만 남긴다. `user_memory_access/00-template-and-generation-guide.md` 4~5절과 동일한 밀도를 맞춘다.

- 기본 형식:
  - `### 5.N <기능/구현 묶음명>`
  - `#### 5.N.M \`함수명()\` 또는 \`SYS_*\` 분기의 구체 역할`
  - `위치: <파일 경로>`
  - `역할: <이 함수가 맡는 책임>`
  - `규칙 1, 규칙 2...: <구현 조건>`
  - `금지 1: <하면 안 되는 구현>`

- 포함 기준:
  - 이 feature 문서 범위의 syscall 분기, helper, `process_*`, fd table, `load`/실행 파일 정리 등 **한 번이라도 수정할 수 있는 진입점**
  - 성공 경로뿐 아니라 실패 정리 경로, 부모-자식 동기화 경로, 반환값 기록 지점
  - 다른 담당 문서(User Memory Access, Argument Passing)와 맞물리는 **호출 순서·전제**

- 제외 기준:
  - User Memory Access helper **내부 구현 본문** 중복
  - 문서 주제 밖 syscall의 세부 정책(해당 번호 feature 문서의 5장에 둔다)

### 4.1 함수 기준 구현 주석 예시

```md
### 5.1 `halt` syscall

#### 5.1.1 `syscall_handler()`의 `SYS_HALT` 분기
- 위치: `pintos/userprog/syscall.c`
- 역할: 사용자 halt 요청을 커널 정지 경로로 연결한다.
- 규칙 1: 반환값이 없는 syscall이므로 RAX를 사용자 결과로 쓰지 않는다.
- 규칙 2: 정지 후 이 스레드는 더 이상 사용자로 돌아가지 않는다.
- 금지 1: halt를 단순 return으로 끝내 테스트가 기대하는 정지를 건너뛰지 않는다.

구현 체크 순서:
1. syscall number 매핑에서 halt를 식별한다.
2. 문서화된 커널 정지 API를 호출한다.
3. 이후 경로가 unreachable임을 주석으로 남긴다.
```

---

## 5. User Memory Access와의 경계

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

## 6. 문서 생성 절차 (권장 워크플로우)

1. **문서 범위 확정**
   - dispatcher/process/file/fd/rox 중 무엇인지 먼저 결정
2. **템플릿 복사**
   - 공용 템플릿을 새 파일 이름으로 복사
3. **syscall 정책 기준으로 섹션 채우기**
   - 인자 검증 자체는 User Memory Access로 링크하고 중복 설명하지 않음
4. **함수 기준 구현 주석(5장) 작성**
   - 4장의 각 소기능마다 대응하는 `5.N`을 두고, 위 4절 형식을 채운다.
5. **테스트 링크 동기화**
   - `2. testing/00-README.md`
   - `1. feature/01-feature-overview-system-call-flow.md`
6. **테스트 결과 반영**
   - 관련 `halt`, `exit`, `open/read/write`, `exec/wait`, `rox-*` 결과를 테스팅 문서에 반영

---

## 7. 빠른 체크리스트

- [ ] syscall number dispatch가 안전한가?
- [ ] syscall별 반환값과 실패 정책이 테스트 기대와 맞는가?
- [ ] fd table이 프로세스별로 관리되는가?
- [ ] `read`/`write`가 stdin/stdout/file을 구분하는가?
- [ ] `exec`/`wait` 동기화와 exit status 전달이 일관적인가?
- [ ] 실행 중인 파일에 deny-write가 적용되는가?
