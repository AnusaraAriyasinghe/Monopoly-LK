#include <stdio.h>
#include <stdlib.h>
#include "types.h"

#define INSURANCE_DURATION 20

static int mortgageEligible(sqr *sq, int pid){
    return (sq->owner == pid && !sq->mortgaged && !sq->loanLocked && sq->houses == 0 && !sq->hasHotels && (sq->type == PROPERTY || sq->type == RAILWAY || sq->type == UTILITY));
}

static int mortgageValueOf(sqr *board, int sqrID, economicState *econ, int pid){
    return applyPercent(board[sqrID].mortagePrice, getEffectivePercent(econ, board, sqrID, VALTYP_MORTGAGE, pid));
}

static void mortgageSquare(player *players, sqr *board, int sqrId, economicState *econ){
    sqr *sq = &board[sqrId];
    player *p = &players[sq->owner];
    int value = mortgageValueOf(board, sqrId, econ, sq->owner);

    sq->mortgaged = 1;
    p->cash += value;
    printf("* %s mortgages %s to the Bank for LKR %d. New Cash Balance: LKR %d.\n",
           p->name, sq->name, value, p->cash);
}

//Pays an additional when unmortgage
#define UNMORTGAGE_FEE_PERCENT 10

int unmortgageSquare(player *players, sqr *board, int sqrId, economicState *econ){
    sqr *sq = &board[sqrId];
    if (!sq->mortgaged || sq->owner == -1) return 0;

    player *p = &players[sq->owner];
    int value = mortgageValueOf(board, sqrId, econ, sq->owner);
    int redemption = value + (value * UNMORTGAGE_FEE_PERCENT) / 100;

    if (p->cash < redemption) return 0;

    p->cash -= redemption;
    sq->mortgaged = 0;
    printf("* %s pays LKR %d to lift the mortgage on %s. Remaining Cash Balance: LKR %d.\n",
           p->name, redemption, sq->name, p->cash);
    return 1;
}

void handleMortgageDecisions(player *players, sqr *board, int pid, economicState *econ){
    player *p = &players[pid];
    for (int i = 0; i < 40; i++) {
        if (board[i].owner != pid || !board[i].mortgaged) continue;

        int value = mortgageValueOf(board, i, econ, pid);
        int redemption = value + (value * UNMORTGAGE_FEE_PERCENT) / 100;

        if (p->cash - redemption >= 1000) { //Players only unmortage if they have more than 1000 after unmortaging
            unmortgageSquare(players, board, i, econ);
        }
    }
}

void raiseCashByMortgaging(player *players, sqr *board, int pid, economicState *econ){
    player *p = &players[pid];
    while (p->cash < 0) {
        int bestID = -1, bestVal = -1;
        for (int i = 0; i < 40; i++) {
            if (!mortgageEligible(&board[i], pid)) continue;
            int value = mortgageValueOf(board, i, econ, pid);
            if (value > bestVal) {
                bestVal = value;
                bestID = i;
            }
        }
        if (bestID == -1) break;
        mortgageSquare(players, board, bestID, econ);
    }
}

int calculateMaxLoan(player *players, sqr *board, int pid, economicState *econ){
    int totalmortageVal = 0;
    for(int i = 0; i < 40; i++){
        if(board[i].owner == pid && !board[i].mortgaged && !board[i].loanLocked){
            if(board[i].type == RAILWAY || board[i].type == UTILITY || board[i].type == PROPERTY){
                totalmortageVal += applyPercent(board[i].mortagePrice,
                    getEffectivePercent(econ, board, i, VALTYP_MORTGAGE, pid));
            }
        }
    }
    return (totalmortageVal * 75) / 100;
}

static void lockEligibleCollateral(player *players, sqr *board, int pid){
    for (int i = 0; i < 40; i++) {
        if (board[i].owner == pid && !board[i].mortgaged && !board[i].loanLocked) {
            if (board[i].type == PROPERTY || board[i].type == RAILWAY || board[i].type == UTILITY){
                board[i].loanLocked = 1;
            }
        }
    }
}

static void unlockAllCollateral(player *players, sqr *board, int pid){
    for (int i = 0; i < 40; i++) {
        if (board[i].owner == pid && board[i].loanLocked) {
            board[i].loanLocked = 0;
        }
    }
}

