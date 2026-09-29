#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "types.h"

static const int INFLATION_RATES[6] = {-3, 0, 2, 5, 8, 12};

typedef enum NationalCardEffect{
    CARD_RENT_HOTEL_PCT,
    CARD_RENT_RAILWAY_PCT,
    CARD_RENT_UTILITY_PCT,
    CARD_PROPERTY_VALUE_ALL_PCT,
    CARD_PROPERTY_VALUE_RAILWAY_PCT,
    CARD_CONSTRUCTION_COST_PCT,
    CARD_LOAN_INTEREST,
    CARD_INSURANCE_PREMIUM_PCT,
    CARD_CASH_GRANT_ALL,
    CARD_CASH_GRANT_RANDOM_PLAYER,
    CARD_PROPERTY_CLOSED,
    CARD_CONSTRUCTION_SUSPENDED,
    CARD_INSTANT_DAMAGE_OWNED,
    CARD_INSTANT_GROUP_VALUE_BUMP
}nationalCardEffect;

typedef struct NationalCard{
    const char *name;
    const char *description;
    int durationRounds;         
    nationalCardEffect effectType;
    int magnitude;              
}nationalCard;

static const nationalCard NATIONAL_CARDS[NATIONAL_CARD_COUNT] = {
    {"Tourism Hype",          "Hotels earn double rent for 5 rounds.",              5,  CARD_RENT_HOTEL_PCT,           100},
    {"Fuel Shortage",         "Railway rent doubles for 5 rounds.",                 5,  CARD_RENT_RAILWAY_PCT,         100},
    {"Heavy Floods",          "Random coastal property damaged.",                   0,  CARD_INSTANT_DAMAGE_OWNED,       0},
    {"Political Rally",       "One random property closed for 2 rounds.",           2,  CARD_PROPERTY_CLOSED,            0},
    {"Stock Market Rise",     "All property values increase by 10%.",              15,  CARD_PROPERTY_VALUE_ALL_PCT,    10},
    {"Economic Downturn",     "Property values decrease by 15%.",                  15,  CARD_PROPERTY_VALUE_ALL_PCT,   -15},
    {"Housing Subsidy",       "House construction cost reduced by 30%.",           15,  CARD_CONSTRUCTION_COST_PCT,    -30},
    {"Interest Rate Cut",     "Loan interest reduced by 2%.",                      15,  CARD_LOAN_INTEREST,       -2},
    {"Interest Rate Increase","Loan interest increased by 2%.",                    15,  CARD_LOAN_INTEREST,        2},
    {"Tax Amnesty",           "Each player receives LKR 2,000.",                    0,  CARD_CASH_GRANT_ALL,       2000},
    {"Power Failure",         "Utility income halved for 3 rounds.",                3,  CARD_RENT_UTILITY_PCT,         -50},
    {"Foreign Funding",       "Commercial property values increase by 15%.",       15,  CARD_PROPERTY_VALUE_ALL_PCT,    15},
    {"Port Expansion",        "Railway station values increase by 20%.",           15,  CARD_PROPERTY_VALUE_RAILWAY_PCT,20},
    {"Festival Season",       "Hotels receive 50% additional rent.",               15,  CARD_RENT_HOTEL_PCT,            50},
    {"Labour Strike",         "Construction suspended for 2 rounds.",               2,  CARD_CONSTRUCTION_SUSPENDED,     0},
    {"Insurance Discount",    "Premiums reduced by 20%.",                          15,  CARD_INSURANCE_PREMIUM_PCT,    -20},
    {"Property Revaluation",  "Random property group appreciates by 15%.",          0,  CARD_INSTANT_GROUP_VALUE_BUMP,  15},
    {"Currency Depreciation", "Construction costs increase by 10%.",               15,  CARD_CONSTRUCTION_COST_PCT,     10},
    {"Government Grant",      "Random player receives LKR 5,000.",                  0,  CARD_CASH_GRANT_RANDOM_PLAYER,5000},
    {"National Disaster",     "Random developed property damaged.",                 0,  CARD_INSTANT_DAMAGE_OWNED,       0},
};

