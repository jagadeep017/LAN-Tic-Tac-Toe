#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

/*
 * This file contains the implementation of the Tic Tac Toe game.
 *
 *
 * The game allows two players on different devices on the same network to play Tic Tac Toe.
 * Players take turns to place their marks (X or O) on the grid.
 * The game checks for a win condition or a draw after each move.
 *
 * Functions in this file:
 * - Function to display the game board.
 * - Function to check for win conditions.
 * - Function to check for a draw.
 * - Main function to run the game loop.
 */

#define MULTICAST_PORT     10017
#define MULTICAST_IP       "224.0.0.170"
#define TCP_PORT           10018
#define VERIFY_NUMBER      1234509876

struct games    //structure to store the game data
{
    short int size;
    char player[2][31];
    char current;
    char choice;
    char arr[3][3];
    short int winner[3][2];
    short int offset;
    short int cnt;
}game = {3,{"O","X"},'O','\0',{' ',' ',' ',' ',' ',' ',' ',' ',' '},{-1,-1,-1,-1,-1,-1},0,0};   //init game data

struct broadcast_data{
    int id;
    char host_name[31];
};

struct ll{
    struct sockaddr_in addr;
    char name[31];
    struct ll *next;
}*sel_host;

struct hosting_sock{
    struct sockaddr_in *dest_addr;
    int udp;
};

struct game_data{
    short int offset;
    char choice;
};

void* hosting(void * temp){
    struct hosting_sock * temp1 = temp;
    struct broadcast_data data;
    data.id = htonl(VERIFY_NUMBER);
    strncpy(data.host_name, game.player[0], 31);

    while(1){
        if(sendto(temp1->udp, &data, sizeof(data), 0, (struct sockaddr *)temp1->dest_addr, sizeof(struct sockaddr_in)) < 0){
            perror("sendto");
            close(temp1->udp);
            exit(1);
        }
        sleep(3);
        //wait for response
    }
    return NULL;
}


int check(){  //function to check the winner return 1 if winner is found else 0
    int i=0,j=0;

    for(i=0;i<game.size;i++){
        if((game.arr[i][0]=='X' && game.arr[i][1]=='X' && game.arr[i][2]=='X')||(game.arr[i][0]=='O' && game.arr[i][1]=='O' && game.arr[i][2]=='O')){
            game.winner[0][0]=i;       //storing the winner's position
            game.winner[0][1]=0;
            game.winner[1][0]=i;
            game.winner[1][1]=1;
            game.winner[2][0]=i;
            game.winner[2][1]=2;
            return 1;
        }
        if((game.arr[0][i]=='X' && game.arr[1][i]=='X' && game.arr[2][i]=='X')||(game.arr[0][i]=='O' && game.arr[1][i]=='O' && game.arr[2][i]=='O')){
            game.winner[0][0]=0;       //storing the winner's position
            game.winner[0][1]=i;
            game.winner[1][0]=1;
            game.winner[1][1]=i;
            game.winner[2][0]=2;
            game.winner[2][1]=i;
            return 1;
        }
    }

    if((game.arr[0][0]=='X' && game.arr[1][1]=='X' && game.arr[2][2]=='X')||(game.arr[0][0]=='O' && game.arr[1][1]=='O' && game.arr[2][2]=='O')){
        game.winner[0][0]=0;       //storing the winner's position
        game.winner[0][1]=0;
        game.winner[1][0]=1;
        game.winner[1][1]=1;
        game.winner[2][0]=2;
        game.winner[2][1]=2;
        return 1;
    }
    if((game.arr[0][2]=='X' && game.arr[1][1]=='X' && game.arr[2][0]=='X')||(game.arr[0][2]=='O' && game.arr[1][1]=='O' && game.arr[2][0]=='O')){
        game.winner[0][0]=0;       //storing the winner's position
        game.winner[0][1]=2;
        game.winner[1][0]=1;
        game.winner[1][1]=1;
        game.winner[2][0]=2;
        game.winner[2][1]=0;
        return 1;
    }
    return 0;
}

