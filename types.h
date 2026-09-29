#ifndef TYPES_H
#define TYPES_H

#define POLICY_NONE 0
#define POLICY_BASIC 1
#define POLICY_COMPREHENSIVE 2
#define POLICY_BUSINESS 3

//Enumerations

typedef enum SquareType{
    START,
    PROPERTY,
    RAILWAY,
    UTILITY,
    BANK,
    INSURANCE,
    EVENT,
    JAIL,
    GOTOJAIL,
    FREEPARKING,
    TAX
}sqr_typ;

typedef enum SquareGroup{
    Orange,
    LightBlue,
    DarkBlue,
    Pink,
    Brown,
    Yellow,
    Green,
    Red,
    NA
}sqr_grp;

typedef enum ValueType{
    VALTYP_PURCHASE,
    VALTYP_MORTGAGE,
    VALTYP_RENT,
    VALTYP_CONSTRUCTION,
    VALTYP_INSURANCE_PREMIUM,
    VALTYP_AUCTION_START,
    VALTYP_VALUE
}valueType;

typedef enum GovRegulation{
    REG_INCREASE_PROPERTY_TAX,
    REG_REDUCE_LOAN_INTEREST,
    REG_HOUSING_SUBSIDY,
    REG_LUXURY_PROPERTY_TAX,
    REG_RAILWAY_MODERNIZATION,
    REG_ELECTRICITY_TARIFF_REVISION,
    REG_INSURANCE_REGULATION,
    REG_ANTI_SPECULATION_ACT,
    REG_COUNT,
    REG_NONE = -1
}govRegulation;

typedef enum LoanAction{
    LOAN_ACTION_NONE,
    LOAN_ACTION_TAKE,
    LOAN_ACTION_REPAY_FULL,
    LOAN_ACTION_REPAY_PARTIAL,
    LOAN_ACTION_REFINANCE,
    LOAN_ACTION_TOPUP
}loanAction;

#define NATIONAL_CARD_COUNT 20
#define REGIONAL_CARD_COUNT 12
#define MAX_ACTIVE_EVENTS_PER_PLAYER 8 //No of events that can be stacked

//Structs

typedef struct ActiveCardEffect{
    int cardId;
    int roundsRemaining;
    int affectedSquareIndex;
}activeCardEffect;

typedef struct Player{
    char name[24];
    char strat;
    int cash;
    int position;
    int inJail;
    int jailTurns;
    int rounds;
    int loanAmount;
    int loanInterestRate;
    int loanRoundsRemaining;
    int railwaysOwned;
    int utilitiesOwned;
    int isBankrupt;
}player;

typedef struct Square{
    char name[48];
    int owner;
    int age;
    int buyPrice;
    unsigned mortgaged: 1;
    int mortagePrice;
    unsigned int loanLocked: 1;
    int rent;
    unsigned int houses: 3;
    int housePrice;
    unsigned int hasHotels: 1;
    int hotelPrice;
    int position;
    unsigned int insureType: 3;
    int insuranceRoundsRemaining;
    unsigned int hadDisaster: 1;
    sqr_typ type;
    sqr_grp group;
    int buildingCondition;
    int roundsSinceMaintenance;
    unsigned int structuralDamage: 1;
}sqr;

typedef struct EconomicState{
    int inflationRatePercent;
    double inflationMultiplier;
    int baseLoanInterestRate;
    int marketBoomActive[NA];
    int marketDeclineActive[NA];
    int marketRoundsRemaining[NA];
    int marketLastAffectedRound[NA];
    sqr_grp lastBoomGroup;
    sqr_grp lastDeclineGroup;
    govRegulation activeRegulation;
    int nationalCardCursor;
    activeCardEffect playerEvents[4][MAX_ACTIVE_EVENTS_PER_PLAYER];
    int activeRegionalCardIndex;  
    int regionalCardRoundsRemaining;
    int regionalCardSquares[REGIONAL_CARD_COUNT][4];
    int regionalCardSquareCount[REGIONAL_CARD_COUNT];
    int regionalCardSquares2[REGIONAL_CARD_COUNT][4];
    int regionalCardSquareCount2[REGIONAL_CARD_COUNT];
}economicState;


