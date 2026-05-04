# Denying Writes to Executables 면접 대본

## Q1. 왜 실행 파일 쓰기를 막아야 하나요?
실행 중인 코드가 디스크에서 바뀌면 이후 읽히는 코드나 데이터가 달라질 수 있습니다. 그래서 실행 중인 executable file에는 쓰기를 막아 일관성을 유지합니다.

## Q2. 어떻게 구현하나요?
`load()`에서 실행 파일을 `filesys_open()`으로 연 뒤 `file_deny_write()`를 호출합니다. 중요한 점은 그 file 객체를 바로 닫지 않고 current thread에 저장해 프로세스 생존 동안 유지하는 것입니다.

## Q3. 언제 다시 쓰기를 허용하나요?
프로세스 종료 시 `process_exit()` 또는 cleanup 경로에서 저장해 둔 executable file에 `file_allow_write()`를 호출하고, 이어서 `file_close()`를 호출합니다.

## Q4. fork에서는 무엇을 조심해야 하나요?
부모와 자식이 같은 file pointer를 얕은 복사하면 cleanup에서 중복 close나 allow-write 타이밍 문제가 생길 수 있습니다. 필요한 경우 `file_duplicate()`로 자식의 file 객체를 분리해야 합니다.

