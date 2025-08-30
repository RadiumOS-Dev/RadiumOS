#include "../terminal/terminal.h"
#include "../keyboard/keyboard.h"
#include "../utility/utility.h"
#include "../timers/timer.h"
#define M 3
#define S 256
#define L 300
#define I 5
#define U 32
#define K 32

void C(char *d, size_t s) {
    volatile char *p = d;
    while (s--) *p++ = 0;
}

void secure_clear(char *d, size_t s) {
    volatile char *p = d;
    while (s--) *p++ = 0;
}

int v(const char* i, int m) {
    return strlen(i) > 0 && strlen(i) <= m;
}

int login() {
    char u[S], p[S], k[S];
    int a = 0, l = I, c = 0;

    while (1) {
        if (c > 0) {
            print("\nAccount locked. Please wait.");
            for (int i = 0; i < l; i++) {
                delay(10000);
                print(".");
            }
            c = 0; a = 0; l = I; continue;
        }

        print("\nUsername: ");
        keyboard_input(u);
        if (!v(u, U)) {
            print("\nInvalid username.");
            continue;
        }

        print("\nPassword: ");
        keyboard_input_secure(p);
        print("\nKeyPhrase: ");
        keyboard_input_secure(k);
        if (!v(k, K)) {
            print("\nInvalid keyphrase.");
            C(p, sizeof(p)); continue;
        }

        if (!strcmp(u, "root") && !strcmp(p, "toor") && !strcmp(k, "meow")) {
            C(p, sizeof(p)); C(k, sizeof(k)); C(u, sizeof(u));
            return 0;
        } else {
            print("\nInvalid username or password.");
            a++;
            if (a >= M) {
                print("\nToo many failed attempts. Account locked.");
                c = l; l = (l < L) ? l * 2 : L; a = 0;
            }
        }
        C(p, sizeof(p)); C(k, sizeof(k)); C(u, sizeof(u));
    }
    return -1;
}
