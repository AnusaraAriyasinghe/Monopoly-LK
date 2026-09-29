#include <string.h>
#include "types.h"

void initPlayers(player *players){
    strncpy(players[0].name, "Aggressive Investor", sizeof(players[0].name)-1);
    players[0].name[sizeof(players[0].name)-1] = '\0';
    players[0].strat='A';

    strncpy(players[1].name, "Conservative Banker", sizeof(players[1].name)-1);
    players[1].name[sizeof(players[1].name)-1] = '\0';
    players[1].strat='C';

    strncpy(players[2].name, "Risk Taker", sizeof(players[2].name)-1);
    players[2].name[sizeof(players[2].name)-1] = '\0';
    players[2].strat='R';

    strncpy(players[3].name, "Opportunistic Trader", sizeof(players[3].name)-1);
    players[3].name[sizeof(players[3].name)-1] = '\0';
    players[3].strat='O';

    for (int i = 0; i < 4; i++) {
        players[i].cash = 30000;
        players[i].position = 0;
        players[i].inJail = 0;
        players[i].jailTurns = 0;
        players[i].rounds = 0;
        players[i].loanAmount = 0;
        players[i].loanInterestRate = 0;
        players[i].loanRoundsRemaining = 0;
        players[i].railwaysOwned = 0;
        players[i].utilitiesOwned = 0;
        players[i].isBankrupt = 0;
    }
}

int decidePurchase(player *p, sqr *sq){
    if (p->cash < sq->buyPrice) return 0;
    switch (p->strat){
        case 'A': 
            return (p->cash - sq->buyPrice) >= sq->rent;
        case 'C': 
            return (p->cash - sq->buyPrice) >= p->cash / 2;
        case 'R': 
            return 1;
        case 'O':
            return (sq->rent * 7) >= sq->buyPrice;  //Not clearly defined : Used - If rentPrice with 4 houses exceeds tha value of buyPrice then Buy
        default:
            return 1;
    }
}

int getMaxAuctionBid(player *p, sqr *sq){
    int ceiling;
    switch (p->strat){
        case 'A':
            ceiling = (sq->buyPrice * 120) / 100;
            break;
        case 'C': 
            ceiling = sq->buyPrice - 1;
            if (ceiling > p->cash / 2) {
                ceiling = p->cash / 2;
            }
            break;
        case 'R': 
            ceiling = p->cash;
            break;
        case 'O': 
            ceiling = (sq->buyPrice * 70) / 100;
            break;
        default:
            ceiling = sq->buyPrice;
            break;
    }
    if (ceiling > p->cash) ceiling = p->cash;
    if (ceiling < 0) ceiling = 0;
    return ceiling;
}

loanAction decideLoanAction(player *p, int maxLoan){
    if (p->loanAmount == 0){
        switch (p->strat){
            case 'A': //?? 
                if(maxLoan>0){
                    return LOAN_ACTION_TAKE;
                }else{ 
                    return LOAN_ACTION_NONE;
                }
            case 'C': // bankruptcy imminent
                if(maxLoan > 0 && p->cash < 3000){
                    return LOAN_ACTION_TAKE;
                }else{ 
                    return LOAN_ACTION_NONE;
                }
            case 'R': 
                if(maxLoan > 0){
                    return LOAN_ACTION_TAKE;
                }else{ 
                    return LOAN_ACTION_NONE;
                }
            case 'O': //?? Can't calc expected income so went with cash < maxLoan
                if(maxLoan > 0 && p->cash < maxLoan){
                    return LOAN_ACTION_TAKE;
                }else{ 
                    return LOAN_ACTION_NONE;
                }
            default:
                if(maxLoan > 0 && p->cash < 8000){
                    return LOAN_ACTION_TAKE;
                }else{ 
                    return LOAN_ACTION_NONE;
                }
                
        }
    }
    if (p->loanAmount != 0){
        switch (p->strat){
            case 'A':
                if (p->cash > 2 * p->loanAmount) return LOAN_ACTION_REPAY_FULL;
                if (maxLoan > 0) return LOAN_ACTION_TOPUP;
                return LOAN_ACTION_NONE;
            case 'C': 
                if (p->cash >= p->loanAmount) return LOAN_ACTION_REPAY_FULL;
                if (p->cash >= 2000) return LOAN_ACTION_REPAY_PARTIAL;
                if (p->loanRoundsRemaining <= 5) return LOAN_ACTION_REFINANCE;
                return LOAN_ACTION_NONE;
            case 'R': 
                if (maxLoan > 0) return LOAN_ACTION_TOPUP;
                if (p->loanRoundsRemaining <= 5) return LOAN_ACTION_REFINANCE;
                return LOAN_ACTION_NONE;
            case 'O': 
                if (p->cash >= p->loanAmount) return LOAN_ACTION_REPAY_FULL;
                if (p->cash >= p->loanAmount / 2) return LOAN_ACTION_REPAY_PARTIAL;
                if (p->loanRoundsRemaining <= 5) return LOAN_ACTION_REFINANCE;
                return LOAN_ACTION_NONE;
            default:
                if (p->cash >= p->loanAmount) return LOAN_ACTION_REPAY_FULL;
                return LOAN_ACTION_NONE;
        }
    }
}

int decideConstruction(player *p, int cost){
    if (p->cash < cost) return 0;

    switch (p->strat){
        case 'A': 
            return 1;
        case 'C': //purchase logic
            return (p->cash - cost) >= p->cash / 2;
        case 'R': 
            return 1;
        case 'O': //should take subsidy into acc
            return (p->cash - cost) >= p->cash / 2;
        default:
            return 1;
    }
}
//rule: never allows C to build hotels unless loanamont is 0
int allowsHotelUpgrade(player *p){
    if (p->strat == 'C') return p->loanAmount == 0;
    return 1;
}

int decideRenovation(player *p, int depreciationP, int structuralDamage){
    if (structuralDamage) return 1;

    switch (p->strat){
        case 'A': //val not defined
            return depreciationP > 25;
        case 'C': 
            return depreciationP > 10;
        case 'R': 
            return depreciationP >= 30;
        case 'O': 
            return depreciationP > 15;
        default:
            return depreciationP > 15;
    }
}

int decideMaintenance(player *p, int cost){
    if (p->cash < cost) return 0;

    switch (p->strat){
        case 'A': 
            return 1;
        case 'C': //50% cash 
            return (p->cash - cost) >= p->cash / 2;
        case 'R': 
            return 1;
        case 'O': 
            return (p->cash - cost) >= p->cash / 3;
        default:
            return 1;
    }
}

int decideInsurancePolicy(player *p, sqr *sq){
    int developed = (sq->houses > 0 || sq->hasHotels);

    switch (p->strat){
        case 'A': 
            if (sq->hasHotels) return POLICY_COMPREHENSIVE;
            if (sq->houses > 0) return POLICY_BASIC;
        case 'C': 
            if(developed) return POLICY_COMPREHENSIVE;
        case 'R': 
            if(sq->hadDisaster) return POLICY_COMPREHENSIVE;
        case 'O':
            if(developed && sq->buyPrice >= 6000) return POLICY_COMPREHENSIVE;
        default:
            return POLICY_NONE;
    }
}
