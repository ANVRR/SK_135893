#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#include <stdbool.h>
#define PORT 1234
#define MAX_GAMES 5

typedef struct {
    int sockets[2];
    int welcome[2];
    int currentPlayer; //0 = player a, 1 = player b
    char board[8][8];
    int active; //both players connected
} GameState;
GameState games[MAX_GAMES] = {{0}};

void InitializeGame(GameState* game) {
    CreateTable(game->board);
    game->currentPlayer = 0;  //start with player a
    game->welcome[0] = 0;
    game->welcome[1] = 0;
}

void CreateTable(char board[8][8]) { //fill the matrix
    for(int i = 0; i <= 7; i++) {
        for(int j = 0; j <= 7; j++) {
            if((i + j) % 2 == 0) board[i][j] = '-'; //'-' white squares
            else board[i][j] = 'o'; //'o' playable black squares
        }
    }
    
    //place 'b' in rows 1,2,3
    for(int i = 0; i <= 2; i++) {
        for(int j = 0; j <= 7; j++) {
            if(board[i][j] == 'o') board[i][j] = 'b';
        }
    }
    
    //place 'a' in rows 6,7,8
    for(int i = 5; i <= 7; i++) {
        for(int j = 0; j <= 7; j++) {
            if(board[i][j] == 'o') board[i][j] = 'a';
        }
    }
}

char* ShowBoard(char board[8][8]) { //convert board state to string "-b-b-b-bb-b"
    int bufferSize = 100;
    char* boardRepresentation = (char*)malloc(bufferSize);
    int pos = 0;
    for (int i = 0; i <= 7; i++) {
        for (int j = 0; j <= 7; j++) {
            pos += sprintf(boardRepresentation + pos, "%c", board[i][j]);
        }
    }
    return boardRepresentation;
}

int CheckWin(char board[8][8], int player) {
    char opponentPiece = (player == 0) ? 'b' : 'a';
    char opponentKing = (player == 0) ? 'B' : 'A';
    int opponentHasPieces = 0;
    
    for (int row = 0; row < 8; ++row) { //look for enemy pieces
        for (int col = 0; col < 8; ++col) {
            if (board[row][col] == opponentPiece || board[row][col] == opponentKing) {
                opponentHasPieces = 1;
                break;
            }
        }
        if (opponentHasPieces) {
            break; //exit if at least one piece is found
        }
    }

    return !opponentHasPieces;
}



void PromoteToKing(char board[8][8], int row, int col) {
    if (board[row][col] == 'a' && row == 0) { //'a' piece reaches top
        board[row][col] = 'A'; //promote
    } else if (board[row][col] == 'b' && row == 7) { //'b' piece reaches bottom
        board[row][col] = 'B';
    }
}

int CanCapture(char board[8][8], int player) {
    char playerPiece = (player == 0) ? 'a' : 'b';
    char playerKing = (player == 0) ? 'A' : 'B';
    char opponentPiece = (player == 0) ? 'b' : 'a';
    char opponentKing = (player == 0) ? 'B' : 'A';
    int playerDirection = (player == 0) ? -1 : 1; //-1 for player 'a', 1 for player 'b'
    
    //directions for regular pieces and kings
    int directions[4][2] = {{playerDirection, -1}, {playerDirection, 1},
                             {-playerDirection, -1}, {-playerDirection, 1}}; //all four diagonal directions
    
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (board[row][col] == playerPiece || board[row][col] == playerKing) {
                int directionsToCheck = (board[row][col] == playerPiece) ? 2 : 4; //2 for pieces, 4 for king
                
                for (int i = 0; i < directionsToCheck; ++i) {
                    int midRow = row + directions[i][0];
                    int midCol = col + directions[i][1];
                    int destRow = midRow + directions[i][0];
                    int destCol = midCol + directions[i][1];
                    
                    //check bounds and possible capture
                    if (midRow >= 0 && midRow < 8 && midCol >= 0 && midCol < 8 &&
                        destRow >= 0 && destRow < 8 && destCol >= 0 && destCol < 8) {
                        if ((board[midRow][midCol] == opponentPiece || board[midRow][midCol] == opponentKing) && board[destRow][destCol] == 'o') {
                            return 1; //capture possible
                        }
                    }
                }
            }
        }
    }
    return 0; //capture not possible
}

