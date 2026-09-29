#include <stdio.h>
#include <stdlib.h>
#include "types.h"

#define INCOME_TAX_RATE 10

int rolldice(){
    int d1 = (rand() % 6) + 1;
    int d2 = (rand() % 6) + 1;
    int dice_roll=d1+d2;

    return dice_roll;
}

static int rollDiceDoubles(int *isDouble){
    int d1 = (rand() % 6) + 1;
    int d2 = (rand() % 6) + 1;
    *isDouble = (d1 == d2);
    return d1 + d2;
}

void determineTurnOrder(player *players, int *order) {
    int rolls[4];
    int assigned[4] = {0}; 

    for (int i = 0; i < 4; i++) {
        rolls[i] = rolldice();
        printf("%s rolls %d.\n", players[i].name, rolls[i]);
    }
    int current_spot = 0;
    while (current_spot < 4) {
        int max_roll = -1;
        for (int i = 0; i < 4; i++) {
            if (!assigned[i] && rolls[i] > max_roll) {
                max_roll = rolls[i];
            }
        }
        int group[4];
        int group_size = 0;
        for (int i = 0; i < 4; i++) {
            if (!assigned[i] && rolls[i] == max_roll) {
                group[group_size++] = i;
            }
        }
        int group_rolls[4];
        for (int i = 0; i < group_size; i++) {
            group_rolls[i] = rolls[group[i]]; 
        }
        int group_assigned_count = 0;
        while (group_assigned_count < group_size) {

            int sub_max = -1;
            for (int i = 0; i < group_size; i++) {
                if (group_rolls[i] > sub_max) {
                    sub_max = group_rolls[i];
                }
            }
            int sub_tied_count = 0;
            for (int i = 0; i < group_size; i++) {
                if (group_rolls[i] == sub_max) {
                    sub_tied_count++;
                }
            }
            if (sub_tied_count > 1) {
                printf("Tiebreaker needed among tied leaders...\n");
                for (int i = 0; i < group_size; i++) {
                    if (group_rolls[i] == sub_max) {
                        group_rolls[i] = rolldice();
                        printf("%s rerolls %d.\n", players[group[i]].name, group_rolls[i]);
                    }
                }
            } else {
                for (int i = 0; i < group_size; i++) {
                    if (group_rolls[i] == sub_max) {
                        int winner = group[i];
                        order[current_spot++] = winner;
                        assigned[winner] = 1;
                        group_rolls[i] = -1; 
                        group_assigned_count++;
                        break;
                    }
                }
            }
        }
    }
}

void runAuction(player *players, sqr *board, int sqId, int *order, economicState *econ) {
    sqr *sq = &board[sqId];
    printf("\n--- AUCTION START: %s (Market Value: LKR %d) ---\n", sq->name, sq->buyPrice);

    int startingBid = applyPercent(sq->buyPrice / 2, getEffectivePercent(econ, board, sqId, VALTYP_AUCTION_START, -1));
    int currentBid = 0;
    int currentHighestBidder = -1;

    // Rule 6: auctions involve every player except bankrupt players.
    int active[4];
    int activeCount = 0;
    for (int i = 0; i < 4; i++) {
        active[i] = !players[i].isBankrupt;
        if (active[i]) activeCount++;
    }

    int turnIdx = 0;

    while (activeCount > 1 || (activeCount == 1 && currentHighestBidder == -1)) {
        int pid = order[turnIdx % 4];

        if (active[pid]) {
            int minRequiredBid = (currentHighestBidder == -1) ? startingBid : currentBid + 250;

            if (players[pid].cash >= minRequiredBid && minRequiredBid <= getMaxAuctionBid(&players[pid], sq)) {
                currentBid = minRequiredBid;
                currentHighestBidder = pid;
                printf("  %s bids LKR %d.\n", players[pid].name, currentBid);
            } else {
                printf("  %s withdraws.\n", players[pid].name);
                active[pid] = 0;
                activeCount--;
            }
        }

        turnIdx++;

        if (activeCount == 0 && currentHighestBidder == -1) {
            break;
        }
    }

    if (currentHighestBidder != -1) {
        printf("--- %s wins the auction for LKR %d! ---\n", players[currentHighestBidder].name, currentBid);
        players[currentHighestBidder].cash -= currentBid;
        sq->owner = currentHighestBidder;
        if (sq->type == RAILWAY) {
            players[currentHighestBidder].railwaysOwned++;
        } else if (sq->type == UTILITY) {
            players[currentHighestBidder].utilitiesOwned++;
        }
        printf("%s's remaining Balance : LKR %d.\n", players[currentHighestBidder].name, players[currentHighestBidder].cash);
        evaluateConstruction(players, board, currentHighestBidder, econ);
    } else {
        printf("--- No bids were placed. %s stays with the Bank. ---\n", sq->name);
    }
}

