# 테스트 노트 템플릿/양식/생성 가이드

이 문서는 feature/testing 문서를 일정한 형식으로 빠르게 작성하기 위한 공통 가이드입니다.

---

## 1. 폴더 구조 기준

- 기능 문서: `<track>/1. feature`
- 테스트 문서: `<track>/2. testing`
- 공통 템플릿: `taejung_files/0. references/templates/test-notes`

---

## 2. 템플릿 사용 규칙

### 2.1 overview 문서
- 템플릿: `TEMPLATE-feature-overview.md`
- 용도: 큰 그림/학습 순서/실수 포인트 정리

### 2.2 deep-dive 문서
- 템플릿: `TEMPLATE-feature-deep-dive.md`
- 용도: 구현 세부, 함수별 규칙, 체크 순서

### 2.3 testing 문서
- 템플릿: `TEMPLATE-testing-note.md`
- 용도: 테스트 단위 재현/점검 체크리스트

---

## 3. 네이밍 규칙

- feature overview: `01-feature-overview-<topic>.md`
- feature deep-dive: `NN-feature-<topic>.md`
- testing: `NN-<test-name>.md`
- 번호는 학습 순서 기준으로 증가

---

## 4. 문서 생성 절차 (권장)

1. 작성 대상이 overview / deep-dive / testing 중 무엇인지 확정
2. 해당 템플릿 복사
3. 기능별 가이드는 `개념 설명 → 시퀀스 및 흐름 → 구현 주석 (보면 되는 함수/구조체)` 순서로 채움
4. 위치/역할/규칙/금지/체크 순서를 실제 코드 기준으로 채움
5. `2. testing/00-README.md`와 feature 링크를 동기화
