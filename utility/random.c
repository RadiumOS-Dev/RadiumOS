// random.c - Random Number Generator Implementation with Slots Game
#include "random.h"
#include "../timers/timer.h"
#include "../keyboard/keyboard.h"
#include "../vga/vga.h"
#include "../terminal/terminal.h"
#include <stdint.h>
#include <stdbool.h>

// ===== LINEAR CONGRUENTIAL GENERATOR (LCG) =====
static uint32_t rand_seed = 1;


int rand(void) {
    // LCG formula: next = (a * current + c) mod m
    // a = 1664525, c = 1013904223, m = 2^32
    rand_seed = (1664525 * rand_seed + 1013904223);
    return (rand_seed >> 16) & 0x7FFF; // Return 15 bits (0 to 32767)
}

// Get random number in range [min, max]
int rand_range(int min, int max) {
    if (min >= max) return min;
    return min + (rand() % (max - min + 1));
}

// ===== SLOTS GAME IMPLEMENTATION =====

#define NUM_SYMBOLS 7
#define JACKPOT_CREDITS 1000
#define STARTING_CREDITS 100

// Slot symbols with their ASCII representations and payouts
typedef struct {
    char symbol;
    const char* name;
    int payout_3;  // Payout for 3 matching
    int payout_2;  // Payout for 2 matching
} SlotSymbol;

static const SlotSymbol symbols[NUM_SYMBOLS] = {
    {'7', "SEVEN ", 100, 10},  // Jackpot symbol
    {'$', "DOLLAR", 50,  5},
    {'@', "AT    ", 30,  3},
    {'#', "HASH  ", 20,  2},
    {'*', "STAR  ", 15,  1},
    {'+', "PLUS  ", 10,  1},
    {'-', "MINUS ", 5,   0}
};

// Slot machine state
static int player_credits = STARTING_CREDITS;
static int bet_amount = 10;

// Helper function to get symbol by index
static const SlotSymbol* get_symbol(int index) {
    if (index < 0 || index >= NUM_SYMBOLS) {
        return &symbols[NUM_SYMBOLS - 1]; // Default to last symbol
    }
    return &symbols[index];
}

// ===== BLACKJACK GAME IMPLEMENTATION =====

#define DECK_SIZE 52
#define MAX_HAND_SIZE 11  // Theoretical maximum (all Aces)

// Card suits and ranks
typedef enum {
    HEARTS = 0,
    DIAMONDS,
    CLUBS,
    SPADES
} Suit;

typedef enum {
    ACE = 1,
    TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN,
    JACK, QUEEN, KING
} Rank;

// Card structure
typedef struct {
    Rank rank;
    Suit suit;
} Card;

// Hand structure
typedef struct {
    Card cards[MAX_HAND_SIZE];
    int count;
} Hand;

// Deck structure
typedef struct {
    Card cards[DECK_SIZE];
    int top;  // Index of next card to deal
} Deck;

// Get card name
static const char* get_rank_name(Rank rank) {
    switch (rank) {
        case ACE: return "A";
        case TWO: return "2";
        case THREE: return "3";
        case FOUR: return "4";
        case FIVE: return "5";
        case SIX: return "6";
        case SEVEN: return "7";
        case EIGHT: return "8";
        case NINE: return "9";
        case TEN: return "10";
        case JACK: return "J";
        case QUEEN: return "Q";
        case KING: return "K";
        default: return "?";
    }
}

// Get suit symbol
static char get_suit_symbol(Suit suit) {
    switch (suit) {
        case HEARTS: return 3;    // ♥
        case DIAMONDS: return 4;  // ♦
        case CLUBS: return 5;     // ♣
        case SPADES: return 6;    // ♠
        default: return '?';
    }
}

// Get suit color
static enum vga_color get_suit_color(Suit suit) {
    if (suit == HEARTS || suit == DIAMONDS) {
        return VGA_COLOR_RED;
    }
    return VGA_COLOR_WHITE;
}

// Get card value for blackjack
static int get_card_value(Rank rank) {
    if (rank >= JACK && rank <= KING) {
        return 10;
    }
    return rank;
}

// Initialize deck
static void init_deck(Deck* deck) {
    int idx = 0;
    for (int suit = HEARTS; suit <= SPADES; suit++) {
        for (int rank = ACE; rank <= KING; rank++) {
            deck->cards[idx].rank = rank;
            deck->cards[idx].suit = suit;
            idx++;
        }
    }
    deck->top = 0;
}

