# User Programs 테스트 노트 가이드

이 폴더는 `User Programs` 주차 문서를 **기능 이해 -> 구현 -> 테스트 검증** 흐름으로 정리한 진입 가이드입니다.

## 트랙 구성
- Argument Passing 트랙: `argument_passing`
- User Memory Access 트랙: `user_memory_access`
- System Calls 트랙: `system_calls`

## 공통 템플릿/생성 규칙
- 통합 템플릿 위치: `../../0. references/templates/test-notes`
- 트랙별 생성 가이드:
  - `argument_passing/00-template-and-generation-guide.md`
  - `user_memory_access/00-template-and-generation-guide.md`
  - `system_calls/00-template-and-generation-guide.md`

## 권장 학습 순서
1. Argument Passing 트랙
   - `argument_passing/1. feature/01-feature-overview-argument-flow.md`
   - `argument_passing/1. feature/02-feature-parse-command-line.md`
   - `argument_passing/1. feature/03-feature-build-user-stack.md`
   - `argument_passing/1. feature/04-feature-register-setup-and-entry.md`
   - `argument_passing/2. testing/00-README.md`
   - `argument_passing/2. testing/01-args-none.md` ~ `05-args-dbl-space.md`
2. User Memory Access 트랙
   - `user_memory_access/1. feature/01-feature-overview-user-memory-access.md`
   - `user_memory_access/1. feature/02-feature-user-pointer-validation.md`
   - `user_memory_access/1. feature/03-feature-safe-copy-in-out.md`
   - `user_memory_access/1. feature/04-feature-page-fault-and-process-kill.md`
   - `user_memory_access/2. testing/00-README.md`
   - `user_memory_access/2. testing/01-bad-read-write.md` ~ `05-null-and-empty-pointer.md`
3. System Calls 트랙
   - `system_calls/1. feature/01-feature-overview-system-call-flow.md`
   - `system_calls/1. feature/02-feature-syscall-dispatch-and-args.md`
   - `system_calls/1. feature/03-feature-process-syscalls.md`
   - `system_calls/1. feature/04-feature-file-syscalls-and-fd-table.md`
   - `system_calls/1. feature/05-feature-executable-write-deny.md`
   - `system_calls/2. testing/00-README.md`
   - `system_calls/2. testing/01-halt-exit.md` ~ `05-multi-and-rox.md`

## 폴더 역할
- `argument_passing`: 인자 전달 feature/testing 문서 세트
- `user_memory_access`: 사용자 포인터 검증과 안전 복사 feature/testing 문서 세트
- `system_calls`: syscall dispatch, 프로세스/파일 syscall, fd table 문서 세트
- `../../0. references/templates/test-notes/*`: 공통 템플릿 세트
