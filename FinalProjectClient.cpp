#include "C:\Users\zeno\source\repos\final\Common.h"

#define BUFSIZE 512

int client_init();
int send_name();
int retval;
const char* SERVERIP;
int SERVERPORT;
WSADATA wsa;
SOCKET sock;
struct sockaddr_in serveraddr;

// 데이터 통신에 사용할 변수
char buf[BUFSIZE + 1];
int len;
int keepRunning = 1;  // 수신 스레드가 실행 중인지 여부를 나타내는 플래그

DWORD WINAPI ReceiveThread(LPVOID arg);

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Usage : %s <SERVERIP> <SERVERPORT>\n", argv[0]);
        exit(1);
    }

    SERVERIP = argv[1];
    SERVERPORT = atoi(argv[2]);

    // 클라이언트 초기화
    client_init();

    // 이름 입력 후 전송 
    while (1) {
        send_name();

        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            return -1;
        }
        else if (retval == 0)
            return -1;

        buf[retval] = '\0';
        printf("%s", buf);
        if (!strcmp(buf, "FAIL")) {
            printf(": Duplicate nicknames are not allowed\n");
        }
        else {
            printf("\n");
            break;
        }

    }


    // 서버와 연결이 되었으니 채팅하면 된다는 메시지를 서버에서 받아와 출력 (현재 참여 중인 인원 수 와 함께)
    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        err_display("recv()");
        return -1;
    }
    else if (retval == 0)
        return -1;

    // 받은 데이터 출력
    buf[retval] = '\0';
    printf("%s\n\n", buf);

    // 수신 스레드 생성
    HANDLE hThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)sock, 0, NULL);
    if (hThread == NULL) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // 서버와 데이터 통신
    while (keepRunning) {
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        retval = send(sock, buf, (int)strlen(buf), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

        if (strcmp(buf, "/exit") == 0) {
            keepRunning = 0;  // 수신 스레드 종료 플래그 설정
            break;
        }
    }

    // 수신 스레드가 종료될 때까지 기다림
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // 소켓 닫기
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}

int client_init() {
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    return 0;
}

int send_name() {
    printf("\n[Input your name] ");
    if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
        return -1;

    len = (int)strlen(buf);
    if (buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    if (strlen(buf) == 0)
        return -2;

    retval = send(sock, buf, (int)strlen(buf), 0);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
        return -3;
    }
    return 0;
}

DWORD WINAPI ReceiveThread(LPVOID arg) {
    SOCKET sock = (SOCKET)arg;
    char buf[BUFSIZE + 1];
    int retval;

    while (keepRunning) {
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            if (keepRunning)  // 비정상 종료만 표시
                err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        buf[retval] = '\0';
        printf("%s\n", buf);
    }

    return 0;
}
