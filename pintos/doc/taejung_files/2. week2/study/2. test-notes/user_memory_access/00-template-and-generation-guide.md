# User Memory Access 테스트 노트 템플릿/양식/생성 가이드

이 문서는 User Memory Access 학습 문서를 새로 만들거나 확장할 때,
같은 형식으로 빠르게 작성하기 위한 통합 가이드입니다.

---

## 1. 폴더 구조 기준

- 기능 문서: `user_memory_access/1. feature`
- 테스트 문서: `user_memory_access/2. testing`
- 공용 템플릿: `../../0. references/templates/test-notes`

---

## 2. 현재 표준 문서 세트

### feature 문서
- `01-feature-overview-user-memory-access.md`
- `02-feature-user-pointer-validation.md`
- `03-feature-safe-copy-in-out.md`
- `04-feature-page-fault-and-process-kill.md`

### testing 문서
- `00-README.md`
- `01-bad-read-write.md`
- `02-bad-jump.md`
- `03-syscall-bad-ptr.md`
- `04-boundary-copy.md`
- `05-null-and-empty-pointer.md`

---

## 3. 템플릿 사용 규칙

### 3.1 feature 문서 템플릿
- 기준: `../../0. references/templates/test-notes/TEMPLATE-feature-deep-dive.md`
- 용도: 기능 단위 학습/구현 문서 작성
- 기능별 가이드는 `개념 설명 → 시퀀스 및 흐름 → 구현 주석 (보면 되는 함수/구조체)` 순서로 작성

### 3.2 testing 문서 템플릿
- 기준: `../../0. references/templates/test-notes/TEMPLATE-testing-note.md`
- 용도: 테스트 하나 또는 테스트 묶음 기준 검증 문서 작성

---

## 4. 구현 주석 작성 양식 (핵심 규칙)

구현 주석은 "바로 코드에 반영 가능한 정보"만 남깁니다.

- 기본 형식:
  - `위치: <파일 경로>`
  - `역할: <이 함수가 하는 일>`
  - `규칙 1, 규칙 2...: <구현 조건>`

- 포함 기준:
  - 사용자 포인터를 직접 역참조하는 경로
  - syscall 인자, 문자열, 버퍼 검증 경로
  - 잘못된 접근 시 현재 프로세스만 종료해야 하는 경계

- 제외 기준:
  - syscall 본래 기능 설명만 있고 사용자 메모리 검증과 무관한 내용
  - 파일 디스크립터 정책처럼 별도 주제에서 다룰 내용

---

## 5. 문서 생성 절차 (권장 워크플로우)

1. **문서 범위 확정**
   - feature인지 testing인지 먼저 결정
2. **템플릿 복사**
   - 공용 템플릿을 새 파일 이름으로 복사
3. **사용자 주소 경계 기준으로 섹션 채우기**
   - 포인터 범위, 페이지 매핑, 복사 길이, 실패 처리까지 명시
4. **테스트 링크 동기화**
   - `2. testing/00-README.md`
   - `1. feature/01-feature-overview-user-memory-access.md`
5. **테스트 결과 반영**
   - 관련 `bad-*`, `*-bad-ptr`, `*-boundary` 결과를 테스팅 문서에 반영

---

## 6. 네이밍 규칙

- feature: `NN-feature-<topic>.md`
- testing: `NN-<test-group>.md`
- 번호는 학습 순서 기준으로 증가
- 파일 이동/삭제 시 `00-README`와 `01` 학습 순서를 함께 수정

---

## 7. 빠른 체크리스트

- [ ] 사용자 주소와 커널 주소 경계를 분리했는가?
- [ ] 문자열은 NUL 종료까지 안전하게 읽는가?
- [ ] 버퍼는 길이 전체가 사용자 영역인지 확인하는가?
- [ ] page boundary를 가로지르는 버퍼를 처리하는가?
- [ ] 실패 시 커널 panic이 아니라 현재 프로세스 종료로 이어지는가?