int CanCaptureFromPosition(char board[8][8], int srcRow, int srcCol, int player) {
    char opponentPiece = (player == 0) ? 'b' : 'a';
    char opponentKing = (player == 0) ? 'B' : 'A';
    char playerKing = (player == 0) ? 'A' : 'B';
    int isKing = (board[srcRow][srcCol] == playerKing);
    int captureDirection = (player == 0) ? -1 : 1; //0 for white (up), 1 for black (down)
    int directions[4][2] = {
        {captureDirection, -1}, {captureDirection, 1},
        {-captureDirection, -1}, {-captureDirection, 1} //only for kings
    };

    for (int i = 0; i < 4; ++i) {
        //if not a king, skip backward directions
        if (!isKing && i >= 2) {
            continue;
        }
        int midRow = srcRow + directions[i][0];
        int midCol = srcCol + directions[i][1];
        int destRow = midRow + directions[i][0];
        int destCol = midCol + directions[i][1];
        //check bounds and possible capture
        if (midRow >= 0 && midRow < 8 && midCol >= 0 && midCol < 8 && destRow >= 0 && destRow < 8 && destCol >= 0 && destCol < 8) {
            if ((board[midRow][midCol] == opponentPiece || board[midRow][midCol] == opponentKing) && board[destRow][destCol] == 'o') {
                return 1; //capture is possible
            }
        }
    }
    return 0; //capture not possible
}

void PerformCapture(char board[8][8], int srcRow, int srcCol, int destRow, int destCol) {
    int midRow = (srcRow + destRow) / 2;
    int midCol = (srcCol + destCol) / 2;
    board[destRow][destCol] = board[srcRow][srcCol]; //move piece
    board[srcRow][srcCol] = 'o'; //clear source square
    board[midRow][midCol] = 'o'; //rmove captured piece

    //check for promotion after capture
    PromoteToKing(board, destRow, destCol);
}

int IsValidMove(char board[8][8], int srcRow, int srcCol, int destRow, int destCol, int player) {
    //check if out of bounds
    if (srcRow < 0 || srcRow > 7 || srcCol < 0 || srcCol > 7 || destRow < 0 || destRow > 7 || destCol < 0 || destCol > 7) {
        return 0;
    }

    //identifying pieces
    char playerPiece = (player == 0) ? 'a' : 'b';
    char playerKing = (player == 0) ? 'A' : 'B';
    char opponentPiece = (player == 0) ? 'b' : 'a';
    char opponentKing = (player == 0) ? 'B' : 'A';

    //check if source is a king or normal piece
    int isKing = (board[srcRow][srcCol] == playerKing);
    if (board[srcRow][srcCol] != playerPiece && !isKing) {
        return 0; //none
    }

    //check if dest is empty
    if (board[destRow][destCol] != 'o') {
        return 0;
    }

    //calculate dir and dist
    int rowDistance = destRow - srcRow;
    int colDistance = abs(destCol - srcCol);

    //regular move
    if (isKing && abs(rowDistance) == 1 && colDistance == 1) {
        return 1; //valid king move
    } else if (!isKing && ((player == 0 && rowDistance == -1) || (player == 1 && rowDistance == 1)) && colDistance == 1) {
        return 1; //valid regular move
    }

    //capture move
    if ((isKing || (player == 0 && rowDistance == -2) || (player == 1 && rowDistance == 2)) && colDistance == 2) {
        int midRow = srcRow + rowDistance / 2;
        int midCol = srcCol + (destCol - srcCol) / 2;
        if (midRow >= 0 && midRow < 8 && midCol >= 0 && midCol < 8 && (board[midRow][midCol] == opponentPiece || board[midRow][midCol] == opponentKing) && board[destRow][destCol] == 'o') {
            return 2; //valid capture
        }
    }

    return 0; //invalid move
}

int MakeMove(int sock, char board[8][8], char* move, int* currentPlayer) {
    printf("Wykonano ruch: %s\n", move);

    //convert move into board coordinates
    int srcRow = 7 - (move[1] - '1');
    int srcCol = tolower(move[0]) - 'a';
    int destRow = 7 - (move[4] - '1');
    int destCol = tolower(move[3]) - 'a';
    
    //look for available captures
    int captureAvailable = CanCapture(board, *currentPlayer);

    //check if move is valid
    int moveType = IsValidMove(board, srcRow, srcCol, destRow, destCol, *currentPlayer);

    if (captureAvailable) {
        //if there is a capture move available - only captures are valid
        if (moveType == 2) { //capture is valid
            PerformCapture(board, srcRow, srcCol, destRow, destCol); //perform capture

            //look for more captures
            if (CanCaptureFromPosition(board, destRow, destCol, *currentPlayer)) {
                //do not swap turns if multi-capture is possible
                return 2;
            } else {
                //swap turns
                *currentPlayer = (*currentPlayer + 1) % 2;
                return 1; //capture successful, eot
            }
        } else {
            //if available capture is not performed, invalidate move
            sendMessage(sock, "Musisz wykonac bicie!");
            return 0;
        }
    } else if (moveType == 1) { //no captures available
        //make regular move
        board[destRow][destCol] = board[srcRow][srcCol];
        board[srcRow][srcCol] = 'o';
        //check for possible promotion
        PromoteToKing(board, destRow, destCol);
        //swap turns
        *currentPlayer = (*currentPlayer + 1) % 2;
        return 1; //successful move
    }

    sendMessage(sock, "Nieprawidlowy ruch.");
    return 0; //invalidate move
}

