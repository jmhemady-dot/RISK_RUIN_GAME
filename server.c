#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define MAX 2048

typedef struct {
    char name[31];
    int hp;
    int money;
    int spy_count;
    int loan_timer;
} Player;

void send_msg(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

int recv_msg(int sock, char *buffer) {
    bzero(buffer, MAX);
    int n = recv(sock, buffer, MAX - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        buffer[strcspn(buffer, "\r\n")] = 0;
    }
    return n;
}

void send_instructions(int sock) {
    char *rules =
      "\n===============================================================\n"
      "                WELCOME TO THE RISK OR RUIN GAME                \n"
      "===============================================================\n"

      "GAME OVERVIEW:\n"
      "You and your opponent are in a battle of survival and strategy.\n"
      "Manage your money, predict your opponent, and outplay them.\n"
      "\n"
      "Each turn, both players choose an action.\n"
      "Actions can deal damage, generate money, or set up future plays.\n"
      "\n"
      "Be careful with loans — they give instant advantage,\n"
      "but failing to repay will cost you HP.\n"
      "\n"
      "Win by reducing your opponent's HP to 0.\n"
      "Think ahead. Every decision matters.\n"

      "---------------------------------------------------------------\n"
      "COMMANDS:\n"
      " - work   : Gain $2. No risk.\n"
      " - attack : Costs $2, deals 2 DMG.\n"
      " - defend : Costs $1, blocks 1 DMG.\n"
      " - loan   : Gain $5 instantly. In 3 turns, pay\n"
      "            back $7 or lose 2 HP.\n"
      " - spy    : Costs 1 Spy charge. Reveal enemy\n"
      "            move and choose again.\n"
      " - allin  : Costs EVERYTHING. Deals 4 DMG.\n"
      "===============================================================\n\n";
    send_msg(sock, rules);
}

int is_spy(char *action, Player *p) {
    return (strncmp(action, "spy", 3) == 0 && p->spy_count > 0);
}

int is_valid_action(char *a, Player *p) {
    if (strcmp(a, "work") == 0) return 1;
    if (strcmp(a, "attack") == 0 && p->money >= 2) return 1;
    if (strcmp(a, "defend") == 0 && p->money >= 1) return 1;
    if (strcmp(a, "loan") == 0 && p->loan_timer == -1) return 1;
    if (strcmp(a, "spy") == 0 && p->spy_count > 0) return 1;
    if (strcmp(a, "allin") == 0 && p->money > 0) return 1;
    return 0;
}

int main(int argc, char *argv[]) {
    int server_sock, p1_sock, p2_sock, port;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);
    char buffer[MAX];
    
    Player p1 = {"", 10, 5, 2, -1};
    Player p2 = {"", 10, 5, 2, -1};

    if (argc < 2) {
        printf("Usage: %s port\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 2);

    printf("Waiting for Player 1...\n");
    p1_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);
    send_msg(p1_sock, "Connected! Waiting for Player 2...\n");

    printf("Waiting for Player 2...\n");
    p2_sock = accept(server_sock, (struct sockaddr*)&client_addr, &len);

    // NAME INPUT
    send_msg(p1_sock, "Opponent joined! Enter your IGN: ");
    recv_msg(p1_sock, buffer);
    strncpy(p1.name, buffer, sizeof(p1.name) - 1);

    send_msg(p2_sock, "Player 1 is ready! Enter your IGN: ");
    recv_msg(p2_sock, buffer);
    strncpy(p2.name, buffer, sizeof(p2.name) - 1);

    send_instructions(p1_sock);
    send_instructions(p2_sock);

    while (p1.hp > 0 && p2.hp > 0) {

        // ===== LOAN COUNTDOWN (FAIR) =====
        if (p1.loan_timer > 0) {
            p1.loan_timer--;
            if (p1.loan_timer == 0) {
                if (p1.money >= 7) {
                    p1.money -= 7;
                    send_msg(p1_sock, "[INFO] Loan paid successfully.\n");
                } else {
                    p1.hp -= 2;
                    send_msg(p1_sock, "[PENALTY] Failed to pay loan. -2 HP\n");
                }
                p1.loan_timer = -1;
            }
        }

        if (p2.loan_timer > 0) {
            p2.loan_timer--;
            if (p2.loan_timer == 0) {
                if (p2.money >= 7) {
                    p2.money -= 7;
                    send_msg(p2_sock, "[INFO] Loan paid successfully.\n");
                } else {
                    p2.hp -= 2;
                    send_msg(p2_sock, "[PENALTY] Failed to pay loan. -2 HP\n");
                }
                p2.loan_timer = -1;
            }
        }

        // ===== PRESSURE SYSTEM =====

        if (p1.loan_timer > 0) {
            snprintf(buffer, MAX,
                "[DEBT] %d turns left | You owe $7\n",
                p1.loan_timer);
            send_msg(p1_sock, buffer);

            if (p1.loan_timer == 2)
                send_msg(p1_sock, "[WARNING] 2 turns left. Prepare funds.\n");

            if (p1.loan_timer == 1)
                send_msg(p1_sock, "[CRITICAL] FINAL TURN! PAY OR LOSE HP!\n");

            if (p1.loan_timer == 1 && p1.money < 7)
                send_msg(p1_sock, "[CRITICAL] You cannot afford the loan! WORK NOW!\n");
        }

        if (p2.loan_timer > 0) {
            snprintf(buffer, MAX,
                "[DEBT] %d turns left | You owe $7\n",
                p2.loan_timer);
            send_msg(p2_sock, buffer);

            if (p2.loan_timer == 2)
                send_msg(p2_sock, "[WARNING] 2 turns left. Prepare funds.\n");

            if (p2.loan_timer == 1)
                send_msg(p2_sock, "[CRITICAL] FINAL TURN! PAY OR LOSE HP!\n");

            if (p2.loan_timer == 1 && p2.money < 7)
                send_msg(p2_sock, "[CRITICAL] You cannot afford the loan! WORK NOW!\n");
        }

        if (p2.loan_timer > 0)
            send_msg(p1_sock, "[INFO] Opponent is in debt.\n");

        if (p1.loan_timer > 0)
            send_msg(p2_sock, "[INFO] Opponent is in debt.\n");

        // ===== STATUS DISPLAY =====
        char loan1[64], loan2[64];

        if (p1.loan_timer > 0)
            snprintf(loan1, sizeof(loan1),
                "ACTIVE (%d turns left, owe $7)", p1.loan_timer);
        else
            strcpy(loan1, "NONE");

        if (p2.loan_timer > 0)
            snprintf(loan2, sizeof(loan2),
                "ACTIVE (%d turns left, owe $7)", p2.loan_timer);
        else
            strcpy(loan2, "NONE");

        snprintf(buffer, MAX,
        "\n"
        "============================================\n"
        "                STATUS PANEL                \n"
        "============================================\n"

        "  [%s]\n"
        "  HP    : %d\n"
        "  MONEY : $%d\n"
        "  SPY   : %d\n"
        "  LOAN  : %s\n"
        "--------------------------------------------\n"
        "  [%s]\n"
        "  HP    : %d\n"
        "  MONEY : $%d\n"
        "  SPY   : %d\n"
        "  LOAN  : %s\n"
        "============================================\n",

        p1.name, p1.hp, p1.money, p1.spy_count, loan1,
        p2.name, p2.hp, p2.money, p2.spy_count, loan2
        );
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);

        // ===== ACTION INPUT =====
        char p1_action[MAX], p2_action[MAX];

        // Player 1 validation
        do {
            send_msg(p1_sock, "Choose action: ");
            recv_msg(p1_sock, p1_action);

            if (!is_valid_action(p1_action, &p1)) {
                send_msg(p1_sock,
                    "[ERROR] Invalid action or insufficient resources.\n");
            }

        } while (!is_valid_action(p1_action, &p1));

        // Player 2 validation
        do {
            send_msg(p2_sock, "Choose action: ");
            recv_msg(p2_sock, p2_action);

            if (!is_valid_action(p2_action, &p2)) {
                send_msg(p2_sock,
                    "[ERROR] Invalid action or insufficient resources.\n");
            }

        } while (!is_valid_action(p2_action, &p2));

        int p1_spy = is_spy(p1_action, &p1);
        int p2_spy = is_spy(p2_action, &p2);

        if (p1_spy && p2_spy) {
            p1.spy_count--;
            p2.spy_count--;

            send_msg(p1_sock, "[SPY] Both players used spy. No information gained. Both lose 1 spy!\n");
            send_msg(p2_sock, "[SPY] Both players used spy. No information gained. Both lose 1 spy!\n");
        }
        else if (p1_spy && !p2_spy) {
            p1.spy_count--;
            snprintf(buffer, MAX, "[SPY] Opponent chose: %.50s\n", p2_action);
            send_msg(p1_sock, buffer);

            do {
                send_msg(p1_sock, "Choose action: ");
                recv_msg(p1_sock, p1_action);

                if (!is_valid_action(p1_action, &p1)) {
                    send_msg(p1_sock, "[ERROR] Invalid action.\n");
                }

            } while (!is_valid_action(p1_action, &p1));

            p1_spy = is_spy(p1_action, &p1);
        }
        else if (p2_spy && !p1_spy) {
            p2.spy_count--;
            snprintf(buffer, MAX, "[SPY] Opponent chose: %.50s\n", p1_action);
            send_msg(p2_sock, buffer);

            do {
                send_msg(p2_sock, "Choose action: ");
                recv_msg(p2_sock, p2_action);

                if (!is_valid_action(p2_action, &p2)) {
                    send_msg(p2_sock, "[ERROR] Invalid action.\n");
                }

            } while (!is_valid_action(p2_action, &p2));

            p2_spy = is_spy(p2_action, &p2);
        }

        int d1 = 0, d2 = 0;

        // ===== ACTIONS =====
        if (strcmp(p1_action, "attack") == 0 && p1.money >= 2) {
            p1.money -= 2; d2 = 2;
        } else if (strcmp(p1_action, "defend") == 0 && p1.money >= 1) {
            p1.money -= 1;
        } else if (strcmp(p1_action, "work") == 0) {
            p1.money += 2;
        } else if (strcmp(p1_action, "loan") == 0) {
            if (p1.loan_timer == -1) {
                p1.money += 5;
                p1.loan_timer = 3;
                send_msg(p1_sock, "[INFO] Loan taken: +$5. Pay $7 in 3 turns.\n");
            } else {
                send_msg(p1_sock, "[WARNING] You already have an active loan.\n");
            }
        } else if (strcmp(p1_action, "allin") == 0) {
            if (p1.money <= 0) {
                send_msg(p1_sock, "[ERROR] No money for ALL-IN!\n");
            } else {
                d2 = 4;
                p1.money = 0;
                send_msg(p1_sock, "[ALL-IN] You sacrificed everything for 4 DMG!\n");
            }
        }

        if (strcmp(p2_action, "attack") == 0 && p2.money >= 2) {
            p2.money -= 2; d1 = 2;
        } else if (strcmp(p2_action, "defend") == 0 && p2.money >= 1) {
            p2.money -= 1;
        } else if (strcmp(p2_action, "work") == 0) {
            p2.money += 2;
        } else if (strcmp(p2_action, "loan") == 0) {
            if (p2.loan_timer == -1) {
                p2.money += 5;
                p2.loan_timer = 3;
                send_msg(p2_sock, "[INFO] Loan taken: +$5. Pay $7 in 3 turns.\n");
            } else {
                send_msg(p2_sock, "[WARNING] You already have an active loan.\n");
            }
          } else if (strcmp(p2_action, "allin") == 0) {
              if (p2.money <= 0) {
                  send_msg(p2_sock, "[ERROR] No money for ALL-IN!\n");
              } else {
                  d1 = 4;
                  p2.money = 0;
                  send_msg(p2_sock, "[ALL-IN] You sacrificed everything for 4 DMG!\n");
              }
          }

        if (strcmp(p1_action, "defend") == 0) d1--;
        if (strcmp(p2_action, "defend") == 0) d2--;

        if (d1 < 0) d1 = 0;
        if (d2 < 0) d2 = 0;

        p1.hp -= d1;
        p2.hp -= d2;

        send_msg(p1_sock, "Round resolved!\n");
        send_msg(p2_sock, "Round resolved!\n");
    }

    // FINAL RESULT
    int p1_score = 0, p2_score = 0;

    if (p1.hp <= 0 && p2.hp <= 0) {
        send_msg(p1_sock, "Draw!\n");
        send_msg(p2_sock, "Draw!\n");
    }
    else if (p1.hp <= 0) {
        p2_score = 1;
        snprintf(buffer, MAX, "%s wins!\n", p2.name);
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
    }
    else {
        p1_score = 1;
        snprintf(buffer, MAX, "%s wins!\n", p1.name);
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
    }

    snprintf(buffer, MAX,
        "\nFINAL SCORE: %s (%d) vs %s (%d)\nGAME OVER\n",
        p1.name, p1_score,
        p2.name, p2_score);

    send_msg(p1_sock, buffer);
    send_msg(p2_sock, buffer);

    close(p1_sock);
    close(p2_sock);
    close(server_sock);
    return 0;
}
