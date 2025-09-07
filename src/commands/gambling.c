#include <stdint.h>
#include <stdbool.h>
#include "../utility/utility.h"
#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../timers/timer.h"

#define COMMAND_BUFFER_SIZE 256

// Custom PRNG implementation
static uint32_t prng_seed = 1;

void custom_srand(uint32_t seed) {
    prng_seed = seed ? seed : 1;
}

uint32_t custom_rand() {
    prng_seed = (1664525 * prng_seed + 1013904223);
    return prng_seed;
}

uint32_t custom_rand_range(uint32_t max) {
    return custom_rand() % max;
}

// Card helpers
int card_value(int card) {
    int rank = card % 13;
    if (rank >= 10) return 10; // J, Q, K
    if (rank == 0) return 11;  // Ace initially 11
    return rank + 1;
}

int hand_total(int* hand, int count) {
    int total = 0;
    int aces = 0;
    for (int i = 0; i < count; i++) {
        int val = card_value(hand[i]);
        total += val;
        if (val == 11) aces++;
    }
    while (total > 21 && aces > 0) {
        total -= 10;
        aces--;
    }
    return total;
}

void print_hand(const char* name, int* hand, int count) {
    print(name);
    print(": ");
    for (int i = 0; i < count; i++) {
        int rank = hand[i] % 13;
        int suit = hand[i] / 13;
        const char* ranks[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
        const char* suits[] = {"♠", "♥", "♦", "♣"};
        print(ranks[rank]);
        print(suits[suit]);
        if (i < count - 1) print(", ");
    }
    print("\n");
}

void gambling_command(int argc, char* argv[]) {
    char userinput[COMMAND_BUFFER_SIZE];
    int balance = 100; // Starting balance
    int loan = 0;
    const int loan_amount = 100;
    const float interest_rate = 0.1f; // 10% interest on loan

    // Seed PRNG once
    static bool seeded = false;
    if (!seeded) {
        custom_srand(get_ticks());
        seeded = true;
    }

    print("Welcome to Blackjack! Your starting balance is 100.\n");

    while (true) {
        // Offer loan if balance <= 0 and no current loan
        if (balance <= 0 && loan == 0) {
            print("Your balance is zero or negative.\n");
            print("Would you like to take a loan of 100 units with 10% interest? (y/n): ");
            keyboard_input(userinput);
            if (userinput[0] == 'y' || userinput[0] == 'Y') {
                loan = loan_amount;
                balance += loan_amount;
                print("Loan granted. You owe 110 units (principal + interest).\n");
                printr("Current balance: %d\n", balance);
            } else {
                print("No loan taken. Game over.\n");
                break;
            }
        }

        // Prompt for bet
        int bet = 0;
        while (true) {
            print("Enter your bet (balance: ");
            printr("%d", balance);
            print("): ");
            keyboard_input(userinput);

            bet = atoi(userinput);
            if (bet <= 0) {
                print("Bet must be a positive number.\n");
            } else if (bet > balance) {
                print("You cannot bet more than your current balance.\n");
            } else {
                break;
            }
        }

        // Initialize and shuffle deck
        int deck[52];
        for (int i = 0; i < 52; i++) deck[i] = i;

        for (int i = 51; i > 0; i--) {
            int j = custom_rand_range(i + 1);
            int temp = deck[i];
            deck[i] = deck[j];
            deck[j] = temp;
        }

        int deck_pos = 0;

        int player_hand[12];
        int dealer_hand[12];
        int player_count = 0;
        int dealer_count = 0;

        // Deal initial cards
        player_hand[player_count++] = deck[deck_pos++];
        dealer_hand[dealer_count++] = deck[deck_pos++];
        player_hand[player_count++] = deck[deck_pos++];
        dealer_hand[dealer_count++] = deck[deck_pos++];

        print("\n--- New Blackjack Game ---\n");

        // Show dealer's first card only
        print_hand("Dealer", dealer_hand, 1);
        print("Dealer: [hidden]\n");

        print_hand("Player", player_hand, player_count);
        printr("Player total: %d\n", hand_total(player_hand, player_count));

        // Player turn
        bool player_bust = false;
        while (true) {
            print("Hit or Stand? (h/s): ");
            keyboard_input(userinput);
            if (strcmp(userinput, "h") == 0 || strcmp(userinput, "hit") == 0) {
                player_hand[player_count++] = deck[deck_pos++];
                print_hand("Player", player_hand, player_count);
                int total = hand_total(player_hand, player_count);
                printr("Player total: %d\n", total);
                if (total > 21) {
                    print("Bust! You lose.\n");
                    player_bust = true;
                    break;
                }
            } else if (strcmp(userinput, "s") == 0 || strcmp(userinput, "stand") == 0) {
                break;
            } else {
                print("Invalid input. Please enter 'h' or 's'.\n");
            }
        }

        int player_total = hand_total(player_hand, player_count);
        if (!player_bust) {
            // Dealer turn
            print_hand("Dealer", dealer_hand, dealer_count);
            int dealer_total = hand_total(dealer_hand, dealer_count);
            printr("Dealer total: %d\n", dealer_total);

            while (dealer_total < 17) {
                print("Dealer hits.\n");
                dealer_hand[dealer_count++] = deck[deck_pos++];
                print_hand("Dealer", dealer_hand, dealer_count);
                dealer_total = hand_total(dealer_hand, dealer_count);
                printr("Dealer total: %d\n", dealer_total);
            }

            if (dealer_total > 21) {
                print("Dealer busts! You win!\n");
                balance += bet;
            } else if (dealer_total > player_total) {
                print("Dealer wins.\n");
                balance -= bet;
            } else if (dealer_total < player_total) {
                print("You win!\n");
                balance += bet;
            } else {
                print("Push (tie).\n");
                // balance unchanged
            }
        } else {
            // Player busts, lose bet
            balance -= bet;
        }

        // If player has loan, try to deduct repayment automatically
        if (loan > 0) {
            int repayment = (int)(loan * (1.0f + interest_rate));
            if (balance >= repayment) {
                balance -= repayment;
                print("Loan repaid with interest.\n");
                loan = 0;
            } else {
                printr("You still owe %d units on your loan.\n", repayment - balance);
            }
        }

        printr("Current balance: %d\n", balance);

        if (balance <= 0 && loan == 0) {
            print("You have no money and no loan. Game over.\n");
            break;
        }

        print("Play again? (y/n): ");
        keyboard_input(userinput);
        if (userinput[0] != 'y' && userinput[0] != 'Y') {
            print("Thanks for playing!\n");
            break;
        }
    }
}
