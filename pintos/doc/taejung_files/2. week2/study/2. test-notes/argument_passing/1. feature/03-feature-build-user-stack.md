# 03 — 기능 2: 사용자 스택 레이아웃 구성 (ABI 계약)

## 1. 구현 목적 및 필요성
### 이 기능이 무엇인가
파싱된 토큰 배열을 사용자 스택에 C 호출 규약이 기대하는 구조로 배치하는 기능입니다.

### 왜 이걸 하는가 (문제 맥락)
`argv`는 값이 아니라 포인터 체인입니다. 문자열만 복사해 두면 동작하지 않고, 포인터 배열과 센티널까지 완성되어야 합니다.

### 완성의 의미 (결과 관점)
유저 진입 직후 `_start`가 `argv[0..argc-1]`를 순서대로 읽고 `argv[argc]==NULL`을 만족합니다.

## 2. 가능한 구현 방식 비교
- 방식 A: 문자열 → 정렬 → NULL → 포인터 배열 → fake return (권장)
  - 장점: 디버깅 용이, 문서/예제와 일치
- 방식 B: 포인터 테이블 선할당 후 역계산
  - 장점: 메모리 이동 감소 가능
  - 단점: 주소 계산 복잡
- 선택: A

## 3. 시퀀스와 단계별 흐름
```mermaid
sequenceDiagram
  participant SB as stack_builder
  participant US as USER_STACK
  SB->>US: 문자열들 역순 push
  SB->>US: 8-byte 정렬 보정
  SB->>US: NULL sentinel push
  SB->>US: argv 포인터들 역순 push
  SB->>US: fake return address push
  SB->>SB: argv_user_addr, rsp 확정
```

1. `rsp = USER_STACK`에서 시작한다.
2. 문자열을 뒤에서부터 복사하고 각 시작 주소를 배열에 기록한다.
3. 포인터 push 전에 8바이트 정렬을 맞춘다.
4. `NULL` 센티널을 먼저 push한다.
5. 문자열 주소들을 역순 push해 `argv[0]`이 가장 낮은 주소에 오게 한다.
6. 포인터 배열의 시작 주소를 `argv_user_addr`로 호출부에 돌려준다.
7. 가짜 return address(0)를 push한다.
8. 최종 `rsp`를 `intr_frame`에 반영한다.

## 4. 기능별 가이드 (개념/흐름 + 구현 주석 위치)
### 4.1 기능 A: 문자열 블록 배치
#### 개념 설명
스택은 높은 주소에서 낮은 주소로 자라므로, 문자열을 뒤쪽 인자부터 복사해야 최종 `argv[0]`, `argv[1]` 순서를 자연스럽게 만들 수 있습니다.

#### 시퀀스 및 흐름
```mermaid
flowchart TD
  INIT[sp = user_if->rsp] --> LOOP[argc - 1부터 0까지 반복]
  LOOP --> LEN[strlen(argv[i]) + 1 계산]
  LEN --> DEC[sp를 len만큼 감소]
  DEC --> BOUND[USER_STACK - PGSIZE 하한 검사]
  BOUND --> COPY[문자열과 NUL 복사]
  COPY --> SAVE[arg_addrs[i] = sp]
```

1. `setup_stack()`이 만든 초기 `rsp`에서 시작한다.
2. 각 문자열 길이는 끝의 `\0`까지 포함해 계산한다.
3. `sp`를 먼저 낮춘 뒤 페이지 하한을 넘지 않는지 확인한다.
4. 문자열을 사용자 스택에 복사하고 시작 주소를 `arg_addrs[]`에 저장한다.

#### 구현 주석 (보면 되는 함수)
- 위치: `pintos/userprog/process.c`의 인자 스택 구성 루틴
- 위치: `pintos/userprog/process.c`의 `build_user_stack_args()`

### 4.2 기능 B: 포인터 블록 배치
#### 개념 설명
문자열만 복사해서는 `argv`가 완성되지 않습니다. 사용자 코드가 읽을 수 있는 포인터 배열을 사용자 스택 안에 만들어야 합니다.

#### 시퀀스 및 흐름
```mermaid
sequenceDiagram
  participant SB as stack_builder
  participant US as USER_STACK
  SB->>US: sp 8-byte 정렬
  SB->>US: argv[argc] NULL push
  loop argc - 1 downto 0
    SB->>US: arg_addrs[i] push
  end
  SB->>SB: argv_user_addr = sp
```

