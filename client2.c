#include <stdio.h> // For printf(), fprintf(), perror()
#include <sys/socket.h> // For socket(), connect(), send(), recv()
#include <arpa/inet.h> // For sockaddr_in, inet_addr()
#include <stdlib.h> // For atoi()
#include <string.h> // For exit(), memset()
#include <unistd.h> // For close()

#define RECVBUFSIZE 32 // 受信バッファサイズ
#define DEFAULT_PORT 7 // echo サービスの well-known ポート

unsigned int info[][5]={
  {153,126,160,243,8000}, //最初がゴール
  //{153,126,160,243,5003},
  {172,18,0,1,5010},
  {153,126,160,243,5006},
  {0,0,0,0,0}
};

void errorHandler (char *msg) {
  perror(msg);
  exit(1);
}


unsigned long long make_data(unsigned char* ans,unsigned int info[][5], unsigned char* data, unsigned long long length){
  bool exit_f=true;
  for(int i=0;i<5;i++){
    if(info[0][i]){
      exit_f=false;
      break;
    }
  }
  if(exit_f){
      for(int i =0;i<length;i++){
        ans[i]=data[i];
      }
      return length;
  }
  unsigned char *new_data;
  new_data = (unsigned char*)malloc(16+length);
  new_data[0] = info[0][0];
  new_data[1] = info[0][1];
  new_data[2] = info[0][2];
  new_data[3] = info[0][3];

  new_data[4] = info[0][4]/(256*256*256);
  new_data[5] = (info[0][4]%(256*256*256))/(256*256);
  new_data[6] = (info[0][4]%(256*256))/256;
  new_data[7] = info[0][4]%256;

  new_data[8] = 0;
  new_data[9] = 0;
  new_data[10] =0;
  new_data[11] =0;
  for(int i=0;i<4;i++){
    unsigned long long base=1;
    for(int ii=(3-i);ii>0;ii--){
      base*=256;
    }

    if(i){
      new_data[i+12] = (length%(base*256))/(base);
    }else{

      new_data[i+12] = length/base;
    }
  }
  for(unsigned long long i=0; i<length; i++){
    new_data[16+i] = data[i];
  }
  for(int i=0;;i++){
    bool exit_f=true;
    for(int ii=0;ii<5;ii++){
      if(exit_f && info[i+1][ii])exit_f=false;
      info[i][ii]=info[i+1][ii];
    }
    if(exit_f)break;
  }
  return make_data(ans, info, new_data, length+16);
}



int main (int argc, char *argv[]) {
  unsigned char* test;
  unsigned char tdata[]={'f','o','o','\0'};
  unsigned long long plength;
  test = (unsigned char *)malloc(4+16*5);
  plength= make_data(test, info, tdata, 4);
  for(unsigned long long i=0;i<plength;i++){
    printf("%u (%d/%d)\n", test[i], i, plength);
  }

  int sock; // socket descriptor
  struct sockaddr_in echoServerAddr;
  unsigned short echoServerPort;
  char *serverIP;
  char *echoString;
  /*unsigned char data[21];
  data[0]=127;
  data[1]=0;
  data[2]=0;
  data[3]=1;
  data[4]=0;
  data[5]=0;
  data[6]=0x1f;
  data[7]=0x40;
  data[8]=0;
  data[9]=0;
  data[10]=0;
  data[11]=0;
  data[12]=0;
  data[13]=0;
  data[14]=0;
  data[15]=5;
  data[16]='h';
  data[17]='o';
  data[18]='g';
  data[19]='e';
  data[20]='\0'; */

  char echoBuffer[RECVBUFSIZE];
  unsigned int echoStringLen;
  int recvBytes, totalRecvBytes;

  // arguments validation
  if ((argc < 3) || (argc > 4)) {
    fprintf(stderr, "Usage: %s <Server IP> <Echo Word> [<Server Port>]\n", argv[0]);
    exit(1);
  }

  serverIP = argv[1];
  echoString = argv[2];

  // port validation
  if (argc == 4) {
    echoServerPort = atoi(argv[3]);
  } else {
    echoServerPort = DEFAULT_PORT;
  }

  // create socket
  if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    errorHandler("socket() failed");
  }

  // create address struct of echo server
  memset(&echoServerAddr, 0, sizeof(echoServerAddr)); // echoServerAddr 構造体の領域をゼロクリアしておく
  echoServerAddr.sin_family = AF_INET;
  echoServerAddr.sin_addr.s_addr = inet_addr(serverIP);
  echoServerAddr.sin_port = htons(echoServerPort);

  // connect echo server
  if (connect(sock, (struct sockaddr *) &echoServerAddr, sizeof(echoServerAddr)) < 0) {
    errorHandler("connect() failed");
  }

  // send message to echo server
  if (send(sock, test, plength, 0) != plength) {
    errorHandler("send() failed");
  }

  // recieve message from echo server
  totalRecvBytes = 0;
  printf("Recieved: ");
  do {
    if ((recvBytes = recv(sock, echoBuffer, RECVBUFSIZE-1, 0)) < 0) {
      errorHandler("recv() failed");
    }
    totalRecvBytes += recvBytes;
    echoBuffer[recvBytes] = '\0';
    printf("%s", echoBuffer);
  }while(recvBytes >0);
  printf("\n");

  // close socket
  close(sock);
  exit(0);


}