static int ownsMonopoly(sqr *board, sqr_grp g, int pid){
    int anyInGroup = 0;
    for (int i = 0; i < 40; i++) {
        if (board[i].type == PROPERTY && board[i].group == g) {
            anyInGroup = 1;
            if (board[i].owner != pid) return 0;
        }
    }
    return anyInGroup;
}

void evaluateConstruction(player *players, sqr *board, int pid, economicState *econ){
    player *p = &players[pid];

    if (isConstructionSuspended(econ, pid)) return; 

    for (sqr_grp g = Orange; g < NA; g++) {
        if (!ownsMonopoly(board, g, pid)) continue;

        int built = 1;
        while (built) {
            built = 0;

            int allHotel = 1, localMin = 4, targetId = -1;
            for (int i = 0; i < 40; i++) {
                if (board[i].type == PROPERTY && board[i].group == g && !board[i].hasHotels) {
                    allHotel = 0;
                    if (board[i].houses < localMin) localMin = board[i].houses;
                }
            }
            if (allHotel) break;

            if (localMin == 4) {
                if (!allowsHotelUpgrade(p)) break;
                for (int i = 0; i < 40; i++) {
                    if (board[i].type == PROPERTY && board[i].group == g &&
                        board[i].houses == 4 && !board[i].hasHotels) {
                        targetId = i;
                        break;
                    }
                }
                sqr *sq = &board[targetId];
                int hotelCost = applyPercent(sq->hotelPrice, getEffectivePercent(econ, board, targetId, VALTYP_CONSTRUCTION, pid));
                if (!decideConstruction(p, hotelCost)) break;

                p->cash -= hotelCost;
                sq->houses = 0;
                sq->hasHotels = 1;
                sq->buildingCondition = 100;
                sq->roundsSinceMaintenance = 0;
                sq->structuralDamage = 0;
                printf("%s converts %s's houses into a Hotel for LKR %d. Remaining Balance : LKR %d.\n",
                       p->name, sq->name, hotelCost, p->cash);
                built = 1;
            } else {
                for (int i = 0; i < 40; i++) {
                    if (board[i].type == PROPERTY && board[i].group == g &&
                        !board[i].hasHotels && board[i].houses == localMin) {
                        targetId = i;
                        break;
                    }
                }
                sqr *sq = &board[targetId];
                int houseCost = applyPercent(sq->housePrice, getEffectivePercent(econ, board, targetId, VALTYP_CONSTRUCTION, pid));
                if (!decideConstruction(p, houseCost)) break;

                p->cash -= houseCost;
                sq->houses++;
                sq->buildingCondition = 100;
                sq->roundsSinceMaintenance = 0;
                sq->structuralDamage = 0;
                printf("%s constructs a house on %s (now %d house%s) for LKR %d. Remaining Balance : LKR %d.\n",
                       p->name, sq->name, sq->houses, sq->houses == 1 ? "" : "s", houseCost, p->cash);
                built = 1;
            }
        }
    }
}