// Shuffle deck using Fisher-Yates algorithm
static void shuffle_deck(Deck* deck) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand_range(0, i);
        
        // Swap cards[i] and cards[j]
        Card temp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = temp;
    }
    deck->top = 0;
}

// Deal a card from deck
static Card deal_card(Deck* deck) {
    if (deck->top >= DECK_SIZE) {
        // Reshuffle if deck is empty
        shuffle_deck(deck);
    }
    return deck->cards[deck->top++];
}

// Initialize hand
static void init_hand(Hand* hand) {
    hand->count = 0;
}

// Add card to hand
static void add_card(Hand* hand, Card card) {
    if (hand->count < MAX_HAND_SIZE) {
        hand->cards[hand->count++] = card;
    }
}

// Calculate hand value (handles Aces)
static int calculate_hand_value(Hand* hand) {
    int value = 0;
    int aces = 0;
    
    // First, count all cards and track aces
    for (int i = 0; i < hand->count; i++) {
        int card_val = get_card_value(hand->cards[i].rank);
        if (hand->cards[i].rank == ACE) {
            aces++;
            value += 11;  // Initially count Ace as 11
        } else {
            value += card_val;
        }
    }
    
    // Adjust for Aces if busted
    while (value > 21 && aces > 0) {
        value -= 10;  // Convert an Ace from 11 to 1
        aces--;
    }
    
    return value;
}

// Check if hand is blackjack (Ace + 10-value card)
static bool is_blackjack(Hand* hand) {
    if (hand->count != 2) return false;
    
    int value = calculate_hand_value(hand);
    return value == 21;
}

// Display a single card
static void display_card(Card card, bool hidden) {
    if (hidden) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        print("[##]");
    } else {
        terminal_setcolor(get_suit_color(card.suit));
        print("[");
        print(get_rank_name(card.rank));
        terminal_putchar(get_suit_symbol(card.suit));
        print("]");
    }
    print(" ");
}

// Display hand
static void display_hand(Hand* hand, const char* name, bool hide_first) {
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    print("  ");
    print(name);
    print(": ");
    
    for (int i = 0; i < hand->count; i++) {
        display_card(hand->cards[i], hide_first && i == 0);
    }
    
    if (!hide_first) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print(" = ");
        char buf[10];
        itoa(calculate_hand_value(hand), buf, 10);
        print(buf);
        
        if (is_blackjack(hand)) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            print(" BLACKJACK!");
        }
    }
    print("\n");
}

