# 3. interview — Denying Writes to Executables

이 폴더는 실행 파일 write deny 요구사항을 면접에서 설명하기 위한 자료입니다.

## 핵심 답변
실행 중인 executable file은 `load()`에서 연 file 객체에 `file_deny_write()`를 걸고, 프로세스가 종료될 때까지 닫지 않아서 쓰기 금지를 유지합니다. 종료 시에는 `file_allow_write()` 후 `file_close()`로 상태를 복구합니다.

## 꼭 말할 포인트
- 이름 비교가 아니라 file 계층의 deny-write API를 사용한다.
- deny-write가 유지되려면 file 객체를 닫지 않고 보관해야 한다.
- cleanup에서 allow-write와 close 순서가 중요하다.
- fork/exec에서는 file 객체 소유권을 얕은 복사하면 중복 close 문제가 생길 수 있다.