void handleLanding(player *players, sqr *board, int id, int diceRoll, int *order, economicState *econ){
    sqr *sq = &board[players[id].position];
    int sqrID = players[id].position;

    switch (sq->type) {
        case PROPERTY:
            if (sq->owner == -1){
                int effBuyPrice = applyPercent(sq->buyPrice, getEffectivePercent(econ, board, sqrID, VALTYP_PURCHASE, -1));
                sqr effSq = *sq;
                effSq.buyPrice = effBuyPrice;
                effSq.rent = applyPercent(sq->rent, getEffectivePercent(econ, board, sqrID, VALTYP_RENT, -1));
                int antiSpeculationBlocked = 0;
                if (econ->activeRegulation == REG_ANTI_SPECULATION_ACT) {
                    int undeveloped = 0;
                    for (int i = 0; i < 40; i++) {
                        if (board[i].type == PROPERTY && board[i].owner == id &&
                            board[i].houses == 0 && !board[i].hasHotels) {
                            undeveloped++;
                        }
                    }
                    if (undeveloped >= 3) {
                        antiSpeculationBlocked = 1;
                        printf("* Anti-Speculation Act: %s already holds 3 undeveloped properties and may not acquire more.\n", players[id].name);
                    }
                }
                if(!antiSpeculationBlocked && decidePurchase(&players[id], &effSq)){
                    players[id].cash -= effBuyPrice;
                    sq->owner = id;
                    printf("%s purchased %s for LKR %d.\n", players[id].name, sq->name, effBuyPrice);
                    printf("Remaining Balance : LKR %d.\n", players[id].cash);
                    evaluateConstruction(players, board, id, econ);
                } else {
                    if (!antiSpeculationBlocked) {
                        printf("%s cannot afford %s (LKR %d) and declines to purchase.\n", players[id].name, sq->name, effBuyPrice);
                    }
                    runAuction(players, board, players[id].position, order, econ);
                }
            }else if(sq->owner!=-1 && sq->owner!=id){
                if (isSquareClosed(econ, sqrID)) {
                    printf("* %s is closed this round (Political Rally) — no rent collected.\n", sq->name);
                } else if (sq->mortgaged) {
                    printf("* %s is mortgaged — no rent collected (Rule 7).\n", sq->name);
                } else {
                    int baseRent = applyPercent(sq->rent, getEffectivePercent(econ, board, sqrID, VALTYP_RENT, sq->owner));
                    int rentDue;
                    if(sq->houses==0){
                        if(sq->hasHotels){
                            rentDue = baseRent*10;
                        }else{
                            rentDue = baseRent;
                        }
                    }else if (sq->houses==1){
                        rentDue = baseRent*2;
                    }else if (sq->houses==2){
                        rentDue = baseRent*3;
                    }else if (sq->houses==3){
                        rentDue = baseRent*5;
                    }else if (sq->houses==4){
                        rentDue = baseRent*7;
                    }else{
                            rentDue = baseRent*10;
                    }

                    int deprPct = propertyDepreciationPercent(sq);
                    if (deprPct > 0) rentDue = applyPercent(rentDue, -deprPct);

                    int buildingClosed = 0;
                    if (sq->houses > 0 || sq->hasHotels) {
                        int condPct = buildingConditionRentPercent(sq);
                        if (condPct == 0) {
                            buildingClosed = 1;
                        } else {
                            rentDue = (rentDue * condPct) / 100;
                            if (sq->structuralDamage) rentDue = applyPercent(rentDue, -25);
                        }
                    }

                    if (buildingClosed) {
                        printf("* %s's building is closed (condition below 25%%) — no rent collected until maintenance is completed.\n", sq->name);
                    } else {
                        int ownerId = sq->owner;
                        printf("%s pays LKR %d rent to %s.\n", players[id].name, rentDue, players[ownerId].name);
                        players[id].cash -= rentDue;
                        players[ownerId].cash += rentDue;
                        printf("%s's remaining Balance : LKR %d.\n", players[id].name, players[id].cash);
                        printf("%s earned LKR %d. New Balance : LKR %d.\n", players[ownerId].name, rentDue, players[ownerId].cash);
                        if (players[id].cash < 0) {
                            raiseCashByMortgaging(players, board, id, econ);
                            if (players[id].cash < 0) {
                                declareBankruptcy(players, board, id, order, econ);
                            }
                        }
                    }
                }
            }else{
                handleRenovationOpportunity(players, board, id, sqrID);
            }
            break;
        case RAILWAY:
            if (sq->owner == -1){
                int effBuyPrice = applyPercent(sq->buyPrice, getEffectivePercent(econ, board, sqrID, VALTYP_PURCHASE, -1));
                sqr effSq = *sq;
                effSq.buyPrice = effBuyPrice;
                if(decidePurchase(&players[id], &effSq)){
                    players[id].cash -= effBuyPrice;
                    sq->owner = id;
                    players[id].railwaysOwned++;
                    printf("%s purchased %s for LKR %d.\n", players[id].name, sq->name, effBuyPrice);
                    printf("Remaining Balance : LKR %d.\n", players[id].cash);
                } else {
                    printf("%s cannot afford %s (LKR %d) and declines to purchase.\n", players[id].name, sq->name, effBuyPrice);
                    runAuction(players, board, players[id].position, order, econ);
                }
            }else if (sq->owner!=-1 && sq->owner!=id && sq->mortgaged){
                printf("* %s is mortgaged — no rent collected (Rule 7).\n", sq->name);
            }else if (sq->owner!=-1 && sq->owner!=id){
                int rentDue;
                if(players[sq->owner].railwaysOwned==1){
                    rentDue=250;
                }else if(players[sq->owner].railwaysOwned==2){
                    rentDue=500;
                }else if(players[sq->owner].railwaysOwned==3){
                    rentDue=1000;
                }else if(players[sq->owner].railwaysOwned==4){
                    rentDue=2000;
                }
                rentDue = applyPercent(rentDue, getEffectivePercent(econ, board, sqrID, VALTYP_RENT, sq->owner));
                int railDeprPct = propertyDepreciationPercent(sq);
                if (railDeprPct > 0) rentDue = applyPercent(rentDue, -railDeprPct);
                int ownerId = sq->owner;
                printf("%s pays LKR %d in railway fees to %s.\n", players[id].name, rentDue, players[ownerId].name);
                players[id].cash -= rentDue;
                players[ownerId].cash += rentDue;
                printf("%s's remaining Balance : LKR %d.\n", players[id].name, players[id].cash);
                printf("%s earned LKR %d. New Balance : LKR %d.\n", players[ownerId].name, rentDue, players[ownerId].cash);
                if (players[id].cash < 0) {
                    raiseCashByMortgaging(players, board, id, econ);
                    if (players[id].cash < 0) {
                        declareBankruptcy(players, board, id, order, econ);
                    }
                }
            }else{
                printf("%s is passing through their own railway station.\n", players[id].name);
            }
            break;

        case UTILITY:
            if (sq->owner == -1) {
                int effBuyPrice = applyPercent(sq->buyPrice, getEffectivePercent(econ, board, sqrID, VALTYP_PURCHASE, -1));
                sqr effSq = *sq;
                effSq.buyPrice = effBuyPrice;
                if (decidePurchase(&players[id], &effSq)) {
                    players[id].cash -= effBuyPrice;
                    sq->owner = id;
                    players[id].utilitiesOwned++;
                    printf("%s purchased %s for LKR %d.\n", players[id].name, sq->name, effBuyPrice);
                    printf("Remaining Balance : LKR %d.\n", players[id].cash);
                } else {
                    printf("%s cannot afford %s (LKR %d) and declines to purchase.\n", players[id].name, sq->name, effBuyPrice);
                    runAuction(players, board, players[id].position, order, econ);
                }
            }else if (sq->owner!=-1 && sq->owner!=id && sq->mortgaged){
                printf("* %s is mortgaged — no rent collected (Rule 7).\n", sq->name);
            }else if (sq->owner!=-1 && sq->owner!=id){
                int rentDue;
                if(players[sq->owner].utilitiesOwned==1){
                    rentDue=4*diceRoll;
                }else if(players[sq->owner].utilitiesOwned==2){
                    rentDue=10*diceRoll;
                }
                rentDue = applyPercent(rentDue, getEffectivePercent(econ, board, sqrID, VALTYP_RENT, sq->owner));
                int utilDeprPct = propertyDepreciationPercent(sq);
                if (utilDeprPct > 0) rentDue = applyPercent(rentDue, -utilDeprPct);
                int ownerId = sq->owner;
                printf("%s pays LKR %d in utility fees to %s.\n", players[id].name, rentDue, players[ownerId].name);
                players[id].cash -= rentDue;
                players[ownerId].cash += rentDue;
                printf("%s's remaining Balance : LKR %d.\n", players[id].name, players[id].cash);
                printf("%s earned LKR %d. New Balance : LKR %d.\n", players[ownerId].name, rentDue, players[ownerId].cash);
                if (players[id].cash < 0) {
                    raiseCashByMortgaging(players, board, id, econ);
                    if (players[id].cash < 0) {
                        declareBankruptcy(players, board, id, order, econ);
                    }
                }
            }else{
                printf("%s is passing through their own utility.\n", players[id].name);
            }
            break;
        case TAX: {
            int taxRate = getEffectiveIncomeTaxRate(econ);
            int taxDue = players[id].cash * taxRate / 100;
            printf("%s lands on %s and owes LKR %d (%d%% of cash) in tax.\n", players[id].name, sq->name, taxDue, taxRate);
            players[id].cash -= taxDue;
            printf("%s's remaining Balance : LKR %d.\n", players[id].name, players[id].cash);
            if (players[id].cash < 0) {
                raiseCashByMortgaging(players, board, id, econ);
                if (players[id].cash < 0) {
                    declareBankruptcy(players, board, id, order, econ);
                }
            }
            break;
        }
        case JAIL:
            printf("%s is just visiting Jail.\n", players[id].name);
            break;
        case GOTOJAIL:
            printf("%s lands on Go To Jail and is sent directly to Jail.\n", players[id].name);
            players[id].position = 10;
            players[id].inJail = 1;
            players[id].jailTurns = 0;
            break;
        case BANK:
            handleBankLanding(players, id, board, econ);
            break;
        case INSURANCE:
            handleInsuranceLanding(players, board, id, econ);
            break;
        case EVENT:
            handleEventLanding(econ, players, board, id);
            break;
        default:
            break;
    }
}

