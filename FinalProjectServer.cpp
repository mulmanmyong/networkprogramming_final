#include "C:\Users\zeno\source\repos\final\Common.h"

#define BUFSIZE    512
#define FULL       20
int SERVERPORT;

// 소켓 정보 저장을 위한 구조체와 변수
struct SOCKETINFO
{
    SOCKET sock;
    char buf[BUFSIZE + 1];
    char nickname[50];
    char ip[INET_ADDRSTRLEN];
    int port;
};
int nTotalSockets = 0;
SOCKETINFO** SocketInfoArray = NULL;
int maxSockets = FULL;

SOCKET client_sock;
SOCKET listen_sock;

int findIndex(SOCKET sock);
void RemoveSocketInfo(SOCKET sock);
bool DuplicateCheck(char* nickname);
void ChangeSocketArraySize(int newSize);

DWORD WINAPI AdminThread(LPVOID arg); // 관리자용 스레드
bool monitor = false;
char pw[BUFSIZE] = "1234";

// 클라이언트와 데이터 통신
DWORD WINAPI ProcessClient(LPVOID arg);

char mes[BUFSIZE];

int main(int argc, char* argv[])
{
    int retval;

    if (argc != 2) {
        printf("Usage : %s <SERVERPORT>\n", argv[0]);
        exit(1);
    }

    SERVERPORT = atoi(argv[1]);

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 소켓 생성
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // listen()
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    // 관리자 명령 스레드 생성
    HANDLE hAdminThread = CreateThread(NULL, 0, AdminThread, NULL, 0, NULL);
    if (hAdminThread == NULL) err_quit("CreateThread()");

    // 동적 배열 할당
    SocketInfoArray = new SOCKETINFO * [maxSockets];
    for (int i = 0; i < maxSockets; i++) {
        SocketInfoArray[i] = NULL;
    }

    // 데이터 통신에 사용할 변수
    struct sockaddr_in clientaddr;
    int addrlen;
    HANDLE hThread;

    while (true) {
        // accept()
    ACCEPT:
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        if (nTotalSockets >= maxSockets) {
            printf("[오류] 소켓 정보를 추가할 수 없습니다!\n");
            closesocket(client_sock);
            goto ACCEPT;
        }
        SOCKETINFO* ptr = new SOCKETINFO;
        if (ptr == NULL) {
            printf("[오류] 메모리가 부족합니다!\n");
            closesocket(client_sock);
            goto ACCEPT;
        }
        ptr->sock = client_sock;

        // 클라이언트 정보 출력
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

        // 클라이언트 이름 입력 받기
    RECV_NAME:
        retval = recv(client_sock, ptr->nickname, (int)strlen(ptr->nickname), 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        ptr->nickname[retval] = '\0';

        char checkmes[BUFSIZE];
        if (!DuplicateCheck(ptr->nickname)) {
            sprintf(checkmes, "FAIL");
            send(client_sock, checkmes, (int)strlen(checkmes), 0);
            goto RECV_NAME;
        }
        else {
            sprintf(checkmes, "SUCCESS");
            send(client_sock, checkmes, (int)strlen(checkmes), 0);
        }
        strcpy(ptr->ip, addr);
        ptr->port = ntohs(clientaddr.sin_port);
        SocketInfoArray[nTotalSockets++] = ptr;
        //add = true;

        printf("\n[클라이언트 접속(%d/%d)] 이름=%s, IP 주소=%s, 포트 번호=%d\n",
            nTotalSockets, maxSockets, ptr->nickname, ptr->ip, ptr->port);

        // 연결메시지 전송 인원 수와 함께
        char connectmes[BUFSIZE];
        sprintf(connectmes,
            "\n서버와 연결이 되었습니다.(%d/%d)\n메시지를 입력하여 대화를 시작하세요!",
            nTotalSockets, maxSockets);
        retval = send(ptr->sock, connectmes, (int)strlen(connectmes), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }

        char addmes[BUFSIZE];
        sprintf(addmes, "\n<<%s님이 접속하였습니다.(%d/%d) [%s:%d]\n",
            ptr->nickname, nTotalSockets, maxSockets, ptr->ip, ptr->port);

        if (nTotalSockets > 1) {
            for (int i = 0; i < nTotalSockets - 1; i++) {
                retval = send(SocketInfoArray[i]->sock, addmes, (int)strlen(addmes), 0);
                if (retval == SOCKET_ERROR) {
                    err_display("send()");
                    break;
                }
            }
        }

        // 스레드 생성
        hThread = CreateThread(NULL, 0, ProcessClient,
            (LPVOID)client_sock, 0, NULL);
        if (hThread == NULL) { RemoveSocketInfo(client_sock); }
        else { CloseHandle(hThread); }
    }

    // 소켓 닫기
    closesocket(listen_sock);

    // 동적 배열 해제
    delete[] SocketInfoArray;

    // 윈속 종료
    WSACleanup();
    return 0;
}

// 클라이언트와 데이터 통신
DWORD WINAPI ProcessClient(LPVOID arg)
{
    int retval;
    SOCKET clientsock = (SOCKET)arg;
    struct sockaddr_in clientaddr;
    char addr[INET_ADDRSTRLEN];
    int addrlen;

    int index = findIndex(clientsock);
    if (index == -1) {
        printf("\nindex is out of range!!\n");
        RemoveSocketInfo(clientsock);
        return -2;
    }

    SOCKETINFO* ptr = SocketInfoArray[index];

    // 클라이언트 정보 얻기
    addrlen = sizeof(clientaddr);
    getpeername(ptr->sock, (struct sockaddr*)&clientaddr, &addrlen);
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));


    while (1) {
    RECV_AGAIN:
        // 데이터 받기
        retval = recv(ptr->sock, ptr->buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("recv()");
            break;
        }
        else if (retval == 0)
            break;

        ptr->buf[retval] = '\0';
        // 명령어 체크
        if (strcmp(ptr->buf, "/exit") == 0) {
            printf("\n<<[%s/%s:%d] %s 명령어 사용!\n",
                ptr->nickname, ptr->ip, ptr->port, ptr->buf);
            break;
        }
        else if (strcmp(ptr->buf, "/member") == 0) {
            printf("\n<<[%s/%s:%d] %s 명령어 사용!\n",
                ptr->nickname, ptr->ip, ptr->port, ptr->buf);

            char tmpmember[BUFSIZE];
            sprintf(mes, "\n대화에 참여중인 사용자 (%d/%d): \n", nTotalSockets, maxSockets);
            for (int i = 0; i < nTotalSockets; i++) {
                sprintf(tmpmember, "            %d. %s : %s : %d\n",
                    i + 1, SocketInfoArray[i]->nickname, SocketInfoArray[i]->ip, SocketInfoArray[i]->port);
                strcat(mes, tmpmember);
            }
            send(ptr->sock, mes, (int)strlen(mes), 0);
            goto RECV_AGAIN;
        }

        // 데이터 보내기
        char tmp_buf[BUFSIZE + 1];
        strcpy(tmp_buf, ptr->buf);
        sprintf(ptr->buf, "[%s] %s", ptr->nickname, tmp_buf);

        if (monitor) {
            printf("%s\n", ptr->buf);
        }

        // 이름길이
        int namelen = (int)strlen(ptr->nickname) + 3;

        // 모든 클라이언트에 전송
        for (int i = 0; i < nTotalSockets; i++) {
            if (ptr->sock == SocketInfoArray[i]->sock) { continue; }
            retval = send(SocketInfoArray[i]->sock, ptr->buf, retval + namelen, 0);
            if (retval == SOCKET_ERROR) {
                err_display("send()");
                break;
            }
        }
    }

    // 소켓 닫기
    RemoveSocketInfo(clientsock);
    return 0;
}