//Functions

// From boards.c
sqr_typ parseSquareType(char* typeStr, char* name);
sqr_grp parseSquareGroup(char* groupStr, char* name);
const char* squareTypeToString(sqr_typ t);
const char* squareGroupToString(sqr_grp p);
void intBoard(sqr *board);
void movePlayer(player *players, int idx, int roll, sqr *board);

// From players.c
void initPlayers(player *players);
int decidePurchase(player *p, sqr *sq);
int getMaxAuctionBid(player *p, sqr *sq);
loanAction decideLoanAction(player *p, int maxLoan);
int decideInsurancePolicy(player *p, sqr *sq);
int decideConstruction(player *p, int cost);
int allowsHotelUpgrade(player *p);
int decideRenovation(player *p, int depreciationPct, int structuralDamage);
int decideMaintenance(player *p, int cost);

// From game.c
int rolldice();
void determineTurnOrder(player *players, int *order);
void playGame(player *players, int *order, sqr *board, int maxRounds, economicState *econ);
void handleLanding(player *players, sqr *board, int id, int diceRoll, int *order, economicState *econ);
void runAuction(player *players, sqr *board, int sqIndex, int *order, economicState *econ);
void evaluateConstruction(player *players, sqr *board, int pid, economicState *econ);

// From finance.c
int calculateMaxLoan(player *players, sqr *board, int pid, economicState *econ);
void handleBankLanding(player *players, int pid, sqr *board, economicState *econ);
void raiseCashByMortgaging(player *players, sqr *board, int pid, economicState *econ);
int unmortgageSquare(player *players, sqr *board, int sqIdx, economicState *econ);
void handleMortgageDecisions(player *players, sqr *board, int pid, economicState *econ);
void applyLoanRoundEnd(player *players, int pid, sqr *board);
void handleInsuranceLanding(player *players, sqr *board, int pid, economicState *econ);
void applyInsuranceRoundEnd(player *players, int pid, sqr *board);
void checkForDisaster(player *players, sqr *board, int roundNum, int *order, economicState *econ);
int propertyDepreciationPercent(sqr *sq);
int buildingConditionRentPercent(sqr *sq);
int currentMarketValue(sqr *sq);
void calPropertyAge(sqr *board);
void checkBuildingCondition(sqr *board);
void performMaintenance(player *players, sqr *board, int pid, economicState *econ);
void handleRenovationOpportunity(player *players, sqr *board, int pid, int sqIdx);
void declareBankruptcy(player *players, sqr *board, int pid, int *order, economicState *econ);
int calculateNetWorth(player *players, sqr *board, int pid);

// From game.c
int countBankruptPlayers(player *players);

// From events.c
void initEconomicState(economicState *econ, sqr *board);
void calInflation(economicState *econ, int roundNum);
void calMarket(economicState *econ, int roundNum);
void checkRegulation(economicState *econ, player *players, sqr *board, int roundNum);
void checkRegionalCard(economicState *econ, int roundNum);
void checkNationalCardEffects(economicState *econ, player *players);
int getEffectivePercent(economicState *econ, sqr *board, int sqIndex, valueType kind, int ownerId);
int applyPercent(int baseValue, int pct);
int getEffectiveLoanInterestRate(economicState *econ, int pid);
int getEffectiveIncomeTaxRate(economicState *econ);
void handleEventLanding(economicState *econ, player *players, sqr *board, int id);
int isSquareClosed(economicState *econ, int sqIndex);
int isConstructionSuspended(economicState *econ, int pid);
void printMarketConditionsReport(economicState *econ, sqr *board);

#endif