// Display blackjack table
static void display_blackjack_table(Hand* player, Hand* dealer, bool show_dealer_card, int current_bet) {
    terminal_clear();
    
    // Title
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    print("\n");
    print("  =============================================\n");
    print("                 BLACKJACK\n");
    print("  =============================================\n\n");
    
    // Credits and bet
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("  Credits: ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    char buf[20];
    itoa(player_credits, buf, 10);
    print(buf);
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("    Bet: ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    itoa(current_bet, buf, 10);
    print(buf);
    print("\n\n");
    
    // Dealer's hand
    display_hand(dealer, "Dealer", !show_dealer_card);
    
    print("\n");
    
    // Player's hand
    display_hand(player, "Player", false);
    
    print("\n");
}

// Play one round of blackjack
static void play_blackjack_round() {
    static Deck deck;
    static bool deck_initialized = false;
    
    // Initialize and shuffle deck on first play
    if (!deck_initialized) {
        init_deck(&deck);
        shuffle_deck(&deck);
        deck_initialized = true;
    }
    
    // Reshuffle if less than 15 cards remaining
    if (deck.top > DECK_SIZE - 15) {
        shuffle_deck(&deck);
    }
    
    Hand player, dealer;
    init_hand(&player);
    init_hand(&dealer);
    
    int current_bet = bet_amount;
    
    // Check if player has enough credits
    if (player_credits < current_bet) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("  Insufficient credits!\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("  Press any key...");
        while (keyboard_key() == -1);
        return;
    }
    
    // Deduct bet
    player_credits -= current_bet;
    
    // Deal initial cards (player, dealer, player, dealer)
    add_card(&player, deal_card(&deck));
    add_card(&dealer, deal_card(&deck));
    add_card(&player, deal_card(&deck));
    add_card(&dealer, deal_card(&deck));
    
    // Display initial table
    display_blackjack_table(&player, &dealer, false, current_bet);
    
    // Check for blackjacks
    bool player_blackjack = is_blackjack(&player);
    bool dealer_blackjack = is_blackjack(&dealer);
    
    if (player_blackjack || dealer_blackjack) {
        // Show dealer's card
        display_blackjack_table(&player, &dealer, true, current_bet);
        
        if (player_blackjack && dealer_blackjack) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
            print("  Both have Blackjack! Push (tie).\n");
            player_credits += current_bet;  // Return bet
        } else if (player_blackjack) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            print("  BLACKJACK! You win ");
            int winnings = (current_bet * 3) / 2 + current_bet;  // 3:2 payout
            char buf[20];
            itoa(winnings, buf, 10);
            print(buf);
            print(" credits!\n");
            player_credits += winnings;
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
            print("  Dealer has Blackjack. You lose.\n");
        }
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("\n  Press any key...");
        while (keyboard_key() == -1);
        return;
    }
    
    // Player's turn
    bool player_busted = false;
    bool player_stands = false;
    
    while (!player_busted && !player_stands) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        print("  [H]it  [S]tand  [D]ouble Down");
        
        // Can only double down on first two cards
        if (player.count > 2) {
            print("            ");
        }
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("\n  Your choice: ");
        
        int choice = -1;
        while (choice == -1) {
            int key = keyboard_key();
            if (key == -1) continue;
            
            // H key
            if (key == 0x23) {
                choice = 1;  // Hit
            }
            // S key
            else if (key == 0x1F) {
                choice = 2;  // Stand
            }
            // D key (only if first two cards and enough credits)
            else if (key == 0x20 && player.count == 2 && player_credits >= current_bet) {
                choice = 3;  // Double down
            }
        }
        
        if (choice == 1) {  // Hit
            add_card(&player, deal_card(&deck));
            display_blackjack_table(&player, &dealer, false, current_bet);
            
            if (calculate_hand_value(&player) > 21) {
                player_busted = true;
            }
        } else if (choice == 2) {  // Stand
            player_stands = true;
        } else if (choice == 3) {  // Double down
            player_credits -= current_bet;
            current_bet *= 2;
            add_card(&player, deal_card(&deck));
            display_blackjack_table(&player, &dealer, false, current_bet);
            
            if (calculate_hand_value(&player) > 21) {
                player_busted = true;
            } else {
                player_stands = true;
            }
        }
    }
    
    // Check if player busted
    if (player_busted) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("  BUST! You lose.\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("\n  Press any key...");
        while (keyboard_key() == -1);
        return;
    }
    
    // Dealer's turn (must hit on 16 or less, stand on 17+)
    display_blackjack_table(&player, &dealer, true, current_bet);
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    print("  Dealer reveals...\n");
    print("  Press any key...");
    while (keyboard_key() == -1);
    
    bool dealer_busted = false;
    while (calculate_hand_value(&dealer) < 17) {
        add_card(&dealer, deal_card(&deck));
        display_blackjack_table(&player, &dealer, true, current_bet);
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        print("  Dealer hits...\n");
        print("  Press any key...");
        while (keyboard_key() == -1);
        
        if (calculate_hand_value(&dealer) > 21) {
            dealer_busted = true;
            break;
        }
    }
    
    // Determine winner
    int player_value = calculate_hand_value(&player);
    int dealer_value = calculate_hand_value(&dealer);
    
    print("\n");
    
    if (dealer_busted) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        print("  Dealer BUSTS! You win ");
        char buf[20];
        itoa(current_bet * 2, buf, 10);
        print(buf);
        print(" credits!\n");
        player_credits += current_bet * 2;
    } else if (player_value > dealer_value) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        print("  You win ");
        char buf[20];
        itoa(current_bet * 2, buf, 10);
        print(buf);
        print(" credits!\n");
        player_credits += current_bet * 2;
    } else if (player_value < dealer_value) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("  Dealer wins. You lose.\n");
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
        print("  Push (tie). Bet returned.\n");
        player_credits += current_bet;
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("\n  Press any key...");
    while (keyboard_key() == -1);
}

