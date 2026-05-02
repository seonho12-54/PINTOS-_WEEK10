# 04 — `*-boundary` 복사 경계 검증

## 1. 이 테스트가 검증하는 것
문자열이나 버퍼가 두 페이지 경계에 걸쳐 있을 때, 첫 페이지뿐 아니라 다음 페이지까지 안전하게 검증하고 복사하는지 확인합니다.

## 2. 구현 필요 함수와 규칙
### 문자열 복사 helper
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: `open-boundary`, `exec-boundary`처럼 문자열이 page boundary를 넘어도 NUL까지 읽는다.
- 규칙 2: 다음 페이지가 유효하지 않으면 프로세스 종료로 처리한다.

### 버퍼 범위 검증 helper
- 위치: `pintos/userprog/syscall.c`
- 규칙 1: `read-boundary`, `write-boundary`에서 버퍼 전체 범위를 검증한다.
- 규칙 2: page boundary를 넘는 모든 페이지의 매핑을 확인한다.

## 3. 실패 시 바로 확인할 것
- 문자열 시작 주소만 검사하고 `strlen()`을 바로 호출하는지 확인
- 버퍼 시작 주소만 검사하고 `size` 범위를 무시하는지 확인
- 마지막 바이트 주소 계산에서 overflow나 off-by-one이 있는지 확인

## 4. 재검증
- `open-boundary`, `exec-boundary` 단일 실행 통과 확인
- `read-boundary`, `write-boundary` 단일 실행 통과 확인
- 필요 시 `create-bound`, `fork-boundary`도 함께 확인
