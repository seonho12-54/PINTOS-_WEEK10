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
- `05-feature-user-memory-helper-contract.md`

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
- 5장의 구현 주석은 파일 위치 나열이 아니라 `setup_stack()` 기본 매핑처럼 함수명과 구체 역할 기준으로 작성

### 3.2 testing 문서 템플릿
- 기준: `../../0. references/templates/test-notes/TEMPLATE-testing-note.md`
- 용도: 테스트 하나 또는 테스트 묶음 기준 검증 문서 작성

---

## 4. 구현 주석 작성 양식 (핵심 규칙)

구현 주석은 "바로 코드에 반영 가능한 정보"만 남깁니다.

- 기본 형식:
  - `### 5.N \`함수명()\` 구체 역할`
  - `위치: <파일 경로>`
  - `역할: <이 함수가 맡는 책임>`
  - `규칙 1, 규칙 2...: <구현 조건>`
  - `금지 1: <하면 안 되는 구현>`

- 포함 기준:
  - 사용자 포인터를 직접 역참조하는 경로
  - syscall 인자, 문자열, 버퍼 검증 경로
  - 잘못된 접근 시 현재 프로세스만 종료해야 하는 경계
  - 다른 담당자가 호출해야 하는 helper의 호출 위치와 호출 순서

- 제외 기준:
  - syscall 본래 기능 설명만 있고 사용자 메모리 검증과 무관한 내용
  - 파일 디스크립터 정책처럼 별도 주제에서 다룰 내용
  - 팀 계약 문서에서 다룬 helper의 내부 구현 본문

---

## 5. 함수 기준 구현 주석 예시

```md
### 5.1 `validate_user_buffer()` 버퍼 범위 검증
- 위치: `pintos/userprog/syscall.c`
- 역할: 사용자 버퍼의 전체 `[buffer, buffer + size)` 범위를 검증한다.
- 규칙 1: `size == 0`은 빈 범위로 처리한다.
- 규칙 2: 시작 주소와 마지막 바이트 주소를 모두 고려한다.
- 규칙 3: page boundary를 넘으면 각 페이지 매핑을 확인한다.
- 금지 1: 시작 주소 하나만 보고 전체 버퍼를 통과시키지 않는다.

구현 체크 순서:
1. size가 0인지 먼저 분기한다.
2. 시작 주소가 사용자 주소인지 확인한다.
3. 마지막 바이트까지 범위를 순회하거나 페이지 단위로 검사한다.
4. 검증된 버퍼만 `read()`/`write()` 로직으로 넘긴다.
```

함수명 뒤에는 `기본 매핑`, `인자 배치 루프`, `버퍼 범위 검증`, `실패 종료 경로`처럼 해당 함수가 문서에서 맡는 역할을 붙입니다.

## 6. 문서 생성 절차 (권장 워크플로우)

1. **문서 범위 확정**
   - feature인지 testing인지 먼저 결정
2. **템플릿 복사**
   - 공용 템플릿을 새 파일 이름으로 복사
3. **사용자 주소 경계 기준으로 섹션 채우기**
   - 포인터 범위, 페이지 매핑, 복사 길이, 실패 처리까지 명시
4. **함수 기준 구현 주석 작성**
   - `05-feature-user-memory-helper-contract.md`의 선언과 호출 계약을 기준으로 함수명을 맞춘다.
   - 구현 본문은 넣지 않고 역할, 규칙, 호출 위치, 금지 사항을 적는다.
5. **테스트 링크 동기화**
   - `2. testing/00-README.md`
   - `1. feature/01-feature-overview-user-memory-access.md`
6. **테스트 결과 반영**
   - 관련 `bad-*`, `*-bad-ptr`, `*-boundary` 결과를 테스팅 문서에 반영

---

## 7. 네이밍 규칙

- feature: `NN-feature-<topic>.md`
- testing: `NN-<test-group>.md`
- 번호는 학습 순서 기준으로 증가
- 파일 이동/삭제 시 `00-README`와 `01` 학습 순서를 함께 수정

---

## 8. 빠른 체크리스트

- [ ] 사용자 주소와 커널 주소 경계를 분리했는가?
- [ ] 문자열은 NUL 종료까지 안전하게 읽는가?
- [ ] 버퍼는 길이 전체가 사용자 영역인지 확인하는가?
- [ ] page boundary를 가로지르는 버퍼를 처리하는가?
- [ ] 실패 시 커널 panic이 아니라 현재 프로세스 종료로 이어지는가?
- [ ] 구현 주석이 함수명 기준으로 작성되어 있는가?
- [ ] helper 내부 구현 본문이 아니라 선언/호출 계약 중심으로 작성되어 있는가?