static void foreclose(player *players, int pid, sqr *board){
    player *p = &players[pid];
    printf("\n--- Loan Default: %s failed to repay within the loan duration. ---\n", p->name);

    int hadOtherAssets = 0;
    for (int i = 0; i < 40; i++) {
        if (board[i].owner == pid) {
            if (board[i].loanLocked) {
                printf("  Foreclosed: %s (transferred to Bank).\n", board[i].name);
                if (board[i].type == RAILWAY) {
                    p->railwaysOwned--;
                } else if (board[i].type == UTILITY) {
                    p->utilitiesOwned--;
                }
                board[i].owner = -1;
                board[i].houses = 0;
                board[i].hasHotels = 0;
                board[i].insureType = 0;
                board[i].mortgaged = 0;
                board[i].loanLocked = 0;
                board[i].age = 0;
                board[i].buildingCondition = 100;
                board[i].roundsSinceMaintenance = 0;
                board[i].structuralDamage = 0;
            } else {
                hadOtherAssets = 1;
            }
        }
    }

    printf("  Outstanding debt of LKR %d has been cleared.\n", p->loanAmount);
    p->loanAmount = 0;
    p->loanInterestRate = 0;
    p->loanRoundsRemaining = 0;

    if (!hadOtherAssets) {
        printf("  %s has no remaining assets and is declared BANKRUPT.\n", p->name);
        p->isBankrupt = 1;
        if (p->cash < 0) p->cash = 0;
    }
    printf("---------------------------------------------\n\n");
}

void declareBankruptcy(player *players, sqr *board, int pid, int *order, economicState *econ){
    player *p = &players[pid];
    if (p->isBankrupt) return;

    printf("\n--- %s has been declared bankrupt. ---\n", p->name);

    p->loanAmount = 0;
    p->loanInterestRate = 0;
    p->loanRoundsRemaining = 0;
    p->isBankrupt = 1;
    p->cash = 0;

    int assetId[40];
    int assetCount = 0;
    for (int i = 0; i < 40; i++) {
        if (board[i].owner != pid) continue;

        board[i].houses = 0;
        board[i].hasHotels = 0;
        board[i].insureType = POLICY_NONE;
        board[i].insuranceRoundsRemaining = 0;
        board[i].mortgaged = 0;
        board[i].loanLocked = 0;
        board[i].age = 0;
        board[i].buildingCondition = 100;
        board[i].roundsSinceMaintenance = 0;
        board[i].structuralDamage = 0;
        if (board[i].type == RAILWAY) p->railwaysOwned--;
        else if (board[i].type == UTILITY) p->utilitiesOwned--;
        board[i].owner = -1;

        assetId[assetCount++] = i;
    }
    printf("Remaining assets transferred to the Bank.\n");
    printf("---------------------------------------------\n\n");

    //Auctions the assests of the players who gets bankrupted
    for (int a = 0; a < assetCount; a++) {
        runAuction(players, board, assetId[a], order, econ);
    }
}

int calculateNetWorth(player *players, sqr *board, int pid){
    int worth = players[pid].cash;
    for (int i = 0; i < 40; i++) {
        if (board[i].owner != pid) continue;
        worth += currentMarketValue(&board[i]);
        worth += board[i].houses * board[i].housePrice;
        if (board[i].hasHotels) worth += board[i].hotelPrice;
    }
    worth -= players[pid].loanAmount;
    return worth;
}

int propertyDepreciationPercent(sqr *sq){
    if (sq->age <= 50) return 0;
    int pct = (sq->age - 50) / 5;
    if (pct > 30) pct = 30;
    return pct;
}

int buildingConditionRentPercent(sqr *sq){
    int c = sq->buildingCondition;
    if (c >= 90) return 100;
    if (c >= 75) return 90;
    if (c >= 50) return 75;
    if (c >= 25) return 50;
    return 0;
}

int currentMarketValue(sqr *sq){
    int value = applyPercent(sq->buyPrice, -propertyDepreciationPercent(sq));
    if (sq->structuralDamage) value = applyPercent(value, -15);
    return value;
}

void calPropertyAge(sqr *board){
    for (int i = 0; i < 40; i++) {
        if (board[i].owner == -1) continue;

        int before = propertyDepreciationPercent(&board[i]);
        board[i].age++;
        int after = propertyDepreciationPercent(&board[i]);

        if (after > before) {
            printf("\n* Property Depreciation\n");
            printf("  Property: %s\n", board[i].name);
            printf("  has depreciated by %d%%.\n", after);
            printf("  Current Value: LKR %d.\n", currentMarketValue(&board[i]));
        }
    }
}