typedef struct RegionalCard{
    const char *cardName;
    const char *regionLabel;
    const char *squareNames[4];  
    valueType kind;
    int perecent1;
    int hasSecondEffect;
    const char *squareNames2[4];
    valueType kind2;
    int perecent2;
}regionalCard;

static const regionalCard REGIONAL_CARDS[REGIONAL_CARD_COUNT] = {
    {"Southern Tourism Boom", "Southern Province",              {"Galle Fort", "Unawatuna", "Hikkaduwa", NULL},                                                               VALTYP_RENT,      40, 0, {NULL}, VALTYP_RENT, 0},
    {"Port City Expansion", "Colombo Port Region",              {"Pettah", "Maradana", "Colombo Fort Railway Station", NULL},                                                 VALTYP_PURCHASE,  25, 0, {NULL}, VALTYP_RENT, 0},
    {"IT Industry Growth", "IT Corridor",                       {"Maharagama", "Nugegoda", "Kottawa", NULL},                                                                  VALTYP_PURCHASE,  20, 0, {NULL}, VALTYP_RENT, 0},
    {"Northern Development Programme", "Northern Province",     {"Jaffna Town", "Nallur", "Trincomalee", NULL},                                                               VALTYP_PURCHASE,  30, 0, {NULL}, VALTYP_RENT, 0},
    {"Tea Export Boom", "Hill Country",                         {"Nuwara Eliya", NULL},                                                                                       VALTYP_PURCHASE,  35, 0, {NULL}, VALTYP_RENT, 0},
    {"Airport Expansion", "Western Airport Corridor",           {"Negombo", "Katunayake", "Ja-Ela", NULL},                                                                    VALTYP_RENT,      30, 0, {NULL}, VALTYP_RENT, 0},
    {"University City Growth", "Kandy Region",                  {"Peradeniya", "Kandy City", NULL},                                                                           VALTYP_PURCHASE,  20, 0, {NULL}, VALTYP_RENT, 0},
    {"Beach Pollution", "Southern Province",                    {"Galle Fort", "Unawatuna", "Hikkaduwa", NULL},                                                               VALTYP_RENT,     -30, 0, {NULL}, VALTYP_RENT, 0},
    {"Flood Damage", "Coastal Suburbs",                         {"Bambalapitiya", "Wellawatte", "Mount Lavinia", NULL},                                                       VALTYP_PURCHASE, -20, 0, {NULL}, VALTYP_RENT, 0},
    {"Transport Strike", "National Railway Network",            {"Colombo Fort Railway Station", "Kandy Railway Station", "Galle Railway Station", "Jaffna Railway Station"}, VALTYP_RENT,     -40, 0, {NULL}, VALTYP_RENT, 0},
    {"Electricity Tariff Increase", "National Utility Network", {"Ceylon Electricity Board", "National Water Supply and Drainage Board", NULL},                               VALTYP_RENT,      25, 0, {NULL}, VALTYP_RENT, 0},
    {"Water Shortage", "Coastal Suburbs",                       {"National Water Supply and Drainage Board", NULL},                                                           VALTYP_RENT,      20, 1, {"Bambalapitiya", "Wellawatte", "Mount Lavinia", NULL}, VALTYP_PURCHASE, -10}, //Has 2 effects for 2 groups 
};

static int findSquareByName(sqr *board, const char *name){
    for (int i = 0; i < 40; i++) {
        if (strcmp(board[i].name, name) == 0) return i;
    }
    return -1;
}

