# Denying Writes to Executables 테스트 노트

이 폴더는 KAIST PintOS Project 2의 Denying Writes to Executables 요구사항을 기능, 테스트, 면접 설명 관점으로 정리합니다.

## 폴더 구조
- `1. feature/` - 실행 파일 write deny 기능 설명과 구현 주석
- `2. testing/` - rox 계열 테스트 체크리스트
- `3. interview/` - 면접 설명용 요약과 Q/A

## 원문 참고
- `../../0. references/kaist_docs/project2/5. Denying_Writes_to_Executables.md`

## 핵심 요구사항
- 실행 중인 프로그램의 executable file에는 쓰기를 막는다.
- `file_deny_write()`를 호출한 file 객체를 프로세스 생존 동안 닫지 않고 보관한다.
- 프로세스 종료 시 `file_allow_write()` 후 `file_close()`로 정리한다.
- fork/exec/exit 경계에서 file 객체 소유권을 꼬이게 하지 않는다.