//send a message to the client, i.e. the board
void sendMessage(int sock, const char* msg) {
    send(sock, msg, strlen(msg), 0);
}

void *connection_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    int foundGame = -1;

    //find an existing game or initialize new one
    for (int i = 0; i < MAX_GAMES; i++) {
        if (!games[i].active || (games[i].active && games[i].sockets[1] != 0)) {
            //this game is full, try the next one
            continue;
        }
        if (games[i].sockets[0] == 0) {
            games[i].sockets[0] = sock;
            foundGame = i;
            break;
        } else if (games[i].sockets[1] == 0) {
            games[i].sockets[1] = sock;
            games[i].active = 1;
            CreateTable(games[i].board);
            foundGame = i;
            break;
        }
    }

    //if nothing was found, initialize new game
    if (foundGame == -1) {
        for (int i = 0; i < MAX_GAMES; i++) {
            if (!games[i].active) {
                games[i].sockets[0] = sock;
                games[i].active = 1;
                CreateTable(games[i].board);
                foundGame = i;
                break;
            }
        }
    }

    if (foundGame == -1) {
        printf("Serwer jest zajety lub wystapil inny blad.\n");
        close(sock);
        return NULL;
    }

    //wait for second player
    while (games[foundGame].sockets[1] == 0) {
        usleep(100000);
    }

    //game started, notify both players
    if (games[foundGame].sockets[0] > 0 && games[foundGame].welcome[0] == 0) {
        send(games[foundGame].sockets[0], "Witaj graczu 1\n", strlen("Witaj graczu 1\n"), 0);
        games[foundGame].welcome[0] = 1;
    }
    if (games[foundGame].sockets[1] > 0 && games[foundGame].welcome[1] == 0) {
        send(games[foundGame].sockets[1], "Witaj graczu 2\n", strlen("Witaj graczu 2\n"), 0);
        games[foundGame].welcome[1] = 1;
    }

    GameState* currentGame = &games[foundGame];
    char buffer[1024] = {0};
    while (currentGame->active) {
            memset(buffer, 0, sizeof(buffer)); //clear buffer
	    int valread = read(sock, buffer, 1023);
	    if (valread > 0) {
		buffer[valread] = '\0'; //null-termination.

		//make a move only if it's the right turn
		if (sock == currentGame->sockets[currentGame->currentPlayer]) {
		    int moveResult = MakeMove(sock, currentGame->board, buffer, &currentGame->currentPlayer);

		    if (moveResult == 0) {
		        //notify server about invalid move, print board state to verify
		        printf("Wykonano nieprawidlowy ruch\n");
		    } else {
		        //send updated board
		        char* updatedBoard = ShowBoard(currentGame->board);
		        for (int i = 0; i < 2; ++i) {
		            send(currentGame->sockets[i], updatedBoard, strlen(updatedBoard), 0);
		        }
		        free(updatedBoard);
		        if (CheckWin(currentGame->board, currentGame->currentPlayer)) {
				char winMsg[32];
        			sprintf(winMsg, "Gracz %d zwyciezyl!\n", currentGame->currentPlayer);
        			for (int i = 0; i < 2; ++i) {
        			    sendMessage(currentGame->sockets[i], winMsg);
        			}
			    } else {
				char turnMsg[32];
				sprintf(turnMsg, "Rozpoczela sie tura gracza %d.\n", currentGame->currentPlayer + 1);
				for (int i = 0; i < 2; ++i) {
				    sendMessage(currentGame->sockets[i], turnMsg);
				}
			    }
		    }
		} else {
		    //when wrong player tries to move
		    sendMessage(sock, "To nie jest twoja tura. Poczekaj na swoja kolej.");
		}
		    } else if (valread == 0) {
		        printf("Rozlaczono gracza\n");
		        currentGame->active = 0; //end the game.
		        break;
		    }

	     usleep(100000); // 100 ms
    }
    
    //clean
    close(sock);
    for (int i = 0; i < 2; ++i) {
        if (sock == currentGame->sockets[i]) currentGame->sockets[i] = 0;
    }

    return NULL;
}

int main() {
    int server_fd, new_socket, *new_sock;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    //setup connection
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Serwer jest aktywny na porcie %d\n", PORT);
    
    //accept connection, create new thread, place it into connection_handler
    while(1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            continue;
        }
        
        printf("Zaakceptowano nowe polaczenie\n");
        
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;
        
        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*)new_sock) < 0) {
            perror("could not create thread");
            return 1;
        }
        
        pthread_detach(sniffer_thread);
    }
    
    close(server_fd);
    return 0;
}