int countBankruptPlayers(player *players){
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (!players[i].isBankrupt) count++;
    }
    return count;
}

static int countOwnedSquares(sqr *board, int pid){
    int count = 0;
    for (int i = 0; i < 40; i++) {
        if (board[i].owner == pid) count++;
    }
    return count;
}

static int countHotels(sqr *board, int pid){
    int count = 0;
    for (int i = 0; i < 40; i++) {
        if (board[i].owner == pid && board[i].hasHotels) count++;
    }
    return count;
}

static void printRoundSummary(player *players, sqr *board, int roundNum){
    printf("\n=============================================\n");
    printf("Round %d Summary\n", roundNum);
    printf("=============================================\n");
    for (int i = 0; i < 4; i++) {
        player *p = &players[i];
        if (p->isBankrupt) {
            printf("%s : BANKRUPT\n", p->name);
            printf("---------------------------------------------\n");
            continue;
        }
        printf("%s\n", p->name);
        printf("Cash : LKR %d\n", p->cash);
        printf("Net Worth : LKR %d\n", calculateNetWorth(players, board, i));
        printf("Properties : %d\n", countOwnedSquares(board, i));
        printf("Hotels : %d\n", countHotels(board, i));
        if (p->loanAmount > 0) {
            printf("Outstanding Loan : LKR %d\n", p->loanAmount);
        } else {
            printf("Outstanding Loan : None\n");
        }
        printf("---------------------------------------------\n");
    }
}