//function to print the board takes the address of game structure as input
void print(){
    short int temp = game.offset;        //variable to store the offset
    char index = 0;
    printf("|~~~|~~~|~~~|\n");
    for(int i=0;i<game.size;i++){
        for(int j=0;j<game.size;j++){
            if(i == game.winner[index][0] && j == game.winner[index][1]){
                printf("|\033[0;32m-%c-\033[0m",game.arr[i][j]);
                index++;
                continue;
            }
            if(j==temp){
                printf("| %c_",game.arr[i][j]);
            }
            else{
                printf("| %c ",game.arr[i][j]);
            }
        }
        printf("|\n");
        printf("|~~~|~~~|~~~|\n");
        temp-=game.size;
    }
}

void host(){    //multicast information host name,  ip address , port number
    int udp_sock, tcp_sock;       //a udp socket for broadcast
    if((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){ //if udp socket creation failed
        perror("socket");
        exit(1);    //exit with failure status
    }

    unsigned char ttl = 1;
    if(setsockopt(udp_sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0){
        perror("setsockopt");
        close(udp_sock);
        exit(1);
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_addr.s_addr = inet_addr(MULTICAST_IP);
    dest_addr.sin_port = htons(MULTICAST_PORT);
    dest_addr.sin_family = AF_INET;

    if((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        close(udp_sock);
        exit(1);
    }
    struct sockaddr_in recv_addr = {0};
    recv_addr.sin_port = htons(TCP_PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    recv_addr.sin_family = AF_INET;
    if(bind(tcp_sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0){
        perror("bind");
        close(udp_sock);
        close(tcp_sock);
        exit(1);
    }

    if(listen(tcp_sock,1) < 0){
        perror("listen");
        close(tcp_sock);
        close(udp_sock);
        exit(1);
    }

    struct hosting_sock temp1;
    temp1.dest_addr = &dest_addr;
    temp1.udp = udp_sock;

    pthread_t threadid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if(pthread_create(&threadid, &attr, hosting, &temp1)){
        perror("pthread_create");
        close(udp_sock);
        close(tcp_sock);
        exit(1);
    }

    struct sockaddr_in addr = {0};
    socklen_t len1 = sizeof(addr);


    int client = accept(tcp_sock, (struct sockaddr *)&addr,&len1);

    if(client == -1){
        perror("accept");
        pthread_cancel(threadid);
        close(tcp_sock);
        close(udp_sock);
        exit(1);
    }

    struct broadcast_data data;
    data.id = 0;
    while(1){
        recv(client, &data, sizeof(struct broadcast_data), 0);
        if(data.id == VERIFY_NUMBER){
            break;
        }
    }
    pthread_cancel(threadid);
    strncpy(game.player[1], data.host_name,31);
    close(tcp_sock);
    close(udp_sock);
    close(client);
    // return 0;
}

void insert_host(struct broadcast_data *data, struct sockaddr_in *addr, struct ll **hosts){

    struct ll *new = malloc(sizeof(struct ll));
    if(new == NULL){
        return;
    }
    new->next = NULL;
    memcpy(&new->addr, addr, sizeof(struct sockaddr_in));
    memcpy(&new->name, data->host_name, 31*sizeof(char));

    if(*hosts == NULL){
        *hosts = new;
        return;
    }
    struct ll *temp = *hosts;
    while(temp->next){
        if(!strcmp(temp->name, new->name) && temp->addr.sin_addr.s_addr == new->addr.sin_addr.s_addr){
            free(new);
            return;
        }
        temp =temp->next;
    }
    if(!strcmp(temp->name, new->name) && temp->addr.sin_addr.s_addr == new->addr.sin_addr.s_addr){
        free(new);
        return;
    }
    temp->next = new;
}

void delete_hosts(struct ll *hosts){
    struct ll *prev = hosts;
    if(!prev){
        return;
    }
    hosts = hosts->next;
    while(hosts){
        free(prev);
        prev = hosts;
        hosts = hosts->next;
    }
    free(prev);
}

struct ll * select_host(struct ll *hosts, unsigned int i){
    i--;
    struct ll * prev = hosts;
    hosts = hosts->next;
    while(i - 1){
        free(prev);
        prev = hosts;
        hosts = hosts->next;
        i--;
    }
    delete_hosts(hosts);
    return prev;
}

unsigned int print_list(struct ll *hosts){
    unsigned int i = 1;
    while(hosts){
        printf("%3d) %s %s\n",i++, hosts->name, inet_ntoa(hosts->addr.sin_addr));
        hosts = hosts->next;
    }
    printf("%3d) refresh\n",i);
    return i;
}

void connect_host(){
    int udp_sock, tcp_sock;
    if((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket");
        exit(1);
    }

    int reuse = 1;
    if(setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0){
        perror("setsockopt");
        close(udp_sock);
        exit(1);
    }

    struct sockaddr_in local_addr = {0};
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(MULTICAST_PORT);
    local_addr.sin_family = AF_INET;

    if(bind(udp_sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0){
        perror("bind");
        close(udp_sock);
        exit(1);
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_IP);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if(setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0){
        perror("setsockopt");
        close(udp_sock);
        exit(1);
    }

    //setting a timeout for the multicast
    struct timeval time_out;
    time_out.tv_sec = 1;
    time_out.tv_usec = 0;
    if(setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &time_out, sizeof(time_out)) < 0){
        perror("socksetopt");
        close(udp_sock);
        exit(1);
    }

    while (1) {
        struct broadcast_data data;
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);
        struct ll *hosts = NULL;

        //listing all the hosts
        while(1){
            data.id = 0;
            recvfrom(udp_sock, &data, sizeof(data), 0,(struct sockaddr*)&sender_addr, &sender_len);
            if(data.id == 0){
                break;
            }

            // Verify the ID matches and insert to list
            else if (ntohl(data.id) == VERIFY_NUMBER) {
                // printf("Found host: %s (IP: %s)\n", data.host_name, inet_ntoa(sender_addr.sin_addr));
                insert_host(&data, &sender_addr, &hosts);
            }
        }
        unsigned int i, op;
        char retry = 0;
        while(1){
            i = print_list(hosts);
            printf("Choose a host to connect to 1-%d :-", i);
            scanf("%u", &op);
            if(op == 0 || op > i){
                printf("Invalid option\n");
                sleep(2);
                continue;
            }
            else if(op == i){
                retry = 1;
                delete_hosts(hosts);
                hosts = NULL;
            }
            break;
        }
        if(retry){
            continue;
        }
        sel_host = select_host(hosts, i);

        tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_sock < 0) {
            perror("TCP socket");
            delete_hosts(hosts);
            close(udp_sock);
            exit(1);
        }

        struct sockaddr_in server_addr = sel_host->addr;
        server_addr.sin_port = htons(TCP_PORT);

        if (connect(tcp_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("connect");
            free(sel_host);
            delete_hosts(hosts);
            close(tcp_sock);
            continue;
        }
        data.id = VERIFY_NUMBER;
        strncpy(data.host_name, game.player[1], 31);
        strncpy(game.player[0], sel_host->name, 31);
        send(tcp_sock, &data, sizeof(struct broadcast_data), 0);
        printf("Connected to %s!\n", sel_host->name);
        close(tcp_sock);
        break;
    }
    close(udp_sock);
    sleep(2);
    // return tcp_sock;
}

void run(int mode);

int main() {

    printf("welcome to tik-tac-toe\n");                 //welcome message
    while(1){
        printf("1) host a lobby\n2) join a lobby\nEnter an option :- ");
        int temp;
        scanf("%d",&temp);
        char flag = 1;
        int sock;
        switch(temp){
            case 1:
                printf("Enter name to host :- ");
                scanf("%30s",game.player[0]);
                host();
                run(0);
                break;
            case 2:
                printf("Enter name to connect to host:- ");
                scanf("%30s",game.player[1]);
                connect_host();
                run(1);
                break;
            default:
                getchar();
                printf("Invalid option, try again\n");
                flag = 0;
        }
        if(flag){
            break;
        }
    }
    return 0;               //exit the program
}

void change_turn(){
    if(game.current=='X'){                //changing the player turn
        game.current='O';
    }
    else{
        game.current='X';
    }
}

void move(short int offset, char choice){
    game.arr[offset/game.size][offset%game.size]=choice; //store the input in the array
}

void run(int mode){

    int client,sock;
    struct game_data data = {0};
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    //create a sock
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        perror("socket");
        exit(1);
    }
    struct sockaddr_in addr = {0};
    addr.sin_addr = sel_host->addr.sin_addr;
    addr.sin_port = htons(TCP_PORT);
    addr.sin_family = AF_INET;
    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
        perror("bind");
        exit(1);
    }
    if(listen(sock, 1) < 0){
        perror("listen");
        exit(1);
    }

    if(mode){
        if(connect(sock, (struct sockaddr *)sel_host, sizeof(struct sockaddr)) < 0){
            perror("connect");
            exit(1);
        }
        client = accept(sock, (struct sockaddr *)&client_addr, &len);
        if(client == -1){
            perror("connect");
            exit(1);
        }

        while(recv(client, &data, sizeof(data), 0) == 0);
        move(data.offset, data.offset);
    }
    else{
        client = accept(sock, (struct sockaddr *)&client_addr, &len);
        if(client == -1){
            perror("connect");
            exit(1);
        }
        if(connect(sock, (struct sockaddr *)sel_host, sizeof(struct sockaddr)) < 0){
            perror("connect");
            exit(1);
        }
    }
    while(check()==0){                //checking the winner
        system("clear");
        printf("%s's turn(%c)\n",game.player[game.cnt%2],game.current);
        print();
        game.choice=getchar();                  //taking the input
        while(game.choice=='\n'||game.choice==' '||game.choice=='\t'){  //if the input is a whitespace
            game.offset++;                    //increment the offset
            if(game.offset==9){             //if the offset is 9 reset it to 0
                game.offset=0;
            }
            system("clear");
            printf("%s's turn(%c)\n",game.player[game.cnt%2],game.current);
            print();
            game.choice=getchar();
        }
        if(game.choice>='a'&&game.choice<='z'){     //converting the input to uppercase
            game.choice=game.choice-32;
        }
        if(game.choice==game.current){              //if the input is valid
            if(game.arr[game.offset/game.size][game.offset%game.size]=='X'||game.arr[game.offset/game.size][game.offset%game.size]=='O'){   //if the space is already taken
                printf("space is already taken by %c\n",game.arr[game.offset/game.size][game.offset%game.size]);
                game.offset=-1;
                sleep(1);
                continue;
            }
            game.arr[game.offset/game.size][game.offset%game.size]=game.choice; //store the input in the array
        }
        else if((game.choice==game.current+9||game.choice==game.current-9)&&(game.choice!='X'+9&&game.choice!='O'-9)){  //if player enters the opposite character
            printf("the turn is %c's \n",game.current);     //print the current player
            sleep(1);
            continue;
        }
        else{                                        //if the input is invalid
            system("clear");
            printf("invalid input\n");
            printf("press enter to continue\n");
            sleep(1);
            game.offset=0;
            continue;
        }

        change_turn();

        if(game.cnt==8){                    //if the game is draw just print the board and exit
            if(check()){
                game.cnt++;
                break;
            }
            system("clear");
            printf("Game ended with DRAW\n");
            print();
            exit(1);                       //exit the game
        }
        game.cnt++;                     //increment the move count
        send(sock, &data, sizeof(data), 0);
        if(check()){
            break;
        }
        while(recv(client, &data, sizeof(data), 0) == 0);
        move(data.choice, data.choice);
        change_turn();
        game.cnt++;
    }

    change_turn();
    game.cnt++;
    system("clear");
    printf("Game ended with %s(%c)'s win\n",game.player[game.cnt%2],game.current);  //print the winner
    print();               //print the board
}