// Blackjack main function
static void play_blackjack() {
    while (1) {
        play_blackjack_round();
        
        // Ask if player wants to play again
        terminal_clear();
        terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        print("\n  Play another hand?\n\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("  [Y]es  [N]o\n\n");
        print("  Your choice: ");
        
        while (1) {
            int key = keyboard_key();
            if (key == -1) continue;
            
            // Y key
            if (key == 0x15) {
                break;  // Play again
            }
            // N key
            else if (key == 0x31) {
                return;  // Exit blackjack
            }
        }
    }
}

// Display the slot machine interface
static void display_slots(int reel1, int reel2, int reel3, bool spinning) {
    terminal_clear();
    
    // Title
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    print("\n");
    print("  =============================================\n");
    print("          LUCKY 7 SLOT MACHINE\n");
    print("  =============================================\n\n");
    
    // Credits and bet display
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("  Credits: ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    char buf[20];
    itoa(player_credits, buf, 10);
    print(buf);
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("    Bet: ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    itoa(bet_amount, buf, 10);
    print(buf);
    print("\n\n");
    
    // Slot machine display
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    print("         +-----+-----+-----+\n");
    print("         |     |     |     |\n");
    print("         |  ");
    
    // Display reels
    if (spinning) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK));
        print("?  |  ?  |  ?");
    } else {
        const SlotSymbol* s1 = get_symbol(reel1);
        const SlotSymbol* s2 = get_symbol(reel2);
        const SlotSymbol* s3 = get_symbol(reel3);
        
        // Color based on symbol value
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        terminal_putchar(s1->symbol);
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        print("  |  ");
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
        terminal_putchar(s2->symbol);
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        print("  |  ");
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_putchar(s3->symbol);
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    print("  |\n");
    print("         |     |     |     |\n");
    print("         +-----+-----+-----+\n\n");
}

// Calculate winnings based on reel results
static int calculate_winnings(int reel1, int reel2, int reel3) {
    const SlotSymbol* s1 = get_symbol(reel1);
    const SlotSymbol* s2 = get_symbol(reel2);
    const SlotSymbol* s3 = get_symbol(reel3);
    
    // Three matching symbols
    if (reel1 == reel2 && reel2 == reel3) {
        return s1->payout_3 * bet_amount;
    }
    
    // Two matching symbols
    if (reel1 == reel2 || reel2 == reel3 || reel1 == reel3) {
        const SlotSymbol* match;
        if (reel1 == reel2) match = s1;
        else if (reel2 == reel3) match = s2;
        else match = s1;
        
        return match->payout_2 * bet_amount;
    }
    
    // No match
    return 0;
}

// Display result message
static void display_result(int winnings, int reel1, int reel2, int reel3) {
    const SlotSymbol* s1 = get_symbol(reel1);
    const SlotSymbol* s2 = get_symbol(reel2);
    const SlotSymbol* s3 = get_symbol(reel3);
    
    print("  Result: ");
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    print(s1->name);
    print(" - ");
    print(s2->name);
    print(" - ");
    print(s3->name);
    print("\n\n");
    
    if (winnings > 0) {
        // Check for jackpot (three 7s)
        if (reel1 == 0 && reel2 == 0 && reel3 == 0) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
            print("  *** JACKPOT!!! ***\n");
        } else if (reel1 == reel2 && reel2 == reel3) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
            print("  *** THREE OF A KIND! ***\n");
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
            print("  ** WINNER! **\n");
        }
        
        print("  You won: ");
        char buf[20];
        itoa(winnings, buf, 10);
        print(buf);
        print(" credits!\n");
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("  No match. Better luck next time!\n");
    }
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("\n");
}

// Spin animation
static void spin_animation(int final_reel1, int final_reel2, int final_reel3) {
    // Show spinning for a few iterations
    for (int i = 0; i < 10; i++) {
        display_slots(0, 0, 0, true);
        
        // Delay
        for (volatile int j = 0; j < 5000000; j++);
    }
    
    // Reveal reels one by one with delay
    for (volatile int j = 0; j < 8000000; j++);
    display_slots(final_reel1, -1, -1, false);
    
    for (volatile int j = 0; j < 8000000; j++);
    display_slots(final_reel1, final_reel2, -1, false);
    
    for (volatile int j = 0; j < 8000000; j++);
    display_slots(final_reel1, final_reel2, final_reel3, false);
}