void checkBuildingCondition(sqr *board){
    for (int i = 0; i < 40; i++) {
        sqr *sq = &board[i];
        if (!(sq->houses > 0 || sq->hasHotels)) continue;

        if (sq->buildingCondition > 0) {
            sq->buildingCondition -= 2;
            if (sq->buildingCondition < 0) sq->buildingCondition = 0;
        }
        sq->roundsSinceMaintenance++;

        if (sq->roundsSinceMaintenance > 20 && !sq->structuralDamage) {
            sq->structuralDamage = 1;
            printf("\n--- Structural Damage: %s has been neglected for over 20 rounds. ---\n", sq->name);
            printf("  Property value -15%%, maximum rent -25%%, future maintenance costs +50%%.\n");
        }
    }
}

void performMaintenance(player *players, sqr *board, int pid, economicState *econ){
    (void)econ;
    player *p = &players[pid];
    for (int i = 0; i < 40; i++) {
        sqr *sq = &board[i];
        if (sq->owner != pid || sq->buildingCondition >= 100) continue;
        if (!(sq->houses > 0 || sq->hasHotels)) continue;

        int baseCost = sq->hasHotels ? (sq->hotelPrice * 8) / 100 : (sq->housePrice * sq->houses * 5) / 100;
        int cost = sq->structuralDamage ? (baseCost * 150) / 100 : baseCost;

        if (p->cash < cost || !decideMaintenance(p, cost)) continue;

        p->cash -= cost;
        sq->buildingCondition = 100;
        sq->roundsSinceMaintenance = 0;
        printf("* %s performs maintenance on %s for LKR %d. Condition restored to 100%%. Remaining Balance : LKR %d.\n",
               p->name, sq->name, cost, p->cash);
    }
}

void handleRenovationOpportunity(player *players, sqr *board, int pid, int sqIdx){
    sqr *sq = &board[sqIdx];
    player *p = &players[pid];

    int deprPct = propertyDepreciationPercent(sq);
    int damaged = sq->structuralDamage;

    if (deprPct == 0 && !damaged) {
        printf("%s is passing through their own property.\n", p->name);
        return;
    }

    if (!decideRenovation(p, deprPct, damaged)) {
        printf("%s inspects %s but decides not to renovate yet.\n", p->name, sq->name);
        return;
    }

    int replacementValue = (sq->housePrice * sq->houses) + (sq->hasHotels ? sq->hotelPrice : 0);
    int cost = (damaged && replacementValue > 0)
        ? (replacementValue * 25) / 100
        : (currentMarketValue(sq) * 10) / 100;

    if (p->cash < cost) {
        printf("%s cannot afford to renovate %s (needs LKR %d).\n", p->name, sq->name, cost);
        return;
    }

    p->cash -= cost;
    sq->age = 0;
    sq->structuralDamage = 0;
    if (sq->houses > 0 || sq->hasHotels) {
        sq->buildingCondition = 100;
        sq->roundsSinceMaintenance = 0;
    }

    printf("* %s renovates %s for LKR %d.\n", p->name, sq->name, cost);
    printf("  Depreciation cleared, property age reset, rental income restored.\n");
    printf("  Remaining Balance : LKR %d.\n", p->cash);
}

void applyLoanRoundEnd(player *players, int pid, sqr *board){
    player *p = &players[pid];
    if (p->loanAmount == 0) return;

    int interest = (p->loanAmount * p->loanInterestRate) / 100;
    p->loanAmount += interest;
    printf("* %s's loan accrues LKR %d interest (%d%%). New outstanding balance: LKR %d.\n",
           p->name, interest, p->loanInterestRate, p->loanAmount);

    // Rule-LK 6: hitting zero here means the loan matured unpaid this round boundary.
    if (p->loanRoundsRemaining > 0) {
        p->loanRoundsRemaining--;
    }
    if (p->loanRoundsRemaining <= 0) {
        foreclose(players, pid, board);
    }
}