void initEconomicState(economicState *econ, sqr *board){
    econ->inflationRatePercent = 0;
    econ->inflationMultiplier = 1.0;
    econ->baseLoanInterestRate = 8;

    for (int g = 0; g < NA; g++) {
        econ->marketBoomActive[g] = 0;
        econ->marketDeclineActive[g] = 0;
        econ->marketRoundsRemaining[g] = 0;
        econ->marketLastAffectedRound[g] = -1000;
    }
    econ->lastBoomGroup = NA;
    econ->lastDeclineGroup = NA;

    econ->activeRegulation = REG_NONE;

    econ->nationalCardCursor = 0;
    for (int pid = 0; pid < 4; pid++) {
        for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
            econ->playerEvents[pid][s].cardId = -1;
            econ->playerEvents[pid][s].roundsRemaining = 0;
            econ->playerEvents[pid][s].affectedSquareIndex = -1;
        }
    }

    econ->activeRegionalCardIndex = -1;
    econ->regionalCardRoundsRemaining = 0;
    for (int c = 0; c < REGIONAL_CARD_COUNT; c++) {
        const regionalCard *rc = &REGIONAL_CARDS[c];

        int count = 0;
        for (int n = 0; n < 4 && rc->squareNames[n] != NULL; n++) {
            int idx = findSquareByName(board, rc->squareNames[n]);
            if (idx != -1) econ->regionalCardSquares[c][count++] = idx;
        }
        econ->regionalCardSquareCount[c] = count;

        int count2 = 0;
        if (rc->hasSecondEffect) {
            for (int n = 0; n < 4 && rc->squareNames2[n] != NULL; n++) {
                int idx = findSquareByName(board, rc->squareNames2[n]);
                if (idx != -1) econ->regionalCardSquares2[c][count2++] = idx;
            }
        }
        econ->regionalCardSquareCount2[c] = count2;
    }
}

//Uses math.h to lowerround to the nearest value
int applyPercent(int baseValue, int pct){
    return (int)lround(baseValue * (1.0 + pct / 100.0));
}

static int uptrendP(valueType kind){
    switch (kind) {
        case VALTYP_PURCHASE:     return 15;
        case VALTYP_MORTGAGE:     return 15;
        case VALTYP_RENT:         return 25;
        case VALTYP_CONSTRUCTION: return 10;
        case VALTYP_VALUE:        return 20;
        default:                  return 0;
    }
}

static int downturnP(valueType kind){
    switch (kind) {
        case VALTYP_MORTGAGE:     return -10;
        case VALTYP_RENT:         return -20;
        case VALTYP_AUCTION_START:return -25;
        case VALTYP_VALUE:        return -15;
        default:              return 0;
    }
}

static int regulationDeltaFor(govRegulation reg, sqr_typ type, valueType kind){
    switch (reg) {
        case REG_HOUSING_SUBSIDY:
            return (kind == VALTYP_CONSTRUCTION) ? -30 : 0;
        case REG_RAILWAY_MODERNIZATION:
            return (type == RAILWAY && kind == VALTYP_RENT) ? 25 : 0;
        case REG_ELECTRICITY_TARIFF_REVISION:
            return (type == UTILITY && kind == VALTYP_RENT) ? 20 : 0;
        case REG_INSURANCE_REGULATION:
            return (kind == VALTYP_INSURANCE_PREMIUM) ? -15 : 0;
        default:
            return 0;
    }
}

static const char* groupDisplayName(sqr_grp g){
    switch (g) {
        case Orange:    return "Orange District";
        case LightBlue: return "Coastal Suburbs";
        case DarkBlue:  return "Premier Estates";
        case Pink:      return "Pink District";
        case Brown:     return "Colombo Central";
        case Yellow:    return "Southern Province";
        case Green:     return "Northern Province";
        case Red:       return "Hill Country";
        default:        return "Unknown Region";
    }
}