1. 문자열 복사 후 `sp`를 8바이트 경계로 내린다.
2. `argv[argc]` 역할을 하는 NULL 포인터를 먼저 push한다.
3. `arg_addrs[]`를 역순으로 push해 최종 배열이 `argv[0]`부터 읽히게 한다.
4. 포인터 push가 끝난 현재 `sp`를 `argv_user_addr`로 저장한다.

#### 구현 주석 (보면 되는 함수)
- 위치: `pintos/userprog/process.c`
- 위치: `pintos/userprog/process.c`의 `build_user_stack_args()`

### 4.3 기능 C: 정렬/프레임 마무리
#### 개념 설명
유저 진입 시점의 스택은 ABI가 기대하는 정렬과 호출 프레임 모양을 만족해야 합니다. 포인터 배열 주소를 보존한 뒤 fake return address를 추가하고 최종 `rsp`를 반영합니다.

#### 시퀀스 및 흐름
```mermaid
flowchart LR
  AV[argv_user_addr 저장] --> FAKE[fake return address 0 push]
  FAKE --> RSP[user_if->rsp = sp]
  RSP --> DONE[스택 빌드 성공 반환]
```

1. 포인터 배열 시작 주소를 `*argv_user_addr`에 저장한다.
2. fake return address로 0을 push한다.
3. push 전마다 `USER_STACK - PGSIZE` 하한 검사를 수행한다.
4. 최종 `sp`를 `user_if->rsp`에 저장하고 성공을 반환한다.

#### 구현 주석 (보면 되는 함수)
- 위치: `pintos/userprog/process.c`의 `build_user_stack_args()`

## 5. 구현 주석 (위치별 정리)
### 5.1 `setup_stack()` 기본 매핑
- 위치: `pintos/userprog/process.c`
- 역할: 스택 페이지 매핑과 초기 `rsp` 설정
- 규칙 1: 매핑 실패 시 즉시 false
- 규칙 2: 성공 시에만 `build_user_stack_args()` 호출

구현 체크 순서:
1. 스택 페이지 매핑 성공 여부를 먼저 확인한다.
2. 성공 시 초기 `rsp = USER_STACK`으로 설정한다.
3. 이후 `build_user_stack_args()` 호출로 문자열/포인터 구성 단계에 진입한다.
4. 실패 시 즉시 false를 반환하고 후속 단계를 차단한다.

### 5.2 `build_user_stack_args()` 인자 배치 루프
- 위치: `pintos/userprog/process.c`
- 역할: 문자열/포인터 push 연산의 전담 스택 빌더
- 규칙 1: `sp`를 바이트 단위로 낮춘 뒤 해당 위치에 직접 값을 쓴다
- 규칙 2: 바이트 단위 경계 검사 누락 금지

구현 체크 순서:
1. 문자열을 역순으로 push하고 시작 주소를 `arg_addrs[]`에 기록한다.
2. 8바이트 정렬을 맞춘 뒤 `NULL` 센티널을 push한다.
3. `arg_addrs[]`를 역순 순회해 `argv` 포인터들을 push한다.
4. 포인터 배열 시작 주소를 `argv_user_addr`에 저장한다.
5. 마지막에 fake return address(0)를 push하고 `user_if->rsp`를 갱신한다.

### 5.3 `hex_dump()` 디버깅 관측점
- 위치: `pintos/userprog/process.c` (임시 로그)
- 역할: 실패 시 스택 레이아웃 확인
- 규칙 1: `hex_dump()`로 `rsp` 인근 덤프 확인
- 규칙 2: `argv_user_addr`, `argv[0]`, `argv[argc]` 주소/값 동시 점검

구현 체크 순서:
1. 실패 재현 시점의 최종 `rsp`를 먼저 기록한다.
2. `hex_dump()`로 문자열 블록/포인터 블록 위치를 확인한다.
3. `argv_user_addr`, `argv[0]`, `argv[argc]`를 함께 검증한다.
4. 정렬/센티널/포인터 순서 중 깨진 지점을 기준으로 루프를 역추적한다.

## 6. 테스팅 방법
- 기본 검증: `args-single`
- 포인터 개수/순서: `args-multiple`, `args-many`
- 공백 연계: `args-dbl-space`
- 진단: `hex_dump()`로 문자열/포인터/정렬 구간을 한 번에 확인
