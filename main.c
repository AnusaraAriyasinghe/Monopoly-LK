#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "types.h"

int main(void) {
    srand(time(NULL));

    sqr board[40];
    intBoard(board);

    economicState econ;
    initEconomicState(&econ, board);

    player players[4];
    initPlayers(players);

    printf("=============================================\n");
    printf(" MONOPOLY-LK Simulation\n");
    printf("=============================================\n");
    for (int i = 0; i < 4; i++) {
        printf("Player %d : %s\n", i + 1, players[i].name);
    }
    printf("\nEach player begins with LKR %d.\n\n", players[0].cash);

    printf("--- Determining the First Player ---\n");
    int order[4];
    determineTurnOrder(players, order);

    printf("%s will begin the game.\n\n", players[order[0]].name);
    printf("Turn order:\n");
    for (int i = 0; i < 4; i++) {
        printf("  %d. %s\n", i + 1, players[order[i]].name);
    }
    printf("\n");

    playGame(players, order, board, 500, &econ);

    return 0;
}