static const char* regulationName(govRegulation r){
    switch (r) {
        case REG_INCREASE_PROPERTY_TAX:      return "Increase Property Tax";
        case REG_REDUCE_LOAN_INTEREST:       return "Reduce Loan Interest";
        case REG_HOUSING_SUBSIDY:            return "Housing Subsidy";
        case REG_LUXURY_PROPERTY_TAX:        return "Luxury Property Tax";
        case REG_RAILWAY_MODERNIZATION:      return "Railway Modernization";
        case REG_ELECTRICITY_TARIFF_REVISION:return "Electricity Tariff Revision";
        case REG_INSURANCE_REGULATION:       return "Insurance Regulation";
        case REG_ANTI_SPECULATION_ACT:       return "Anti-Speculation Act";
        default:                             return "None";
    }
}

static const char* regulationDescription(govRegulation r){
    switch (r) {
        case REG_INCREASE_PROPERTY_TAX:      return "Income Tax increases by 50%.";
        case REG_REDUCE_LOAN_INTEREST:       return "Loan interest decreases by 2%.";
        case REG_HOUSING_SUBSIDY:            return "House construction costs reduce 30%.";
        case REG_LUXURY_PROPERTY_TAX:        return "Hotels incur an annual maintenance tax of 25% of property value.";
        case REG_RAILWAY_MODERNIZATION:      return "Railway rents increase 25%.";
        case REG_ELECTRICITY_TARIFF_REVISION:return "Utility rents increase 20%.";
        case REG_INSURANCE_REGULATION:       return "Insurance premiums decrease 15%. Coverage remains unchanged.";
        case REG_ANTI_SPECULATION_ACT:       return "Players may own at most three undeveloped properties.";
        default:                             return "";
    }
}

void calInflation(economicState *econ, int roundNum){
    if (roundNum <= 0 || roundNum % 10 != 0) return;

    econ->inflationRatePercent = INFLATION_RATES[rand() % 6];
    econ->inflationMultiplier *= (1.0 + econ->inflationRatePercent / 100.0);
    econ->baseLoanInterestRate = econ->baseLoanInterestRate + econ->inflationRatePercent;
    if (econ->baseLoanInterestRate < 5) econ->baseLoanInterestRate = 5;
    if (econ->baseLoanInterestRate > 15) econ->baseLoanInterestRate = 15;

    printf("\n--- Inflation Update ---\n");
    printf("  Rate set to %+d%%. Cumulative multiplier now %.2fx.\n",
           econ->inflationRatePercent, econ->inflationMultiplier);
    printf("  New base loan interest rate: %d%%.\n", econ->baseLoanInterestRate);
    printf("---------------------------------------------\n\n");
}

void calMarket(economicState *econ, int roundNum){
    for (int g = 0; g < NA; g++) {
        if (econ->marketRoundsRemaining[g] > 0) {
            econ->marketRoundsRemaining[g]--;
            if (econ->marketRoundsRemaining[g] == 0) {
                econ->marketBoomActive[g] = 0;
                econ->marketDeclineActive[g] = 0;
            }
        }
    }

    if (roundNum <= 0 || roundNum % 10 != 0) return;
    int eligible[NA];
    int eligibleCount = 0;
    for (int g = 0; g < NA; g++) {
        if (roundNum - econ->marketLastAffectedRound[g] >= 30) {
            eligible[eligibleCount++] = g;
        }
    }
    if (eligibleCount < 2) {
        eligibleCount = 0;
        for (int g = 0; g < NA; g++) {
            if (g != econ->lastBoomGroup && g != econ->lastDeclineGroup) {
                eligible[eligibleCount++] = g;
            }
        }
        printf("* Market review: cooldown pool too small, relaxing to avoid only the last-affected groups.\n");
    }
    if (eligibleCount < 2) return;

    sqr_grp boomGroup, declineGroup;
    do {
        boomGroup = (sqr_grp)eligible[rand() % eligibleCount];
    } while (boomGroup == econ->lastBoomGroup && eligibleCount > 1);

    do {
        declineGroup = (sqr_grp)eligible[rand() % eligibleCount];
    } while ((declineGroup == boomGroup || declineGroup == econ->lastDeclineGroup) && eligibleCount > 1);

    econ->marketBoomActive[boomGroup] = 1;
    econ->marketRoundsRemaining[boomGroup] = 10;
    econ->marketLastAffectedRound[boomGroup] = roundNum;
    econ->lastBoomGroup = boomGroup;

    econ->marketDeclineActive[declineGroup] = 1;
    econ->marketRoundsRemaining[declineGroup] = 10;
    econ->marketLastAffectedRound[declineGroup] = roundNum;
    econ->lastDeclineGroup = declineGroup;

    printf("\n--- Market Review ---\n");
    printf("  Market Boom: %s (+%d%% property value, 10 rounds).\n", groupDisplayName(boomGroup), uptrendP(VALTYP_VALUE));
    printf("  Market Decline: %s (%d%% property value, 10 rounds).\n", groupDisplayName(declineGroup), downturnP(VALTYP_VALUE));
    printf("---------------------------------------------\n\n");
}

