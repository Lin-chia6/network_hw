#include "chap03.h"
#include <ctype.h>
#include <stdlib.h>

struct login_info
{
    char account[100];
    char password[100];
    int login;
    char name_id[10];
    int socket;
} login[4];

void check_login(int);
void ls(int);
void challenge(int, char*);
void game_form(int);
void game_play(int, int, char*, char);
void now_form(int, int[], char);
int check_done( int, int, int[], int, char);
void logout(int);
int find_id(int);
void unknown(int);

int main() {

    strcpy(login[0].account, "eleven");
    strcpy(login[0].password, "123");
    login[0].login = 0;
    strcpy(login[1].account, "amy");
    strcpy(login[1].password, "456");
    login[1].login = 0;
    strcpy(login[2].account, "john");
    strcpy(login[2].password, "789");
    login[2].login = 0;

#if defined(_WIN32)
    WSADATA d;
    if (WSAStartup(MAKEWORD(2, 2), &d)) {
        fprintf(stderr, "Failed to initialize.\n");
        return 1;
    }
#endif


    printf("Configuring local address...\n");
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *bind_address;
    getaddrinfo(0, "8080", &hints, &bind_address);


    printf("Creating socket...\n");
    SOCKET socket_listen;
    socket_listen = socket(bind_address->ai_family,
            bind_address->ai_socktype, bind_address->ai_protocol);
    if (!ISVALIDSOCKET(socket_listen)) {
        fprintf(stderr, "socket() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }


    printf("Binding socket to local address...\n");
    if (bind(socket_listen,
                bind_address->ai_addr, bind_address->ai_addrlen)) {
        fprintf(stderr, "bind() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }
    freeaddrinfo(bind_address);


    printf("Listening...\n");
    if (listen(socket_listen, 10) < 0) {
        fprintf(stderr, "listen() failed. (%d)\n", GETSOCKETERRNO());
        return 1;
    }

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socket_listen, &master);
    SOCKET max_socket = socket_listen;

    printf("Waiting for connections...\n");


    while(1) {
        fd_set reads;
        reads = master;
        if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
            fprintf(stderr, "select() failed. (%d)\n", GETSOCKETERRNO());
            return 1;
        }

        SOCKET i;
        for(i = 1; i <= max_socket; ++i) {
            if (FD_ISSET(i, &reads)) {

                if (i == socket_listen) {
                    
                    struct sockaddr_storage client_address;
                    socklen_t client_len = sizeof(client_address);
                    SOCKET socket_client = accept(socket_listen,
                            (struct sockaddr*) &client_address,
                            &client_len);

                    if (!ISVALIDSOCKET(socket_client)) {
                        fprintf(stderr, "accept() failed. (%d)\n",
                                GETSOCKETERRNO());
                        return 1;
                    }

                    FD_SET(socket_client, &master);
                    if (socket_client > max_socket)
                        max_socket = socket_client;

                    char address_buffer[100];
                    getnameinfo((struct sockaddr*)&client_address,
                            client_len,
                            address_buffer, sizeof(address_buffer), 0, 0,
                            NI_NUMERICHOST);
                    printf("New connection from %s\n", address_buffer);

                    check_login(socket_client);

                } 
                else {
                    char read[1024];
                    memset( read, 0, sizeof(read) );
                    int bytes_received = recv(i, read, 1024, 0);
                    if (bytes_received < 1) {
                        FD_CLR(i, &master);
                        CLOSESOCKET(i);
                        continue;
                    }

                    char *list = "ls";
                    char *fight = "fight";
                    char *lgout = "logout";
                    if( strcmp( list, read ) == 0 ) ls(i);
                    else if( strcmp( fight, read ) == 0 ) challenge(i, read);
                    else if( strcmp( lgout, read ) == 0 ) logout( i );
                    else unknown( i );

                }
            } //if FD_ISSET
        } //for i to max_socket
    } //while(1)

    printf("Closing listening socket...\n");
    CLOSESOCKET(socket_listen);

#if defined(_WIN32)
    WSACleanup();
#endif


    printf("Finished.\n");

    return 0;
}

void check_login( int client )
{
    while(1)
    {
        char buf[1000];
        memset(buf, 0, sizeof(buf));

        strcpy(buf, "What's your account?\n\0");
        send(client, buf, strlen(buf), 0);

        char check_acc[1000];
        char check_pas[1000];
        memset(check_acc, 0, sizeof(check_acc));
        memset(check_pas, 0, sizeof(check_pas));
        int acc_r = recv( client, check_acc, sizeof(check_acc), 0);
    
        memset(buf, 0, sizeof(buf));
        strcpy(buf, "Password?\n\0");
        send(client, buf, sizeof(buf), 0);
        int pas_r = recv( client, check_pas, sizeof(check_pas), 0);
        
        int flag = 0;
        int id;

        for( int i = 0; i < 3; i++ )
        {
            if( strcmp( login[i].account, check_acc) == 0 
                && strcmp( login[i].password, check_pas ) == 0 )
            {
                flag = 1;
                id = i;
            }
        }

        if( flag == 1 )
        {
            if( login[id].login == 1 )
            {
                memset(buf, 0, sizeof(buf));
                strcpy(buf, "This is logged in! please try again\n\0");
                send(client, buf, strlen(buf), 0);
                
                continue;
            }
            memset(buf, 0, sizeof(buf));
            strcpy(buf, "-----Success!-----\n-You can use 'ls' to watch who is online\n-You can use 'fight' to challenge others\n-You can use 'logout' to Logout\n");
            send(client, buf, sizeof(buf), 0);
            login[id].login = 1;
            login[id].socket = client;
            char tmp[10];
            memset(tmp, 0, sizeof(tmp));
            sprintf(tmp, "%d", id);
            strcpy(login[id].name_id, tmp);

            
            return;
        }
        else
        {
            memset(buf, 0, sizeof(buf));
            strcpy(buf, "Incorrect! please try again\n\0");
            send(client, buf, strlen(buf), 0);
        }
    }
}

void ls( int client )
{

    send(client, "\nLogin list:\n=================================\n", strlen("\nLogin list:\n=================================\n"), 0);
    send(client, "id | name\n---------------------------------\n", strlen("id | name\n---------------------------------\n"), 0);

    for( int i = 0; i < 3; i++ )
    {
        if( login[i].login == 1 )
        {
            char buf[1000];
            memset(buf, 0, sizeof(buf));
            sprintf(buf, " %d : %s \n", i, login[i].account);
            send(client, buf, strlen(buf), 0);
        }
    }

    //send(client, buf, sizeof(buf), 0);
}

void challenge( int client, char *read )
{
    char buf[1000];
    int client_id;
    client_id = find_id(client);

    ls(client);
    send( client, "\nChoose one's id which you want to challenge\n", strlen("\nChoose one's id which you want to challenge\n"), 0 );

    memset(read, 0, sizeof(read));
    int bytes_received = recv(client, read, 1024, 0);
    char *id = read;
    for( int i = 0; i < 3; i++ )
    {
        if( strcmp(login[i].name_id, id) == 0 )
        {
            send( client, "wait for response...\n", strlen("wait for response...\n"), 0 );
            memset(buf, 0, sizeof(buf));
            sprintf( buf, "'%s' want to play a game with you (yes/no)\n", login[client_id].account);
            send( login[i].socket, buf, strlen(buf), 0 );

            memset(read, 0, sizeof(read));
            bytes_received = recv(login[i].socket, read, 1024, 0);
            char *ok = "yes";
            if( strcmp( ok, read ) == 0 )
            {
                send( login[i].socket, "Let's start the game(OX)\n", strlen("Let's start the game(OX)\n"), 0);
                send( login[i].socket, "---------------------------------\n", strlen("---------------------------------\n"), 0 );
                send( login[i].socket, "Wait for choosing...\n", strlen("Wait for choosing...\n"), 0);
                send( login[client_id].socket, "response is 'yes'!!. Let's start the game(OX)\n", strlen("response is 'yes'!!. Let's start the game(OX)\n"), 0 );
                send( login[client_id].socket, "---------------------------------\n", strlen("---------------------------------\n"), 0 );
                send( login[client_id].socket, "Pick 'o' or 'x'\n", strlen("Pick 'o' or 'x'\n"), 0);
                memset(read, 0, sizeof(read));
                bytes_received = recv(login[client_id].socket, read, 1024, 0);
                char *o = "o";
                char inviter_pick;
                if( strcmp( o, read ) == 0 )
                {
                    inviter_pick = 'O';
                    send( login[client_id].socket, "You are 'O'\n", strlen("You are 'O'\n"), 0);
                    send( login[i].socket, "You are 'X'\n", strlen("You are 'X'\n"), 0);
                    //game_form( login[client_id].socket );
                    //game_form( login[i].socket );
                }
                else
                {
                    inviter_pick = 'X';
                    send( login[client_id].socket, "You are 'X'\n", strlen("You are 'X'\n"), 0);
                    send( login[i].socket, "You are 'O'\n", strlen("You are 'O'\n"), 0);
                    //game_form( login[client_id].socket );
                    //game_form( login[i].socket );
                }

                game_play( login[client_id].socket, login[i].socket, read, inviter_pick );
                return;
            }
            else
            {
                send( login[client_id].socket, "response is 'no', please try again...\n\0", strlen("response is 'no', please try again...\n"), 0 );
                return;
            }
            
        }
    }
    
}

void game_form( int client )
{
    send( client, "\n  0  |  1  |  2  \n", strlen("\n  0  |  1  |  2  \n"), 0 );
    send( client, "-----------------\n", strlen("-----------------\n"), 0 );
    send( client, "  3  |  4  |  5  \n", strlen("  3  |  4  |  5  \n"), 0 );
    send( client, "-----------------\n", strlen("-----------------\n"), 0 );
    send( client, "  6  |  7  |  8  \n", strlen("  6  |  7  |  8  \n"), 0 );
}

void now_form( int client, int record[9], char one_pick )
{
    char another;
    if( one_pick == 'O' ) another = 'X';
    else another = 'O';
    char buf[1024];
    char tmp[9];
    memset(buf, 0, sizeof(buf));
    memset(tmp, ' ', sizeof(tmp));
    for( int i = 0; i < 9; i++ )
    {
        if( record[i] == 1 ) tmp[i] = one_pick;
        else if( record[i] == 2 )tmp[i] = another;
    }
    sprintf(buf,"\n  %c  |  %c  |  %c  \n-----------------\n  %c  |  %c  |  %c  \n-----------------\n  %c  |  %c  |  %c  \n",
            tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5], tmp[6], tmp[7], tmp[8] );
    buf[strlen(buf)] = '\0';

    send( client, buf, strlen(buf), 0 );
}

void game_play( int inviter, int opponent, char *read, char inviter_pick )
{
    int game_record[9];
    memset(game_record, 0, sizeof(game_record));
    int bytes_received, choose, done = 0;
    while( done == 0 )
    {
        game_form(inviter);
        now_form(inviter, game_record, inviter_pick);

        send( inviter, "You turn\n", strlen( "You turn\n"), 0 );
        now_form(opponent, game_record, inviter_pick);
        send( opponent, "Wait for another...\n", strlen( "Wait for another...\n"), 0 );
        memset(read, 0, sizeof(read));
        bytes_received = recv(inviter, read, 1024, 0);
        choose = (int)read[0]%48;
        while( game_record[choose] != 0 )
        {
            send( inviter, "This is be choosing...\n", strlen( "This is be choosing...\n"), 0 );
            memset(read, 0, sizeof(read));
            bytes_received = recv(inviter, read, 1024, 0);
            choose = (int)read[0]%48;
        }
        game_record[choose] = 1;
        
        done = check_done( inviter, opponent, game_record, done, inviter_pick );
        if( done == 1 ) break;

        game_form(opponent);
        now_form(opponent, game_record, inviter_pick);

        send( opponent, "You turn\n", strlen( "You turn\n"), 0 );
        now_form(inviter, game_record, inviter_pick);
        send( inviter, "Wait for another...\n", strlen( "Wait for another...\n"), 0 );
        memset(read, 0, sizeof(read));
        bytes_received = recv(opponent, read, 1024, 0);
        choose = (int)read[0]%48;
        while( game_record[choose] != 0 )
        {
            send( opponent, "This is be choosing, try another...\n", strlen( "This is be choosing, try another...\n"), 0 );
            memset(read, 0, sizeof(read));
            bytes_received = recv(opponent, read, 1024, 0);
            choose = (int)read[0]%48;
        }
        game_record[choose] = 2;

        done = check_done( inviter, opponent, game_record, done, inviter_pick );
    }
}

int check_done( int client1, int client2, int record[9], int done, char pick )
{
    int flag = 0, peace = 1;

    if( (record[0] == 1 && record[1] == 1 && record[2] == 1) 
        || (record[0] == 1 && record[3] == 1 && record[6] == 1)
        || (record[0] == 1 && record[4] == 1 && record[8] == 1) ) flag = 1;
    else if( record[1] == 1 && record[4] == 1 && record[7] == 1 ) flag = 1;
    else if( (record[2] == 1 && record[5] == 1 && record[8] == 1) 
            || (record[2] == 1 && record[4] == 1 && record[6] == 1) ) flag = 1;
    else if( record[3] == 1 && record[4] == 1 && record[5] == 1 ) flag = 1;
    else if( record[6] == 1 && record[7] == 1 && record[8] == 1 ) flag = 1;

    if( flag != 1 )
    {
        if( (record[0] == 2 && record[1] == 2 && record[2] == 2) 
            || (record[0] == 2 && record[3] == 2 && record[6] == 2)
            || (record[0] == 2 && record[4] == 2 && record[8] == 2) ) flag = 2;
        else if( record[1] == 2 && record[4] == 2 && record[7] == 2 ) flag = 2;
        else if( (record[2] == 2 && record[5] == 2 && record[8] == 2) 
                || (record[2] == 2 && record[4] == 2 && record[6] == 2) ) flag = 2;
        else if( record[3] == 2 && record[4] == 2 && record[5] == 2 ) flag = 2;
        else if( record[6] == 2 && record[7] == 2 && record[8] == 2 ) flag = 2;
    }

    if( flag == 0 )
    {
        for( int i = 0; i < 9; i++ )
        {
            if( record[i] == 0 )
            {
                peace = 0;
                break;
            }
        }

        if( peace == 1 )
        {
            now_form(client1, record, pick);
            now_form(client2, record, pick);

            send( client1, "--------------------\n-----..Tie !!..-----\n--------------------\n", strlen("--------------------\n-----..Tie !!..-----\n--------------------\n"), 0 );
            send( client2, "--------------------\n-----..Tie !!..-----\n--------------------\n", strlen("--------------------\n-----..Tie !!..-----\n--------------------\n"), 0 );

            return 1;
        }
    }

    if( flag != 0 )
    {
        done = 1;
        now_form(client1, record, pick);
        now_form(client2, record, pick);
        if( flag == 1 )
        {
            send( client1, "--------------------\n-----You Win !!-----\n--------------------\n", strlen("--------------------\n-----You Win !!-----\n--------------------\n"), 0 );
            send( client2, "--------------------\n-----You Lose..-----\n--------------------\n", strlen("--------------------\n-----You Lose..-----\n--------------------\n"), 0 );
        }
        else
        {
            send( client2, "--------------------\n-----You Win !!-----\n--------------------\n", strlen("--------------------\n-----You Win !!-----\n--------------------\n"), 0 );
            send( client1, "--------------------\n-----You Lose..-----\n--------------------\n", strlen("--------------------\n-----You Lose..-----\n--------------------\n"), 0 );
        }
        return 1;
    }

    return 0;
}

void logout( int client )
{
    send( client, "Logout...\n", strlen("Logout...\n"), 0 );
    int client_id;
    client_id = find_id(client);
    login[client_id].login = 0;
}

int find_id( int client_socket )
{
    int id;
    for( int i = 0; i < 3; i++ )
    {
        if( client_socket == login[i].socket )
        {
            id = i;
            break;
        }
    }

    return id;
}

void unknown( int client )
{
    char buf[1000];

    memset(buf, 0, sizeof(buf));
    strcpy(buf, "This is unknown command...\nYou can use these commands :\n  -You can use 'ls' to watch who is online\n  -You can use 'fight' to challenge others\n  -You can use 'logout' to Logout\n");
    send(client, buf, sizeof(buf), 0);
}