// Display help/paytable
static void display_help() {
    terminal_clear();
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    print("\n  PAYTABLE\n");
    print("  ========\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("  Symbol    3 Match    2 Match\n");
    print("  ------    -------    -------\n");
    
    for (int i = 0; i < NUM_SYMBOLS; i++) {
        print("    ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        terminal_putchar(symbols[i].symbol);
        print("  ");
        print(symbols[i].name);
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("     ");
        char buf[10];
        itoa(symbols[i].payout_3, buf, 10);
        print(buf);
        print("x        ");
        itoa(symbols[i].payout_2, buf, 10);
        print(buf);
        print("x\n");
    }
    
    print("\n  Note: Payouts are multiplied by your bet amount\n");
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK));
    print("\n  Press any key to return...");
    
    while (keyboard_key() == -1);
}

// Main slots game
static void play_slots() {
    if (player_credits < bet_amount) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("  Insufficient credits! Game Over.\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("  Press any key to return...");
        while (keyboard_key() == -1);
        return;
    }
    
    // Deduct bet
    player_credits -= bet_amount;
    
    // Generate random reel results
    int reel1 = rand_range(0, NUM_SYMBOLS - 1);
    int reel2 = rand_range(0, NUM_SYMBOLS - 1);
    int reel3 = rand_range(0, NUM_SYMBOLS - 1);
    
    // Show spin animation
    spin_animation(reel1, reel2, reel3);
    
    // Calculate winnings
    int winnings = calculate_winnings(reel1, reel2, reel3);
    player_credits += winnings;
    
    // Display result
    display_result(winnings, reel1, reel2, reel3);
    
    print("  Press any key to continue...");
    while (keyboard_key() == -1);
}

// Adjust bet
static void adjust_bet() {
    terminal_clear();
    terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
    print("\n  ADJUST BET\n");
    print("  ==========\n\n");
    
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    print("  Current bet: ");
    char buf[20];
    itoa(bet_amount, buf, 10);
    print(buf);
    print("\n\n");
    
    print("  1. 5 credits\n");
    print("  2. 10 credits\n");
    print("  3. 20 credits\n");
    print("  4. 50 credits\n");
    print("  5. 100 credits\n\n");
    print("  Select bet amount (1-5): ");
    
    while (1) {
        int key = keyboard_key();
        if (key == -1) continue;
        
        if (key >= 0x02 && key <= 0x06) { // Keys 1-5
            int choice = key - 0x01;
            switch (choice) {
                case 1: bet_amount = 5; break;
                case 2: bet_amount = 10; break;
                case 3: bet_amount = 20; break;
                case 4: bet_amount = 50; break;
                case 5: bet_amount = 100; break;
            }
            return;
        }
    }
}

// Main stomp/casino menu
void stomp() {
    // Seed random number generator with timer
    srand(get_ticks());
    
    terminal_clear();
    
    while (1) {
        terminal_clear();
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
        print("\n");
        print("  =============================================\n");
        print("               CASINO GAMES\n");
        print("  =============================================\n\n");
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("  Credits: ");
        terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
        char buf[20];
        itoa(player_credits, buf, 10);
        print(buf);
        print("\n\n");
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        print("  1. Slots\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        print("  2. Blackjack (Coming Soon)\n");
        print("  3. Roulette (Coming Soon)\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK));
        print("  4. Adjust Bet\n");
        print("  5. Paytable\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("  6. Add Credits (Cheat)\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK));
        print("  7. Exit Casino\n\n");
        
        terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
        print("  Select option (1-7): ");
        
        int choice = -1;
        while (choice == -1) {
            int key = keyboard_key();
            if (key == -1) continue;
            
            if (key >= 0x02 && key <= 0x08) { // Keys 1-7
                choice = key - 0x01;
            }
        }
        
        switch (choice) {
            case 1: // Slots
                play_slots();
                break;
                
            case 2: // Blackjack (placeholder)
            case 3: // Roulette (placeholder)
                play_blackjack();
                break;
                
            case 4: // Adjust bet
                adjust_bet();
                break;
                
            case 5: // Paytable
                display_help();
                break;
                
            case 6: // Add credits (cheat)
                player_credits += 100;
                terminal_setcolor(vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
                print("\n  Added 100 credits!\n");
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
                print("  Press any key...");
                while (keyboard_key() == -1);
                break;
                
            case 7: // Exit
                return;
        }
    }
}