static void applyLuxuryPropertyTax(economicState *econ, player *players, sqr *board){
    for (int i = 0; i < 40; i++) {
        if (!board[i].hasHotels || board[i].owner == -1) continue;

        player *owner = &players[board[i].owner];
        int effectiveHotelValue = applyPercent(board[i].hotelPrice,
            getEffectivePercent(econ, board, i, VALTYP_CONSTRUCTION, -1));
        int tax = (effectiveHotelValue * 25) / 100;

        owner->cash -= tax;
        printf("* Luxury Property Tax: %s pays LKR %d on %s. Remaining Balance : LKR %d.\n",
               owner->name, tax, board[i].name, owner->cash);
    }
}

void checkRegulation(economicState *econ, player *players, sqr *board, int roundNum){
    if (econ->activeRegulation == REG_LUXURY_PROPERTY_TAX) {
        applyLuxuryPropertyTax(econ, players, board);
    }

    if (roundNum <= 0 || roundNum % 20 != 0) return;

    econ->activeRegulation = (govRegulation)(rand() % REG_COUNT);

    printf("\n--- Government Regulation ---\n");
    printf("  %s introduced.\n", regulationName(econ->activeRegulation));
    printf("  %s\n", regulationDescription(econ->activeRegulation));
    printf("---------------------------------------------\n\n");
}

void checkRegionalCard(economicState *econ, int roundNum){
    if (econ->regionalCardRoundsRemaining > 0) {
        econ->regionalCardRoundsRemaining--;
        if (econ->regionalCardRoundsRemaining == 0) {
            econ->activeRegionalCardIndex = -1;
        }
    }

    if (roundNum <= 0 || roundNum % 15 != 0) return;

    econ->activeRegionalCardIndex = rand() % REGIONAL_CARD_COUNT;
    econ->regionalCardRoundsRemaining = 15;

    const regionalCard *rc = &REGIONAL_CARDS[econ->activeRegionalCardIndex];
    printf("\n--- Regional Development ---\n");
    printf("  %s (%s): %+d%% for 15 rounds.\n", rc->cardName, rc->regionLabel, rc->perecent1);
    if (rc->hasSecondEffect) {
        printf("  Additional effect: %+d%%.\n", rc->perecent2);
    }
    printf("---------------------------------------------\n\n");
}

static int regionalCardDeltaFor(economicState *econ, int sqrID, valueType kind){
    if (econ->activeRegionalCardIndex == -1) return 0;

    int id = econ->activeRegionalCardIndex;
    const regionalCard *rc = &REGIONAL_CARDS[id];
    int pct = 0;

    if (rc->kind == kind) {
        for (int k = 0; k < econ->regionalCardSquareCount[id]; k++) {
            if (econ->regionalCardSquares[id][k] == sqrID) {
                pct += rc->perecent1;
                break;
            }
        }
    }
    if (rc->hasSecondEffect && rc->kind2 == kind) {
        for (int k = 0; k < econ->regionalCardSquareCount2[id]; k++) {
            if (econ->regionalCardSquares2[id][k] == sqrID) {
                pct += rc->perecent2;
                break;
            }
        }
    }
    return pct;
}