void handleBankLanding(player *players, int pid, sqr *board, economicState *econ){
    player *p = &players[pid];
    printf("\n--- %s visits the Bank of Ceylon ---\n", p->name);

    int maxLoan = calculateMaxLoan(players, board, pid, econ);
    loanAction action = decideLoanAction(p, maxLoan);

    switch (action){
        case LOAN_ACTION_TAKE:
            p->loanAmount = maxLoan;
            p->loanInterestRate = getEffectiveLoanInterestRate(econ, pid);
            p->loanRoundsRemaining = 20;
            p->cash += maxLoan;

            lockEligibleCollateral(players, board, pid);

            printf("* %s obtained a secured loan of LKR %d at %d%% interest for 20 rounds.\n", p->name, p->loanAmount, p->loanInterestRate);
            printf("  Pledged properties are now LOAN LOCKED.\n");
            printf("  New Cash Balance: LKR %d.\n", p->cash);
            break;

        case LOAN_ACTION_REPAY_FULL:
            printf("* %s repays loan in full (LKR %d).\n", p->name, p->loanAmount);
            p->cash -= p->loanAmount;
            p->loanAmount = 0;
            p->loanInterestRate = 0;
            p->loanRoundsRemaining = 0;
            unlockAllCollateral(players, board, pid);
            printf("  All pledged collateral properties have been UNLOCKED.\n");
            printf("  Remaining Cash Balance: LKR %d.\n", p->cash);
            break;

        case LOAN_ACTION_REPAY_PARTIAL: {
            int repayAmt = p->cash / 2;
            if (repayAmt > p->loanAmount) repayAmt = p->loanAmount;
            p->cash -= repayAmt;
            p->loanAmount -= repayAmt;
            printf("* %s repays LKR %d towards the loan. Outstanding balance: LKR %d.\n",
                   p->name, repayAmt, p->loanAmount);
            if (p->loanAmount == 0) {
                p->loanInterestRate = 0;
                p->loanRoundsRemaining = 0;
                unlockAllCollateral(players, board, pid);
                printf("  Loan fully settled via repayment. All pledged collateral UNLOCKED.\n");
            }
            printf("  Remaining Cash Balance: LKR %d.\n", p->cash);
            break;
        }

        case LOAN_ACTION_REFINANCE:
            printf("* %s cannot repay but refinances the loan, extending duration to 20 rounds.\n", p->name);
            p->loanRoundsRemaining = 20;
            break;

        case LOAN_ACTION_TOPUP:
            p->loanAmount += maxLoan;
            p->cash += maxLoan;
            lockEligibleCollateral(players, board, pid);
            printf("* %s increases the loan by LKR %d using newly eligible collateral. New outstanding balance: LKR %d.\n",
                   p->name, maxLoan, p->loanAmount);
            printf("  New Cash Balance: LKR %d.\n", p->cash);
            break;

        case LOAN_ACTION_NONE:
        default:
            if (p->loanAmount > 0) {
                printf("* %s has an active loan of LKR %d (%d rounds remaining). No action taken this turn.\n",
                       p->name, p->loanAmount, p->loanRoundsRemaining);
            } else {
                printf("* %s stops by but takes no action this turn (max loan available: LKR %d).\n", p->name, maxLoan);
            }
            break;
    }
    printf("---------------------------------------------\n\n");
}

static int calculatePremium(sqr *board, sqr *sq, int policyType, economicState *econ, int pid){
    int base;
    switch (policyType) {
        case POLICY_BASIC:         base = (sq->buyPrice * 5) / 100; break;
        case POLICY_COMPREHENSIVE: base = (sq->buyPrice * 10) / 100; break;
        case POLICY_BUSINESS:      base = (sq->buyPrice * 15) / 100; break;
        default: return 0;
    }
    return applyPercent(base, getEffectivePercent(econ, board, sq->position, VALTYP_INSURANCE_PREMIUM, pid));
}

static const char* policyName(int policyType){
    switch (policyType) {
        case POLICY_BASIC: return "Basic Property Insurance";
        case POLICY_COMPREHENSIVE: return "Comprehensive Insurance";
        case POLICY_BUSINESS: return "Business Interruption Insurance";
        default: return "No Insurance";
    }
}