static int determineWinner(player *players, sqr *board){
    int winner = -1;
    int bestNetWorth = 0;
    for (int i = 0; i < 4; i++) {
        if (players[i].isBankrupt) continue;
        int nw = calculateNetWorth(players, board, i);
        if (winner == -1 || nw > bestNetWorth) {
            winner = i;
            bestNetWorth = nw;
        }
    }
    return winner;
}

static void printGameEnd(player *players, sqr *board, int endRound){
    printf("\n=============================================\n");
    printf("End of Game\n");
    printf("=============================================\n");
    printf("GAME OVER (after %d round%s)\n\n", endRound, endRound == 1 ? "" : "s");

    int winner = determineWinner(players, board);
    if (winner == -1) {
        printf("No solvent players remain. There is no winner.\n");
        printf("=============================================\n");
        return;
    }

    player *w = &players[winner];
    int propertyValue = calculateNetWorth(players, board, winner) - w->cash + w->loanAmount;

    printf("Winner\n%s\n\n", w->name);
    printf("Total Cash\nLKR %d\n\n", w->cash);
    printf("Total Property Value\nLKR %d\n\n", propertyValue);
    if (w->loanAmount > 0) {
        printf("Outstanding Loans\nLKR %d\n\n", w->loanAmount);
    } else {
        printf("Outstanding Loans\nNone\n\n");
    }
    printf("Net Worth\nLKR %d\n", calculateNetWorth(players, board, winner));
    printf("=============================================\n");
}