static int nationalCardDeltaFor(economicState *econ, sqr *board, int sqIndex, valueType kind, int ownerId){
    if (ownerId < 0 || ownerId > 3) return 0;

    int pct = 0;
    for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
        const activeCardEffect *slot = &econ->playerEvents[ownerId][s];
        if (slot->cardId == -1) continue;
        const nationalCard *card = &NATIONAL_CARDS[slot->cardId];

        switch (card->effectType) {
            case CARD_RENT_HOTEL_PCT:
                if (kind == VALTYP_RENT && board[sqIndex].hasHotels) pct += card->magnitude;
                break;
            case CARD_RENT_RAILWAY_PCT:
                if (kind == VALTYP_RENT && board[sqIndex].type == RAILWAY) pct += card->magnitude;
                break;
            case CARD_RENT_UTILITY_PCT:
                if (kind == VALTYP_RENT && board[sqIndex].type == UTILITY) pct += card->magnitude;
                break;
            case CARD_PROPERTY_VALUE_ALL_PCT:
                if ((kind == VALTYP_PURCHASE || kind == VALTYP_VALUE) && board[sqIndex].type == PROPERTY) pct += card->magnitude;
                break;
            case CARD_PROPERTY_VALUE_RAILWAY_PCT:
                if ((kind == VALTYP_PURCHASE || kind == VALTYP_VALUE) && board[sqIndex].type == RAILWAY) pct += card->magnitude;
                break;
            case CARD_CONSTRUCTION_COST_PCT:
                if (kind == VALTYP_CONSTRUCTION) pct += card->magnitude;
                break;
            case CARD_INSURANCE_PREMIUM_PCT:
                if (kind == VALTYP_INSURANCE_PREMIUM) pct += card->magnitude;
                break;
            default:
                break;
        }
    }
    return pct;
}

int getEffectivePercent(economicState *econ, sqr *board, int sqIndex, valueType kind, int ownerId){
    int pct = 0;

    if (kind != VALTYP_AUCTION_START) {
        pct += (int)lround((econ->inflationMultiplier - 1.0) * 100.0);
    }

    sqr_grp g = board[sqIndex].group;
    if (g != NA) {
        if (econ->marketBoomActive[g]) pct += uptrendP(kind);
        if (econ->marketDeclineActive[g]) pct += downturnP(kind);
    }

    pct += regulationDeltaFor(econ->activeRegulation, board[sqIndex].type, kind);
    pct += regionalCardDeltaFor(econ, sqIndex, kind);
    pct += nationalCardDeltaFor(econ, board, sqIndex, kind, ownerId);

    return pct; 
}

int getEffectiveLoanInterestRate(economicState *econ, int pid){
    int rate = econ->baseLoanInterestRate;
    if (econ->activeRegulation == REG_REDUCE_LOAN_INTEREST) rate -= 2;

    if (pid >= 0 && pid <= 3) {
        for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
            const activeCardEffect *slot = &econ->playerEvents[pid][s];
            if (slot->cardId == -1) continue;
            if (NATIONAL_CARDS[slot->cardId].effectType == CARD_LOAN_INTEREST) {
                rate += NATIONAL_CARDS[slot->cardId].magnitude;
            }
        }
    }

    if (rate < 5) rate = 5;
    if (rate > 15) rate = 15;
    return rate;
}

int getEffectiveIncomeTaxRate(economicState *econ){
    int rate = 10;
    if (econ->activeRegulation == REG_INCREASE_PROPERTY_TAX) rate = (rate * 150) / 100;
    return rate;
}