void handleInsuranceLanding(player *players, sqr *board, int pid, economicState *econ){
    player *p = &players[pid];
    printf("\n--- %s visits the insurance office ---\n", p->name);

    int purchasedAny = 0;
    for (int i = 0; i < 40; i++) {
        sqr *sq = &board[i];
        if (sq->owner != pid || sq->type != PROPERTY) continue;

        int needsCover = (sq->insureType == POLICY_NONE) ||
                          (sq->insuranceRoundsRemaining > 0 && sq->insuranceRoundsRemaining <= 3);
        if (!needsCover) continue;

        int policyType = decideInsurancePolicy(p, sq);
        if (policyType == POLICY_NONE) continue;
        int premium = calculatePremium(board, sq, policyType, econ, pid);

        if (p->cash >= premium) {
            p->cash -= premium;
            sq->insureType = policyType;
            sq->insuranceRoundsRemaining = INSURANCE_DURATION;
            purchasedAny = 1;
            printf("* %s purchased %s.\n", p->name, policyName(policyType));
            printf("  Property: %s\n", sq->name);
            printf("  Premium: LKR %d.\n", premium);
            printf("  Remaining Cash Balance: LKR %d.\n", p->cash);
        } else if (sq->insureType != POLICY_NONE) {
            printf("* %s cannot afford to renew insurance on %s (LKR %d needed).\n", p->name, sq->name, premium);
        }
    }

    if (!purchasedAny) {
        printf("* %s purchases no insurance this visit.\n", p->name);
    }
    printf("---------------------------------------------\n\n");
}

void applyInsuranceRoundEnd(player *players, int pid, sqr *board){
    player *p = &players[pid];
    for (int i = 0; i < 40; i++) {
        sqr *sq = &board[i];
        if (sq->owner != pid || sq->insureType == POLICY_NONE) continue;

        sq->insuranceRoundsRemaining--;

        if (sq->insuranceRoundsRemaining == 3) {
            printf("* Insurance policy on %s expires in 3 rounds.\n", sq->name);
        } else if (sq->insuranceRoundsRemaining <= 0) {
            printf("* Insurance policy on %s has expired. %s is no longer covered.\n", sq->name, p->name);
            sq->insureType = POLICY_NONE;
            sq->insuranceRoundsRemaining = 0;
        }
    }
}

static const char* randomDisasterName(){
    static const char* disasters[] = {
        "Fire", "Flood", "Riot", "Vandalism", "Earthquake", "Building Collapse", "Electrical Failure"
    };
    return disasters[rand() % 7];
}

static int calculateRepairCost(sqr *sq){
    int replacementValue = (sq->housePrice * sq->houses) + (sq->hotelPrice * sq->hasHotels);
    return (replacementValue * 25) / 100;
}

void checkForDisaster(player *players, sqr *board, int roundNum, int *order, economicState *econ){
    if (roundNum <= 0 || roundNum % 10 != 0) return;

    int candidates[25];
    int candidateCount = 0;
    for (int i = 0; i < 40; i++) {
        if (board[i].owner != -1 && (board[i].houses > 0 || board[i].hasHotels)) {
            candidates[candidateCount++] = i;
        }
    }
    if (candidateCount == 0) return;

    sqr *sq = &board[candidates[rand() % candidateCount]];
    player *owner = &players[sq->owner];
    int repairCost = calculateRepairCost(sq);
    sq->hadDisaster = 1;

    printf("\n--- Disaster: %s strikes! ---\n", randomDisasterName());
    printf("  Affected Property: %s\n", sq->name);

    if (sq->insureType == POLICY_NONE) {
        printf("  %s is uninsured and bears the full repair cost.\n", owner->name);
        owner->cash -= repairCost;
        printf("  Repair Cost Paid: LKR %d. Remaining Cash Balance: LKR %d.\n", repairCost, owner->cash);
        if (owner->cash < 0) {
            raiseCashByMortgaging(players, board, sq->owner, econ);
            if (owner->cash < 0) {
                declareBankruptcy(players, board, sq->owner, order, econ);
            }
        }
    } else {
        int coveragePct = (sq->insureType == POLICY_COMPREHENSIVE || sq->insureType == POLICY_BUSINESS) ? 100 : 80;
        int compensation = (repairCost * coveragePct) / 100;

        if (sq->insureType == POLICY_BUSINESS && sq->hasHotels) {
            compensation += sq->rent * 5;
        }

        owner->cash += compensation;
        printf("  Insurance Claim Approved.\n");
        printf("  Compensation Paid: LKR %d. New Cash Balance: LKR %d.\n", compensation, owner->cash);
    }
    printf("---------------------------------------------\n\n");
}
