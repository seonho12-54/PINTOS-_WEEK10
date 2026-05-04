# Argument Passing 테스트 노트 템플릿/양식/생성 가이드

이 문서는 Argument Passing 학습 문서를 새로 만들거나 확장할 때,
같은 형식으로 빠르게 작성하기 위한 통합 가이드입니다.

---

## 1. 폴더 구조 기준

- 기능 문서: `argument_passing/1. feature`
- 테스트 문서: `argument_passing/2. testing`
- 면접 대비: `argument_passing/3. interview`
- 공용 템플릿: `../../1. note-templates`

---

## 2. 현재 표준 문서 세트

### feature 문서
- `01-feature-overview-argument-flow.md`
- `02-feature-parse-command-line.md`
- `03-feature-build-user-stack.md`
- `04-feature-register-setup-and-entry.md`

### testing 문서
- `00-README.md`
- `01-args-none.md`
- `02-args-single.md`
- `03-args-multiple.md`
- `04-args-many.md`
- `05-args-dbl-space.md`

### interview 문서
- `00-README.md`
- `00-interview-script.md`

---

## 3. 템플릿 사용 규칙

### 3.1 feature 문서 템플릿
- 기준: `../../1. note-templates/TEMPLATE-feature-note.md`
- 용도: 기능 단위 학습/구현 문서 작성
- 필수 섹션:
  - `1. 구현 목적 및 필요성`
  - `2. 가능한 구현 방식 비교`
  - `3. 시퀀스와 단계별 흐름`
  - `4. 구현 주석`
  - `5. 테스팅 방법`

### 3.2 testing 문서 템플릿
- 기준: `../../1. note-templates/TEMPLATE-testing-note.md`
- 용도: 테스트 하나 기준 검증 문서 작성
- 필수 섹션:
  - `1. 이 테스트가 검증하는 것`
  - `2. 구현 필요 함수와 규칙`
  - `3. 실패 시 바로 확인할 것`
  - `4. 재검증`

### 3.3 interview 문서 용도
- 기준: `argument_passing/3. interview/00-interview-script.md`
- 용도: 구현 경험을 면접에서 설명하기 위한 대본
- 포함 내용:
  - Q&A 형식의 10가지 핵심 질문
  - 각 답변별 예상 시간 (30초 ~ 3분)
  - 구체적 예시와 스택 레이아웃
  - 자신의 버그/어려움 경험담
  - 면접 팁과 심화 질문 대비

---

## 4. 구현 주석 작성 양식 (핵심 규칙)

구현 주석은 "바로 코드에 반영 가능한 정보"만 남깁니다.

- 기본 형식:
  - `위치: <파일 경로>`
  - `역할: <이 함수가 하는 일>`
  - `규칙 1, 규칙 2...: <구현 조건>`

- 포함 기준:
  - 이번 단계에서 실제로 수정/추가할 함수만 포함
  - 기존 제공 함수 설명만 필요한 경우, 구현 주석 대신 범위 경계 메모로 처리

- 제외 기준:
  - 개념 설명만 있는 문장
  - 구현 위치/조건이 없는 추상 규칙

---

## 5. 문서 생성 절차 (권장 워크플로우)

1. **문서 범위 확정**
   - feature인지 testing인지 먼저 결정
2. **템플릿 복사**
   - 해당 템플릿을 새 파일 이름으로 복사
3. **구현 범위 기준으로 섹션 채우기**
   - 구현 주석은 파일/함수/조건까지 명시
4. **학습 순서/링크 동기화**
   - `2. testing/00-README.md`
   - `1. feature/01-feature-overview-argument-flow.md`
5. **테스트 결과 반영**
   - 관련 `args-*` 결과를 `5. 테스팅 방법` 또는 testing 문서에 반영

---

## 6. 네이밍 규칙

- feature: `NN-feature-<topic>.md`
- testing: `NN-<test-name>.md`
- 번호는 학습 순서 기준으로 증가
- 파일 이동/삭제 시 `00-README`와 `01` 학습 순서를 함께 수정

---

## 7. 빠른 체크리스트

- [ ] 문서가 feature/testing 중 하나의 목적에만 집중하는가?
- [ ] 구현 주석에 위치/역할/규칙이 모두 있는가?
- [ ] 구현 범위 밖 항목이 들어가 있지 않은가?
- [ ] `00-README`와 `01` 학습 순서 링크가 최신인가?
- [ ] 관련 `args-*` 테스트와 설명이 일치하는가?
