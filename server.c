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

void send_instructions(int sock, Player *p) {
    char rules[MAX];
  
    snprintf(rules, MAX,
    "\n===============================================================\n"
    "             WELCOME %s TO THE RISK OR RUIN GAME\n"
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
    " - defend : Costs $1, reduces incoming damage by 50%%.\n"
    " - loan   : Gain $5 instantly. In 3 turns, pay\n"
    "            back $7 or lose 3 HP.\n"
    " - spy    : Costs 1 Spy charge. Reveal enemy\n"
    "            move and choose again.\n"
    " - allin  : Costs EVERYTHING.\n" 
    "            Deals damage equal to HALF your money.\n"
    "===============================================================\n\n",
    p->name
    );

    send_msg(sock, rules);
}

int is_spy(char *action, Player *p) {
    return (strncmp(action, "spy", 3) == 0 && p->spy_count > 0);
}

int is_valid_action(char *a, Player *p) {
    if (strcmp(a, "work") == 0) return 1;
    if (strcmp(a, "attack") == 0) return 1;
    if (strcmp(a, "defend") == 0) return 1;
    if (strcmp(a, "loan") == 0) return 1;
    if (strcmp(a, "spy") == 0 && p->spy_count > 0) return 1;
    if (strcmp(a, "allin") == 0) return 1;
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

    send_msg(p2_sock, "Connected! Waiting for Player 1 to enter IGN...\n");

    // ===== NAME INPUT =====

    // Ask Player 1 first
    send_msg(p1_sock, "Opponent Joined! Enter your IGN: ");
    recv_msg(p1_sock, buffer);
    strncpy(p1.name, buffer, sizeof(p1.name) - 1);

    // Notify Player 1
    send_msg(p1_sock, "Waiting for Player 2 to enter IGN...\n");

    // Notify Player 2 to input
    send_msg(p2_sock, "Player 1 is ready! Enter your IGN: ");
    recv_msg(p2_sock, buffer);
    // ===== NAME INPUT DONE =====
    strncpy(p2.name, buffer, sizeof(p2.name) - 1);

    // THEN continue
    send_instructions(p1_sock, &p1);
    send_instructions(p2_sock, &p2);
    
    // ===== READY SYSTEM (PLACE IT HERE) =====
    int p1_ready = 0;
    int p2_ready = 0;

    send_msg(p1_sock, "\n[READ] Press ENTER after reading instructions...\n");
    send_msg(p2_sock, "\n[READ] Press ENTER after reading instructions...\n");

    fd_set readfds;

    while (!p1_ready || !p2_ready) {
        FD_ZERO(&readfds);
        FD_SET(p1_sock, &readfds);
        FD_SET(p2_sock, &readfds);

        int maxfd = (p1_sock > p2_sock ? p1_sock : p2_sock) + 1;

        select(maxfd, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(p1_sock, &readfds) && !p1_ready) {
            recv_msg(p1_sock, buffer);
            p1_ready = 1;

            send_msg(p1_sock, "[READY] You are ready.\n");
            send_msg(p2_sock, "[READY] Player 1 is ready.\n");
        }

        if (FD_ISSET(p2_sock, &readfds) && !p2_ready) {
            recv_msg(p2_sock, buffer);
            p2_ready = 1;

            send_msg(p2_sock, "[READY] You are ready.\n");
            send_msg(p1_sock, "[READY] Player 2 is ready.\n");
        }
    }

    // ===== START GAME =====
    send_msg(p1_sock, "[START] Both players ready. Game starting...\n");
    send_msg(p2_sock, "[START] Both players ready. Game starting...\n");


    int round = 1;
    int p1_total_damage = 0, p2_total_damage = 0;
    int p1_spy_count = 0, p2_spy_count = 0;
    
    while (p1.hp > 0 && p2.hp > 0) {
    
    int p1_actions = 0, p2_actions = 0;

    int p1_attack = 0, p1_defend = 0, p1_work = 0, p1_loan = 0, p1_allin = 0;
    int p2_attack = 0, p2_defend = 0, p2_work = 0, p2_loan = 0, p2_allin = 0;
        
    char p1_action[MAX], p2_action[MAX];
    char p1_display[64], p2_display[64];
    char summary[MAX];

        // ===== LOAN COUNTDOWN (FAIR) =====
        if (p1.loan_timer > 0) {
            p1.loan_timer--;
            if (p1.loan_timer == 0) {
                if (p1.money >= 7) {
                    p1.money -= 7;
                    send_msg(p1_sock, "[INFO] Loan paid successfully.\n");
                } else {
                    p1.hp -= 3;
                    send_msg(p1_sock, "[PENALTY] Failed to pay loan. -3 HP\n");
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
                    p2.hp -= 3;
                    send_msg(p2_sock, "[PENALTY] Failed to pay loan. -3 HP\n");
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

        sprintf(buffer,
            "\n== ROUND %d =================================\n"
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

            round,
            p1.name, p1.hp, p1.money, p1.spy_count, loan1,
            p2.name, p2.hp, p2.money, p2.spy_count, loan2
        );
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
        
        // ===== SMART TIPS =====

        // Player 1 tips
        if (p1.money < 2) {
            send_msg(p1_sock, "[TIP] Low money. Consider WORK.\n");
        }

        if (p1.loan_timer == 1) {
            send_msg(p1_sock, "[WARNING] Final turn to pay your loan!\n");
        }

        if (p2.hp <= 3 && p1.money >= 4) {
            send_msg(p1_sock, "[TIP] You can finish with ALL-IN!\n");
        }

        if (p2.money >= 7) {
            send_msg(p1_sock, "[TIP] Opponent is rich. Consider DEFEND or SPY. Play Smart!\n");
        }

        // Player 2 tips
        if (p2.money < 2) {
            send_msg(p2_sock, "[TIP] Low money. Consider WORK.\n");
        }

        if (p2.loan_timer == 1) {
            send_msg(p2_sock, "[WARNING] Final turn to pay your loan!\n");
        }

        if (p1.hp <= 3 && p2.money >= 4) {
            send_msg(p2_sock, "[TIP] You can finish with ALL-IN!\n");
        }

        if (p1.money >= 7) {
            send_msg(p2_sock, "[TIP] Opponent is rich. Consider DEFEND.\n");
        }

        // ===== ACTION INPUT (SIMULTANEOUS) =====
        int p1_done = 0, p2_done = 0;
        fd_set readfds;

        // Send prompt to BOTH players first
        send_msg(p1_sock, "Choose action: ");
        send_msg(p2_sock, "Choose action: ");

        while (!p1_done || !p2_done) {
            FD_ZERO(&readfds);
            FD_SET(p1_sock, &readfds);
            FD_SET(p2_sock, &readfds);

            int maxfd = (p1_sock > p2_sock ? p1_sock : p2_sock) + 1;

            select(maxfd, &readfds, NULL, NULL, NULL);

            // ===== PLAYER 1 =====
            if (FD_ISSET(p1_sock, &readfds) && !p1_done) {
                recv_msg(p1_sock, p1_action);

                if (!is_valid_action(p1_action, &p1)) {
                    send_msg(p1_sock, "[ERROR] Invalid action.\n");
                    send_msg(p1_sock, "Choose action: ");
                }
                else if (strcmp(p1_action, "attack") == 0 && p1.money < 2) {
                    send_msg(p1_sock, "[ERROR] Not enough money!\n");
                    send_msg(p1_sock, "Choose action: ");
                }
                else if (strcmp(p1_action, "defend") == 0 && p1.money < 1) {
                    send_msg(p1_sock, "[ERROR] Not enough money!\n");
                    send_msg(p1_sock, "Choose action: ");
                }
                else if (strcmp(p1_action, "allin") == 0 && p1.money <= 0) {
                    send_msg(p1_sock, "[ERROR] No money for ALL-IN!\n");
                    send_msg(p1_sock, "Choose action: ");
                }
                else if (strcmp(p1_action, "loan") == 0 && p1.loan_timer != -1) {
                    send_msg(p1_sock, "[WARNING] You already have an active loan.\n");
                    send_msg(p1_sock, "Choose action: ");
                }
                else {
                    p1_done = 1;
                }
            }

            // ===== PLAYER 2 =====
            if (FD_ISSET(p2_sock, &readfds) && !p2_done) {
                recv_msg(p2_sock, p2_action);

                if (!is_valid_action(p2_action, &p2)) {
                    send_msg(p2_sock, "[ERROR] Invalid action.\n");
                    send_msg(p2_sock, "Choose action: ");
                }
                else if (strcmp(p2_action, "attack") == 0 && p2.money < 2) {
                    send_msg(p2_sock, "[ERROR] Not enough money!\n");
                    send_msg(p2_sock, "Choose action: ");
                }
                else if (strcmp(p2_action, "defend") == 0 && p2.money < 1) {
                    send_msg(p2_sock, "[ERROR] Not enough money!\n");
                    send_msg(p2_sock, "Choose action: ");
                }
                else if (strcmp(p2_action, "allin") == 0 && p2.money <= 0) {
                    send_msg(p2_sock, "[ERROR] No money for ALL-IN!\n");
                    send_msg(p2_sock, "Choose action: ");
                }
                else if (strcmp(p2_action, "loan") == 0 && p2.loan_timer != -1) {
                    send_msg(p2_sock, "[WARNING] You already have an active loan.\n");
                    send_msg(p2_sock, "Choose action: ");
                }
                else {
                    p2_done = 1;
                }
            }
        }

        int p1_spy = is_spy(p1_action, &p1);
        int p2_spy = is_spy(p2_action, &p2);

        if (p1_spy && p2_spy) {
            p1.spy_count--;
            p2.spy_count--;
            
            p1_spy_count++;   
            p2_spy_count++;

            send_msg(p1_sock, "[SPY] Both players used spy. No information gained. Both lose 1 spy!\n");
            send_msg(p2_sock, "[SPY] Both players used spy. No information gained. Both lose 1 spy!\n");
        }
        else if (p1_spy && !p2_spy) {
            p1.spy_count--;
            p1.spy_count++;
            
            snprintf(buffer, MAX, "[SPY] Opponent chose: %.50s\n", p2_action);
            send_msg(p1_sock, buffer);

            do {
                send_msg(p1_sock, "Choose action: ");
                recv_msg(p1_sock, p1_action);

                if (!is_valid_action(p1_action, &p1)) {
                    send_msg(p1_sock, "[ERROR] Invalid action.\n");
                }

            } while (!is_valid_action(p1_action, &p1));
        }
        else if (p2_spy && !p1_spy) {
            p2.spy_count--;
            p2.spy_count++;
            
            snprintf(buffer, MAX, "[SPY] Opponent chose: %.50s\n", p1_action);
            send_msg(p2_sock, buffer);

            do {
                send_msg(p2_sock, "Choose action: ");
                recv_msg(p2_sock, p2_action);

                if (!is_valid_action(p2_action, &p2)) {
                    send_msg(p2_sock, "[ERROR] Invalid action.\n");
                }

            } while (!is_valid_action(p2_action, &p2));

        }

        int d1 = 0, d2 = 0;

        // ===== ACTIONS =====
        if (strcmp(p1_action, "attack") == 0) {
            if (p1.money >= 2) {
                p1.money -= 2;
                d2 = 2;
            }
            p1_attack++;
        }
        else if (strcmp(p1_action, "defend") == 0) {
            if (p1.money >= 1) {
                p1.money -= 1;
            }
            p1_defend++;
        }
        else if (strcmp(p1_action, "work") == 0) {
            p1.money += 2;
            p1_work++;
        }
        else if (strcmp(p1_action, "loan") == 0) {
            p1.money += 5;
            p1.loan_timer = 4;
            p1_loan++;

            send_msg(p1_sock, "[INFO] Loan taken: +$5. Pay $7 in 3 turns.\n");
            send_msg(p1_sock, "[DEBT] You are now in debt!\n");
        }
        else if (strcmp(p1_action, "allin") == 0) {
            int dmg = p1.money / 2;

            if (dmg <= 0) {
                send_msg(p1_sock, "[WEAK ALL-IN] No money! 0 DMG.\n");
            } else {
                d2 = dmg;
                snprintf(buffer, MAX,
                    "[ALL-IN] You used $%d and dealt %d DMG!\n",
                    p1.money, dmg);
                send_msg(p1_sock, buffer);
            }

            p1.money = 0;
            p1_allin++;
        }

       if (strcmp(p2_action, "attack") == 0) {
            if (p2.money >= 2) {
                p2.money -= 2;
                d1 = 2;
            }
            p2_attack++;
        }
        else if (strcmp(p2_action, "defend") == 0) {
            if (p2.money >= 1) {
                p2.money -= 1;
            }
            p2_defend++;
        }
        else if (strcmp(p2_action, "work") == 0) {
            p2.money += 2;
            p2_work++;
        }
        else if (strcmp(p2_action, "loan") == 0) {
            p2.money += 5;
            p2.loan_timer = 4;
            p2_loan++;

            send_msg(p2_sock, "[INFO] Loan taken: +$5. Pay $7 in 3 turns.\n");
            send_msg(p2_sock, "[DEBT] You are now in debt!\n");
        }
        else if (strcmp(p2_action, "allin") == 0) {
            int dmg = p2.money / 2;

            if (dmg <= 0) {
                send_msg(p2_sock, "[WEAK ALL-IN] No money! 0 DMG.\n");
            } else {
                d1 = dmg;
                snprintf(buffer, MAX,
                    "[ALL-IN] You used $%d and dealt %d DMG!\n",
                    p2.money, dmg);
                send_msg(p2_sock, buffer);
            }

            p2.money = 0;
            p2_allin++;
        }

        if (strcmp(p1_action, "defend") == 0) d1 /= 2;
        if (strcmp(p2_action, "defend") == 0) d2 /= 2;

        if (d1 < 0) d1 = 0;
        if (d2 < 0) d2 = 0;

        p1.hp -= d1;
        p2.hp -= d2;
        
        p1_total_damage += d2;  
        p2_total_damage += d1;  
        
        if (p1.hp < 0) p1.hp = 0;
        if (p2.hp < 0) p2.hp = 0;
        
        send_msg(p1_sock, "Round resolved!\n");
        send_msg(p2_sock, "Round resolved!\n");

        // default = final action
        snprintf(p1_display, sizeof(p1_display), "%.10s", p1_action);
        snprintf(p2_display, sizeof(p2_display), "%.10s", p2_action);
            
        snprintf(summary, MAX,
            "\n========= ROUND %d SUMMARY =========\n"
            "  %s used %s | %s used %s\n"
            "-----------------------------------\n"
            "   %s lost %d HP | %s lost %d HP\n"
            "===================================\n",
            round,
            p1.name, p1_display,
            p2.name, p2_display,
            p1.name, d1,
            p2.name, d2
        );

        send_msg(p1_sock, summary);
        send_msg(p2_sock, summary);
        
        if (p1.hp == 0 || p2.hp == 0) {
            break;
        }
        
        round++; 
    }
    
    char final[MAX];

    snprintf(final, MAX,
    "\n=========== MATCH STATS ===========\n"

    "%s\n"
    "  HP Left       : %d\n"
    "  Money Left    : $%d\n"
    "  Total Damage  : %d\n"
    "  Spy Used      : %d\n"
    "----------------------------------\n"
    "%s\n"
    "  HP Left       : %d\n"
    "  Money Left    : $%d\n"
    "  Total Damage  : %d\n"
    "  Spy Used      : %d\n"

    "==================================\n",
    p1.name, p1.hp, p1.money, p1_total_damage, p1_spy_count,
    p2.name, p2.hp, p2.money, p2_total_damage, p2_spy_count
    );

    send_msg(p1_sock, final);
    send_msg(p2_sock, final);
    
    // FINAL RESULT
    int p1_score = 0, p2_score = 0;

    if (p1.hp <= 0 && p2.hp <= 0) {
        send_msg(p1_sock, "Draw!\n");
        send_msg(p2_sock, "Draw!\n");
    }
    else if (p1.hp <= 0) {
        p2_score = 1;
        snprintf(buffer, MAX, "%s WINS!\n", p2.name);
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
    }
    else {
        p1_score = 1;
        snprintf(buffer, MAX, "%s WINS!\n", p1.name);
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
    }

    snprintf(buffer, MAX,
        "\nFINAL SCORE: %s (%d) vs %s (%d)\nGAME OVER!!!\n",
        p1.name, p1_score,
        p2.name, p2_score);

    send_msg(p1_sock, buffer);
    send_msg(p2_sock, buffer);

    close(p1_sock);
    close(p2_sock);
    close(server_sock);
    return 0;
}
