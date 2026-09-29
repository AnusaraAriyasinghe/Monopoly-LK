#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"

sqr_typ parseSquareType(char* typeStr, char* name){
    if (strcmp(typeStr, "PROPERTY")== 0)return PROPERTY;
    if (strcmp(typeStr, "RAILWAY")== 0)return RAILWAY;
    if (strcmp(typeStr, "UTILITY")== 0)return UTILITY;
    if (strcmp(typeStr, "BANK")== 0)return BANK;
    if (strcmp(typeStr, "INSURANCE")== 0)return INSURANCE;
    if (strcmp(typeStr, "EVENT")== 0)return EVENT;
    if (strcmp(typeStr, "TAX")== 0)return TAX;
    if (strcmp(typeStr, "START")== 0)return START;
    if (strcmp(typeStr, "SPECIAL")== 0){
        if (strcmp(name, "Go To Jail")== 0)return GOTOJAIL;
        if (strcmp(name, "Free Parking")== 0)return FREEPARKING;
        return JAIL;
    }
}

sqr_grp parseSquareGroup(char* groupStr, char* name){
    if (strcmp(groupStr, "Orange")== 0)return Orange;
    if (strcmp(groupStr, "LightBlue")== 0)return LightBlue;
    if (strcmp(groupStr, "Brown")== 0)return Brown;
    if (strcmp(groupStr, "Red")== 0)return Red;
    if (strcmp(groupStr, "Yellow")== 0)return Yellow;
    if (strcmp(groupStr, "DarkBlue")== 0)return DarkBlue;
    if (strcmp(groupStr, "Green")== 0)return Green;
    if (strcmp(groupStr, "Pink")== 0)return Pink;
    if (strcmp(groupStr, "NA")== 0)return NA;
    
}

const char* squareTypeToString(sqr_typ t) {
    switch (t) {
        case START: return "START";
        case PROPERTY: return "PROPERTY";
        case RAILWAY: return "RAILWAY";
        case UTILITY: return "UTILITY";
        case BANK: return "BANK";
        case INSURANCE: return "INSURANCE";
        case EVENT: return "EVENT";
        case TAX: return "TAX";
        case JAIL: return "JAIL";
        case GOTOJAIL: return "GOTOJAIL";
        case FREEPARKING: return "FREEPARKING";
    }
    return "NA";
}

const char* squareGroupToString(sqr_grp p) {
    switch (p) {
        case Orange: return "Orange";
        case LightBlue: return "LightBlue";
        case Brown: return "Brown";
        case Red: return "Red";
        case Yellow: return "Yellow";
        case DarkBlue: return "DarkBlue";
        case Green: return "Green";
        case Pink: return "Pink"; 
    }
    return "NA";
}

void intBoard(sqr *board){
    FILE *fptr = fopen("Rent.csv","r");

    if (fptr==NULL){
        printf("Error: File couldn't be opened");
        exit(1);
    }
    char line[256];

    fgets(line, sizeof(line), fptr);

    int i = 0;
    while (fgets(line, sizeof(line), fptr) != NULL && i < 40 ){

        line[strcspn(line, "\r\n")] = '\0';
        
        char* typeStr = strtok(line, ",");
        char* name = strtok(NULL, ",");
        char* priceStr = strtok(NULL, ",");
        char* rentStr = strtok(NULL, ",");
        char* groupStr = strtok(NULL, ",");
        char* houseStr = strtok(NULL, ",");
        char* hotelStr = strtok(NULL, ",");
        
        board[i].position = i;
        strncpy(board[i].name, name, sizeof(board[i].name)-1);
        board[i].name[sizeof(board[i].name) -1] = '\0';
        board[i].type = parseSquareType(typeStr, name);
        board[i].buyPrice = atoi(priceStr);
        board[i].rent = atoi(rentStr);
        board[i].group = parseSquareGroup(groupStr, name);
        board[i].housePrice = atoi(houseStr);
        board[i].hotelPrice = atoi(hotelStr);
        board[i].owner = -1;
        board[i].houses = 0;
        board[i].hasHotels = 0;
        board[i].mortgaged = 0;
        board[i].mortagePrice = board[i].buyPrice/2;
        board[i].loanLocked= 0;
        board[i].age = 0;
        board[i].insureType = 0;
        board[i].insuranceRoundsRemaining = 0;
        board[i].hadDisaster = 0;
        board[i].buildingCondition = 100;
        board[i].roundsSinceMaintenance = 0;
        board[i].structuralDamage = 0;

        i++;
    }
    fclose(fptr);
}

void movePlayer(player *players, int id, int roll, sqr *board){
    if (players[id].position + roll > 39) {
        players[id].cash += 2000;
        printf("%s passes GO and collects LKR 2,000.\n", players[id].name);
        printf("Current Balance : LKR %d.\n", players[id].cash);
        players[id].rounds++;

        applyLoanRoundEnd(players, id, board);
        applyInsuranceRoundEnd(players, id, board);
    }
    printf("\n%s moves from Square %d to Square %d.\n", players[id].name, players[id].position, (players[id].position + roll) % 40);
    players[id].position = (players[id].position + roll) % 40;
}
