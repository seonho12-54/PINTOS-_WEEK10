# 2. testing — Denying Writes to Executables

이 폴더는 실행 파일 write deny 요구사항을 rox 계열 테스트 관점에서 확인합니다.

## 관련 feature 문서
- `../1. feature/01-feature-deny-writes-to-executables.md`

## 우선순위
1. 실행 파일 open 후 deny-write 유지
2. write syscall의 file_write 반환값 반영
3. process exit cleanup에서 allow-write/close
4. fork/exec 경계에서 file 객체 소유권 유지