void printMarketConditionsReport(economicState *econ, sqr *board){
    (void)board;

    printf("=========================================\n");
    printf("Current Market Conditions\n");
    printf("=========================================\n");

    printf("Market Boom\n-------------\n");
    int anyBoom = 0;
    for (int g = 0; g < NA; g++) {
        if (econ->marketBoomActive[g]) {
            printf("%s (+%d%%)\nRounds Remaining : %d\n", groupDisplayName(g), uptrendP(VALTYP_VALUE), econ->marketRoundsRemaining[g]);
            anyBoom = 1;
        }
    }
    if (!anyBoom) printf("None currently active.\n");

    printf("\nMarket Decline\n----------------\n");
    int anyDecline = 0;
    for (int g = 0; g < NA; g++) {
        if (econ->marketDeclineActive[g]) {
            printf("%s (%d%%)\nRounds Remaining : %d\n", groupDisplayName(g), downturnP(VALTYP_VALUE), econ->marketRoundsRemaining[g]);
            anyDecline = 1;
        }
    }
    if (!anyDecline) printf("None currently active.\n");

    printf("\nRegional Development\n-----------------------\n");
    if (econ->activeRegionalCardIndex != -1) {
        const regionalCard *rc = &REGIONAL_CARDS[econ->activeRegionalCardIndex];
        printf("%s\n(%+d%%)\nRounds Remaining : %d\n", rc->cardName, rc->perecent1, econ->regionalCardRoundsRemaining);
    } else {
        printf("None currently active.\n");
    }

    printf("\nInflation\n------------\n");
    printf("%+d%%\n", econ->inflationRatePercent);

    printf("\nCurrent Loan Interest\n-----------------------\n");
    printf("%d%%\n", getEffectiveLoanInterestRate(econ, -1));

    printf("=========================================\n\n");
}

static void applyNationalCardEffect(economicState *econ, player *players, sqr *board, int drawerId, int cardIdx){
    const nationalCard *card = &NATIONAL_CARDS[cardIdx];
    player *drawer = &players[drawerId];

    switch (card->effectType) {
        case CARD_CASH_GRANT_ALL:
            for(int i=0; i<4;i++){
                players[i].cash += card->magnitude;
            }
            printf("  All players receive LKR %d. New Balance of %s is : LKR %d.\n", card->magnitude, drawer->name,  drawer->cash);
            return;

        case CARD_CASH_GRANT_RANDOM_PLAYER: {
            int target = rand() % 4;
            players[target].cash += card->magnitude;
            printf("  %s receives LKR %d. New Balance : LKR %d.\n", players[target].name, card->magnitude, players[target].cash);
            return;
        }

        case CARD_INSTANT_DAMAGE_OWNED: {
            int candidates[40], count = 0;
            for (int i = 0; i < 40; i++) {
                if (board[i].owner == drawerId && (board[i].houses > 0 || board[i].hasHotels)) {
                    candidates[count++] = i;
                }
            }
            if (count == 0) {
                printf("  %s owns no developed property — card has no effect.\n", drawer->name);
                return;
            }
            sqr *sq = &board[candidates[rand() % count]];
            int repairCost = ((sq->housePrice * sq->houses) + (sq->hotelPrice * sq->hasHotels)) * 25 / 100;
            drawer->cash -= repairCost;
            sq->hadDisaster = 1;
            printf("  %s's %s is damaged. Repair Cost Paid : LKR %d. Remaining Balance : LKR %d.\n",
                   drawer->name, sq->name, repairCost, drawer->cash);
            return;
        }

        case CARD_INSTANT_GROUP_VALUE_BUMP: {
            int groupsOwned[NA], count = 0;
            for (int g = 0; g < NA; g++) {
                for (int i = 0; i < 40; i++) {
                    if (board[i].type == PROPERTY && board[i].group == g && board[i].owner == drawerId) {
                        groupsOwned[count++] = g;
                        break;
                    }
                }
            }
            if (count == 0) {
                printf("  %s owns no property group — card has no effect.\n", drawer->name);
                return;
            }
            sqr_grp g = (sqr_grp)groupsOwned[rand() % count];
            printf("  %s's holdings in %s appreciate by %d%%.\n", drawer->name, groupDisplayName(g), card->magnitude);
            return;
        }

        default:
            break; // durationed effects fall through below
    }

    activeCardEffect *slots = econ->playerEvents[drawerId];
    int slotIdx = -1;
    for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
        if (slots[s].cardId == -1) { slotIdx = s; break; }
    }
    if (slotIdx == -1) {
        int minIdx = 0;
        for (int s = 1; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
            if (slots[s].roundsRemaining < slots[minIdx].roundsRemaining) minIdx = s;
        }
        printf("  %s already has the maximum active effects; '%s' replaces the soonest-expiring one.\n",
               drawer->name, NATIONAL_CARDS[slots[minIdx].cardId].name);
        slotIdx = minIdx;
    }

    int affectedSquareIndex = -1;
    if (card->effectType == CARD_PROPERTY_CLOSED) {
        int candidates[40], count = 0;
        for (int i = 0; i < 40; i++) {
            if (board[i].type == PROPERTY && board[i].owner == drawerId) candidates[count++] = i;
        }
        if (count == 0) {
            printf("  %s owns no property to close — card has no effect.\n", drawer->name);
            return;
        }
        affectedSquareIndex = candidates[rand() % count];
        printf("  %s is closed for %d rounds.\n", board[affectedSquareIndex].name, card->durationRounds);
    }

    slots[slotIdx].cardId = cardIdx;
    slots[slotIdx].roundsRemaining = card->durationRounds;
    slots[slotIdx].affectedSquareIndex = affectedSquareIndex;

    printf("  Effect active for %d rounds.\n", card->durationRounds);
}

