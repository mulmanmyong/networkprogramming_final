# Network Programming Final

Windows Winsock 기반의 TCP 다중 클라이언트 채팅 프로그램입니다.

## 주요 기능

- 서버-클라이언트 구조의 콘솔 채팅
- 클라이언트 닉네임 등록 및 중복 검사
- 다중 사용자 메시지 브로드캐스트
- `/member` 참여자 목록 조회
- `/exit` 클라이언트 접속 종료
- 서버 관리자 모드 제공
  - `/monitoron`, `/monitoroff`: 채팅 모니터링 전환
  - `/nop`: 최대 접속 인원 변경
  - `/changepw`: 관리자 비밀번호 변경
  - `/shutdown`: 접속자가 없을 때 서버 종료

## 파일 구성

- `Common.h`: Winsock 공통 헤더 및 오류 처리 함수
- `FinalProjectServer.cpp`: TCP 채팅 서버 및 관리자 기능
- `FinalProjectClient.cpp`: TCP 채팅 클라이언트

## 실행 방식

```bash
FinalProjectServer.exe <SERVERPORT>
FinalProjectClient.exe <SERVERIP> <SERVERPORT>
```

Windows 환경과 Winsock2 라이브러리를 기준으로 작성된 프로젝트입니다.