void playGame(player *players, int *order, sqr *board, int maxRounds, economicState *econ){
    int gameRound = 0;
    int gameEndedEarly = 0;

    while (gameRound < maxRounds){
        for (int i = 0; i < 4; i++) {
            int pid = order[i];

            if (players[pid].isBankrupt) continue;

            performMaintenance(players, board, pid, econ);
            evaluateConstruction(players, board, pid, econ);

            if (players[pid].inJail) {
                players[pid].jailTurns++;

                if (players[pid].jailTurns >= 3) {
                    printf("%s has served 3 turns in Jail and is released.\n", players[pid].name);
                    players[pid].inJail = 0;
                    players[pid].jailTurns = 0;

                    int roll = rolldice();
                    printf("%s rolls %d.\n", players[pid].name, roll);
                    movePlayer(players, pid, roll, board);
                    handleLanding(players, board, pid, roll, order, econ);
                } else {
                    int isDouble;
                    int roll = rollDiceDoubles(&isDouble);
                    printf("%s (in Jail) rolls %d.\n", players[pid].name, roll);

                    if (isDouble) {
                        printf("%s rolled doubles and is released from Jail!\n", players[pid].name);
                        players[pid].inJail = 0;
                        players[pid].jailTurns = 0;
                        movePlayer(players, pid, roll, board);
                        handleLanding(players, board, pid, roll, order, econ);
                    } else {
                        printf("%s did not roll doubles (attempt %d/3) and remains in Jail.\n", players[pid].name, players[pid].jailTurns);
                    }
                }
            } else {
                int roll = rolldice();
                printf("%s rolls %d.\n", players[pid].name, roll);
                movePlayer(players, pid, roll, board);
                handleLanding(players, board, pid, roll, order, econ);
            }

            if (!players[pid].isBankrupt) {
                handleMortgageDecisions(players, board, pid, econ);
            }

            printf("\n---------------------------------------------\n\n");

            if (countBankruptPlayers(players) <= 1) {
                gameEndedEarly = 1;
                break;
            }
        }

        if (gameEndedEarly) break;

        int minRound = 1000;
        for (int j = 0; j < 4; j++) {
            if (players[j].isBankrupt) continue;
            if (players[j].rounds < minRound) {
                minRound = players[j].rounds;
            }
        }
        if(gameRound!=minRound){
            printf("\n========== Round %d Begins ==========\n\n",minRound);
            calInflation(econ, minRound);
            calMarket(econ, minRound);
            checkRegulation(econ, players, board, minRound);
            checkRegionalCard(econ, minRound);
            checkNationalCardEffects(econ, players);
            calPropertyAge(board);
            checkBuildingCondition(board);
            checkForDisaster(players, board, minRound, order, econ);
            printMarketConditionsReport(econ, board);
            printRoundSummary(players, board, minRound);

            if (countBankruptPlayers(players) <= 1) {
                gameEndedEarly = 1;
                break;
            }
        }
        gameRound = minRound;
    }

    printGameEnd(players, board, gameRound);
}