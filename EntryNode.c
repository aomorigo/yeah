#include <stdio.h> // For printf(), fprintf(), perror()
#include <sys/socket.h> // For socket(), bind(), connect(),recv(), send()
#include <arpa/inet.h> // For sockaddr_in, inet_ntoa()
#include <stdlib.h> // For atoi()
#include <string.h> // For exit(), memset()
#include <unistd.h> // For close()

#define MAXPENDING 5 // 同時接続要求の最大数
#define RECVBUFSIZE 32 // 受信バッファサイズ

#define DEFAULT_PORT 7

void errorHandler (char *msg) {
  perror(msg);
  exit(1);
}

void HandleTCPClient(int clientSock) {
  unsigned  char header[16];
  char *body;
  int recvMsgSize;
  unsigned long long length;
  int ip[4];
  unsigned int port;
  char ipstr[16];

  if ((recvMsgSize = recv(clientSock, header, 16, 0)) < 0) {
    errorHandler("recv() failed");
  }

  ip[0]=header[0];
  ip[1]=header[1];
  ip[2]=header[2];
  ip[3]=header[3];
  sprintf(ipstr, "%d.%d.%d.%d\n",ip[0],ip[1],ip[2],ip[3]);
  printf("IP :  %s", ipstr);

  port =
    (header[4]<<24)
    + (header[5]<<16)
    + (header[6]<<8)
    + (header[7]);
  printf("port : %d\n",port);

  length =
    ((unsigned long long)header[8]<<56)
    + ((unsigned long long)header[9]<<48)
    + ((unsigned long long)header[10]<<40)
    + ((unsigned long long)header[11]<<32)
    + ((unsigned long long)header[12]<<24)
    + ((unsigned long long)header[13]<<16)
    + ((unsigned long long)header[14]<<8)
    + ((unsigned long long)header[15]);
  printf("length : %llu\n", length);
  body = (char *)malloc(length);
  recv(clientSock, body, length, 0);
  printf("%s\n", body);
  free(body);

  /*
  if (send(clientSock, echoBuffer, recvMsgSize, 0) != recvMsgSize) {
    errorHandler("send() failed");
  }
  */

  //client

  int sock;
  struct sockaddr_in GoalServerAddr;
  unsigned short GoalServerPort;
  char *GoalserverIP;
  char *echoString;
  unsigned char data[21];

  char echoBuffer[RECVBUFSIZE];
  unsigned int echoStringLen;
  int recvBytes, totalRecvBytes;
  // create socket
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    errorHandler("socket() failed");
  }
  // create address struct of Goalserver
  memset(&GoalServerAddr, 0, sizeof(GoalServerAddr)); // GoalServerAddr 構造体の領域をゼロクリアしておく
  GoalServerAddr.sin_family = AF_INET;
  GoalServerAddr.sin_addr.s_addr = inet_addr(ipstr);
  GoalServerAddr.sin_port = htons(port);

  // connect Goalserver
  if (connect(sock, (struct sockaddr *) &GoalServerAddr, sizeof(GoalServerAddr)) < 0) {
    errorHandler("connect() failed");
  }
  printf("send!!!!!\n" );
  // send message to Goalserver
  if (send(sock, body, length, 0) != length) {
    errorHandler("send() failed");
  }

  // recieve message from Goalserver
  totalRecvBytes = 0;
  printf("Recieved: \n");
  do{
    // echo back message to client
    // 受信データが残ってないか確認
    if ((recvBytes = recv(sock, echoBuffer, RECVBUFSIZE-1, 0)) < 0) {
      errorHandler("recv() failed");
    }
    echoBuffer[recvBytes]='\0';
    printf("%s\n", echoBuffer);
    send(clientSock,echoBuffer,recvBytes,0);
  }while (recvBytes > 0);

  printf("\n");

  // close socket
  close(sock);
  close(clientSock);
  printf("closed\n" );
}

int main (int argc, char *argv[]) {

  int serverSock;
  int clientSock;
  struct sockaddr_in echoServerAddr;
  struct sockaddr_in echoClientAddr;
  unsigned short echoServerPort;
  unsigned int clientLen;

  //server

  echoServerPort = atoi(argv[1]);

  // create socket
  if ((serverSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    errorHandler("socket() failed");
  }

  // create address struct of echo server
  memset(&echoServerAddr, 0, sizeof(echoServerAddr)); // echoServerAddr 構造体の領域をゼロクリアしておく
  echoServerAddr.sin_family = AF_INET;
  echoServerAddr.sin_addr.s_addr = htonl(INADDR_ANY); // local ip 0.0.0.0
  echoServerAddr.sin_port = htons(echoServerPort);

  // bind socket
  if (bind(serverSock, (struct sockaddr *) &echoServerAddr, sizeof(echoServerAddr)) < 0) {
    errorHandler("bind() failed");
  }

  // listen port
  if (listen(serverSock, MAXPENDING) < 0) {
    errorHandler("listen() failed");
  }

  // accept connection-request from client
  for (;;) {
    clientLen = sizeof(echoClientAddr);
    clientSock = accept(serverSock, (struct sockaddr *) &echoClientAddr, &clientLen);
    if (clientSock < 0) {
      errorHandler("accept() failed");
    }
    printf("Handling client %s\n", inet_ntoa(echoClientAddr.sin_addr));
    HandleTCPClient(clientSock);
  }
    exit(0);
}