void handleEventLanding(economicState *econ, player *players, sqr *board, int id){
    sqr *sq = &board[players[id].position];
    printf("\n--- %s draws a National Event Card at %s ---\n", players[id].name, sq->name);

    int cardIdx = econ->nationalCardCursor;
    econ->nationalCardCursor = (econ->nationalCardCursor + 1) % NATIONAL_CARD_COUNT;
    const nationalCard *card = &NATIONAL_CARDS[cardIdx];

    printf("* Card Drawn: %s\n  %s\n", card->name, card->description);
    applyNationalCardEffect(econ, players, board, id, cardIdx);

    printf("---------------------------------------------\n\n");
}

void checkNationalCardEffects(economicState *econ, player *players){
    for (int pid = 0; pid < 4; pid++) {
        for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
            activeCardEffect *slot = &econ->playerEvents[pid][s];
            if (slot->cardId == -1) continue;

            slot->roundsRemaining--;
            if (slot->roundsRemaining <= 0) {
                printf("* %s's '%s' effect has expired.\n", players[pid].name, NATIONAL_CARDS[slot->cardId].name);
                slot->cardId = -1;
                slot->affectedSquareIndex = -1;
            }
        }
    }
}

int isSquareClosed(economicState *econ, int sqIndex){
    for (int pid = 0; pid < 4; pid++) {
        for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
            const activeCardEffect *slot = &econ->playerEvents[pid][s];
            if (slot->cardId != -1 &&
                NATIONAL_CARDS[slot->cardId].effectType == CARD_PROPERTY_CLOSED &&
                slot->affectedSquareIndex == sqIndex) {
                return 1;
            }
        }
    }
    return 0;
}

int isConstructionSuspended(economicState *econ, int pid){
    for (int s = 0; s < MAX_ACTIVE_EVENTS_PER_PLAYER; s++) {
        const activeCardEffect *slot = &econ->playerEvents[pid][s];
        if (slot->cardId != -1 && NATIONAL_CARDS[slot->cardId].effectType == CARD_CONSTRUCTION_SUSPENDED) {
            return 1;
        }
    }
    return 0;
}
