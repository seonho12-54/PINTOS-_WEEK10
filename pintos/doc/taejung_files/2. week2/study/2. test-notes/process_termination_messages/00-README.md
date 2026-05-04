# Process Termination Messages 테스트 노트

이 폴더는 KAIST PintOS Project 2의 Process Termination Messages 요구사항을 기능, 테스트, 면접 설명 관점으로 정리합니다.

## 폴더 구조
- `1. feature/` - 종료 메시지 기능 설명과 구현 주석
- `2. testing/` - 종료 메시지 관련 테스트 체크리스트
- `3. interview/` - 면접 설명용 요약과 Q/A

## 원문 참고
- `../../0. references/kaist_docs/project2/4. Process_Termination_Messages.md`

## 핵심 요구사항
- 사용자 프로세스가 종료될 때 `<process name>: exit(<status>)` 형식으로 출력한다.
- 사용자 프로세스가 아닌 커널 스레드 종료에는 출력하지 않는다.
- `halt` syscall에는 종료 메시지를 출력하지 않는다.
- 추가 디버그 출력으로 테스트 결과를 오염시키지 않는다.