int findIndex(SOCKET sock)
{
    for (int i = 0; i < nTotalSockets; i++) {
        if (SocketInfoArray[i]->sock == sock) {
            return i;
        }
    }
    return -1;
}

void RemoveSocketInfo(SOCKET sock) {

    int index = findIndex(sock);
    SOCKETINFO* ptr = SocketInfoArray[index];

    // 클라이언트 정보 출력
    printf("\n[클라이언트 종료(%d/%d)] 이름=%s, IP 주소=%s, 포트 번호=%d\n",
        nTotalSockets - 1, maxSockets, ptr->nickname, ptr->ip, ptr->port);

    sprintf(mes, "\n<<%s님이 접속을 종료했습니다.(%d/%d) [%s:%d]\n",
        ptr->nickname, nTotalSockets - 1, maxSockets, ptr->ip, ptr->port);

    // 소켓 
    closesocket(ptr->sock);
    delete ptr;

    if (index != (nTotalSockets - 1))
        SocketInfoArray[index] = SocketInfoArray[nTotalSockets - 1];
    --nTotalSockets;

    for (int i = 0; i < nTotalSockets; i++) {
        int retval = send(SocketInfoArray[i]->sock, mes, (int)strlen(mes), 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
    }
}

bool DuplicateCheck(char* nickname)
{
    for (int i = 0; i < nTotalSockets; i++) {
        if (!strcmp(nickname, SocketInfoArray[i]->nickname)) {
            return false;
        }
    }
    return true;
}

void ChangeSocketArraySize(int newSize) {
    if (newSize <= 0 || newSize == maxSockets) return;

    SOCKETINFO** newSocketArray = new SOCKETINFO * [newSize];
    for (int i = 0; i < newSize; i++) {
        newSocketArray[i] = (i < maxSockets) ? SocketInfoArray[i] : NULL;
    }

    delete[] SocketInfoArray;
    SocketInfoArray = newSocketArray;
    int beforeSize = maxSockets;
    maxSockets = newSize;

    printf("최대 인원이 %d에서 %d로 변경되었습니다.\n", beforeSize, newSize);
}

// 관리자 명령을 처리하는 스레드
DWORD WINAPI AdminThread(LPVOID arg) {
    char adminBuf[BUFSIZE + 1];
    while (true) {
    GET_AGAIN:
        printf("\n/admin을 입력하면 관리자 모드에 진입합니다.\n");
        if (fgets(adminBuf, BUFSIZE + 1, stdin) == NULL)
            break;

        int len = (int)strlen(adminBuf);
        if (adminBuf[len - 1] == '\n')
            adminBuf[len - 1] = '\0';
        if (strlen(adminBuf) == 0)
            continue;

        if (strcmp(adminBuf, "/admin") == 0) {
            int pwcnt = 0;
        PW:
            printf("[/admin]'s pw : ");
            if (fgets(adminBuf, BUFSIZE + 1, stdin) == NULL)
                break;

            int len = (int)strlen(adminBuf);
            if (adminBuf[len - 1] == '\n')
                adminBuf[len - 1] = '\0';
            if (strlen(adminBuf) == 0)
                continue;

            if (strcmp(adminBuf, pw) != 0) {
                printf("비밀번호가 다릅니다. 다시 입력해주세요. %d/3\n", ++pwcnt);
                if (pwcnt == 3) {
                    printf("3회 잘못입력하였습니다. 처음으로 돌아갑니다.\n");
                    goto GET_AGAIN;
                }
                goto PW;

            }

            printf("\n>>관리자 모드 진입. 명령어를 입력해주세요<<");
        CMD_LIST:
            printf("\n/quit - 관리자 모드 나가기, /nop - change number of people\n");
            printf("/monitoron - 채팅 보기, /monitoroff - 채팅 안보기\n");
            printf("/changepw - 관리자 비번 변경, /member - 전체 사용자 조회\n");
            printf("/shutdown - 서버 종료(단, 사용자 없을 경우)\n");

            if (fgets(adminBuf, BUFSIZE + 1, stdin) == NULL)
                break;

            len = (int)strlen(adminBuf);
            if (adminBuf[len - 1] == '\n')
                adminBuf[len - 1] = '\0';
            if (strlen(adminBuf) == 0)
                continue;

            if (strcmp(adminBuf, "/quit") == 0) {
                goto GET_AGAIN;
            }
            else if (strcmp(adminBuf, "/monitoron") == 0) {
                printf(">>지금부터 사용자들의 대화를 볼 수 있습니다.<<\n");
                monitor = true;
            }
            else if (strcmp(adminBuf, "/monitoroff") == 0) {
                printf(">>지금부터 사용자들의 대화를 볼 수 없습니다.<<\n");
                monitor = false;
            }
            else if (strcmp(adminBuf, "/nop") == 0) {
                printf(">>채팅방 최대 인원 수를 변경합니다.<<\n");
                int newSize;
                int beforeSize = maxSockets;

                while (1) {
                    printf("변경할 인원 수를 입력해주세요. : ");
                    scanf("%d", &newSize);
                    if (newSize >= nTotalSockets) break;
                    
                }
                ChangeSocketArraySize(newSize);
                sprintf(adminBuf, "\n>>최대 인원이 %d에서 %d로 변경되었습니다.<<\n", beforeSize, newSize);
                
                if (nTotalSockets >= 1) {
                    for (int i = 0; i < nTotalSockets; i++) {
                        int retval = send(SocketInfoArray[i]->sock, adminBuf, (int)strlen(adminBuf), 0);
                        if (retval == SOCKET_ERROR) {
                            err_display("send()");
                            break;
                        }
                    }
                }   
            }
            else if (strcmp(adminBuf, "/changepw") == 0) {
                while (1) {
                    char newpw[BUFSIZE];
                    char checkpw[BUFSIZE];
                    printf("변경할 비밀번호를 입력해주세요 : ");
                    scanf("%s", &newpw);
                    printf("변경할 비밀번호를 다시 한번 입력해주세요 : ");
                    scanf("%s", &checkpw);
                    

                    if (strcmp(newpw, checkpw) == 0) {
                        strcpy(pw, newpw);
                        printf("비밀번호 변경에 성공하였습니다. 변경된 비밀번호로 접속해주세요.\n");
                        break;
                    }
                    else {
                        printf("비밀번호 변경에 실패하였습니다. 다시 한번 입력해주세요.\n");
                    }
                }
            }
            else if (strcmp(adminBuf, "/member") == 0) {
                printf("\n전체 사용자를 조회합니다. (%d/%d)\n", nTotalSockets, maxSockets);
                for (int i = 0; i < nTotalSockets; i++) {
                    printf("            %d. %s : %s : %d\n",
                        i + 1, SocketInfoArray[i]->nickname, SocketInfoArray[i]->ip, SocketInfoArray[i]->port);
                }
            }
            else if (strcmp(adminBuf, "/shutdown") == 0) {
                if (nTotalSockets == 0) {
                    printf("서버를 종료합니다.\n");
                    closesocket(listen_sock);
                    WSACleanup();
                    break;
                }
                else {
                    printf("현재 연결된 클라이언트가 있으므로 서버를 종료할 수 없습니다.\n");
                }
            }
            else {
                printf("잘못되거나 없는 명령어 입니다.\n");
                goto CMD_LIST;
            }
        }
    }
    
    return 0;
}