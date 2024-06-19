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

// ������ ��ſ� ����� ����
char buf[BUFSIZE + 1];
int len;
int keepRunning = 1;  // ���� �����尡 ���� ������ ���θ� ��Ÿ���� �÷���

DWORD WINAPI ReceiveThread(LPVOID arg);

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Usage : %s <SERVERIP> <SERVERPORT>\n", argv[0]);
        exit(1);
    }

    SERVERIP = argv[1];
    SERVERPORT = atoi(argv[2]);

    // Ŭ���̾�Ʈ �ʱ�ȭ
    client_init();

    // �̸� �Է� �� ���� 
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


    // ������ ������ �Ǿ����� ä���ϸ� �ȴٴ� �޽����� �������� �޾ƿ� ��� (���� ���� ���� �ο� �� �� �Բ�)
    retval = recv(sock, buf, BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        err_display("recv()");
        return -1;
    }
    else if (retval == 0)
        return -1;

    // ���� ������ ���
    buf[retval] = '\0';
    printf("%s\n\n", buf);

    // ���� ������ ����
    HANDLE hThread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)sock, 0, NULL);
    if (hThread == NULL) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    // ������ ������ ���
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
            keepRunning = 0;  // ���� ������ ���� �÷��� ����
            break;
        }
    }

    // ���� �����尡 ����� ������ ��ٸ�
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);

    // ���� �ݱ�
    closesocket(sock);

    // ���� ����
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
            if (keepRunning)  // ������ ���Ḹ ǥ��
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
