#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>

#define MAX 8192

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"
#define GOLD    "\x1b[93m"
#define RESET   "\x1b[0m"

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
    "\n"
    CYAN 
    "╔════════════════════════════════════════════════════════════════════╗\n"
    "║                      WELCOME TO RISK OR RUIN                       ║\n"
    "╠════════════════════════════════════════════════════════════════════╣\n"
    RESET
    
    YELLOW
    "║                        PLAYER : %-30s     ║\n"
    RESET
    
    CYAN
    "╚════════════════════════════════════════════════════════════════════╝\n"
    RESET
    
    "\n"
    
    GREEN
    "      A turn-based battle of strategy, prediction, and survival.\n"
    "        Outsmart your opponent, manage your resources wisely,\n"
    "                   and become the final victor.\n"
    RESET
    
    "\n"
    
    GOLD
    "  ┌────────────────────────── OBJECTIVE ──────────────────────────┐\n"
    RESET
    
    WHITE
    "  │ Defeat your opponent by reducing their HP to 0.               │\n"
    "  │ First player to win 2 matches becomes the champion.           │\n"
    RESET
    
    GOLD
    "  └───────────────────────────────────────────────────────────────┘\n"
    RESET

    "\n"
    
    MAGENTA
    "  ┌────────────────────────── COMMANDS ───────────────────────────┐\n"
    RESET
    
    GREEN
    "  │ WORK                                                          │\n"
    RESET
    "  │   Gain $2 safely. No risk involved.                           │\n"

    "  │                                                               │\n"
    RED
    "  │ ATTACK                                                        │\n"
    RESET
    "  │   Cost : $2  •  Deal 2 damage.                                │\n"

    "  │                                                               │\n"
    BLUE
    "  │ DEFEND                                                        │\n"
    RESET
    "  │   Cost : $1  •  Reduce incoming damage by 50%%.                │\n"

    "  │                                                               │\n"
    YELLOW
    "  │ LOAN                                                          │\n"
    RESET
    "  │   Gain $5 instantly.                                          │\n"
    "  │   Repay $7 after 3 turns or lose 3 HP.                        │\n"

    "  │                                                               │\n"
    
    CYAN
    "  │ SPY                                                           │\n"
    RESET
    "  │   Uses 1 Spy charge.                                          │\n"
    "  │   Reveal opponent action and choose again.                    │\n"
    "  │   If both players use SPY, no information                     │\n"
    "  │   is revealed. Both Players loses Spy charge.                 │\n"

    "  │                                                               │\n"

    MAGENTA
    "  │ ALL-IN                                                        │\n"
    RESET
    "  │   Risk ALL your money for massive damage.                     │\n"
    "  │   Damage equals HALF of your total money.                     │\n"
    
    MAGENTA
    "  └───────────────────────────────────────────────────────────────┘\n"
    RESET

    "\n"
    
    GOLD
    "  ┌──────────────────────────── TIPS ────────────────────────────┐\n"
    RESET
    
    WHITE
    "  │ • Watch your opponent's money carefully.                     │\n"
    "  │ • Save SPY charges for important turns.                      │\n"
    "  │ • Loans can save you — or destroy you.                       │\n"
    "  │ • One smart move can change the entire battle.               │\n"
    RESET
    
    GOLD
    "  └──────────────────────────────────────────────────────────────┘\n"
    RESET

    "\n"

    CYAN
    "════════════════════════════════════════════════════════════════════\n"
    RESET
    
    GREEN
    "                        PREPARE FOR BATTLE\n"
    RESET
    
    CYAN
    "════════════════════════════════════════════════════════════════════\n\n"
    RESET,
    
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
    
    send_msg(p1_sock, "\033[2J\033[H");
    send_msg(p2_sock, "\033[2J\033[H");
    
    
    while (1) {
    
    int p1_score = 0;
    int p2_score = 0;
    int round = 1;
    int match = 1;

    int p1_total_damage = 0, p2_total_damage = 0;
    int p1_spy_count = 0, p2_spy_count = 0;

    int p1_attack = 0, p2_attack = 0;
    int p1_defend = 0, p2_defend = 0;
    int p1_work = 0, p2_work = 0;
    int p1_loan = 0, p2_loan = 0;
    int p1_allin = 0, p2_allin = 0;
    
    while (p1_score < 2 && p2_score < 2) {
    
    p1.hp = 10;
    p2.hp = 10;

    p1.money = 5;
    p2.money = 5;

    p1.spy_count = 2;
    p2.spy_count = 2;

    p1.loan_timer = -1;
    p2.loan_timer = -1;

    round = 1;
    
    char prev_summary1[MAX] = "";
    char prev_summary2[MAX] = "";
    
    while (p1.hp > 0 && p2.hp > 0) {
    
    if (round > 1) {

        send_msg(p1_sock, "\033[2J\033[H");
        send_msg(p2_sock, "\033[2J\033[H");

        send_msg(p1_sock, prev_summary1);
        send_msg(p2_sock, prev_summary2);

        send_msg(p1_sock, "\n");
        send_msg(p2_sock, "\n");
    }
    
    int p1_actions = 0, p2_actions = 0;
    
    char penalty1[128] = "";
    char penalty2[128] = "";
    
    char event1[256] = "";
    char event2[256] = "";
    
    char p1_action[MAX], p2_action[MAX];
    char p1_display[64], p2_display[64];
    char summary[MAX];

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
          
          char hp_bar1[128] = "";
          char hp_bar2[128] = "";
          
          char money_bar1[128] = "";
          char money_bar2[128] = "";
          
          char spy_bar1[32] = "";
          char spy_bar2[32] = "";

          // ================= MONEY DISPLAY LIMIT =================

          int money_display1 = p1.money;
          int money_display2 = p2.money;

          if (money_display1 > 10)
              money_display1 = 10;

          if (money_display2 > 10)
              money_display2 = 10;

          // ================= PLAYER 1 MONEY BAR =================

          strcat(money_bar1, GOLD);

          for (int i = 0; i < money_display1; i++)
              strcat(money_bar1, "▰");

          for (int i = money_display1; i < 10; i++)
              strcat(money_bar1, "▱");

          strcat(money_bar1, RESET);

          // ================= PLAYER 2 MONEY BAR =================

          strcat(money_bar2, GOLD);

          for (int i = 0; i < money_display2; i++)
              strcat(money_bar2, "▰");

          for (int i = money_display2; i < 10; i++)
              strcat(money_bar2, "▱");

          strcat(money_bar2, RESET);
          
          // ================= PLAYER 1 HP BAR =================

          const char *p1_color;

          if (p1.hp >= 7)
              p1_color = GREEN;
          else if (p1.hp >= 4)
              p1_color = YELLOW;
          else
              p1_color = RED;

          strcat(hp_bar1, p1_color);

          for (int i = 0; i < p1.hp; i++)
              strcat(hp_bar1, "▰");

          for (int i = p1.hp; i < 10; i++)
              strcat(hp_bar1, "▱");

          strcat(hp_bar1, RESET);


          // ================= PLAYER 2 HP BAR =================

          const char *p2_color;

          if (p2.hp >= 7)
              p2_color = GREEN;
          else if (p2.hp >= 4)
              p2_color = YELLOW;
          else
              p2_color = RED;

          strcat(hp_bar2, p2_color);

          for (int i = 0; i < p2.hp; i++)
              strcat(hp_bar2, "▰");

          for (int i = p2.hp; i < 10; i++)
              strcat(hp_bar2, "▱");

          strcat(hp_bar2, RESET);
          
          // ================= PLAYER 1 SPY BAR =================

          strcat(spy_bar1, CYAN);

          for (int i = 0; i < p1.spy_count; i++)
              strcat(spy_bar1, "◉");

          for (int i = p1.spy_count; i < 2; i++)
              strcat(spy_bar1, "○");

          strcat(spy_bar1, RESET);


          // ================= PLAYER 2 SPY BAR =================

          strcat(spy_bar2, CYAN);

          for (int i = 0; i < p2.spy_count; i++)
              strcat(spy_bar2, "◉");

          for (int i = p2.spy_count; i < 2; i++)
              strcat(spy_bar2, "○");

          strcat(spy_bar2, RESET);
          
          sprintf(buffer,

          "\n"

          CYAN "╔════════════════════════════════════════════════════════════╗\n" RESET
          CYAN "║" RESET
          WHITE "                  RISK OR RUIN BATTLE                    "
          CYAN "   ║\n" RESET

          CYAN "╠════════════════════════════════════════════════════════════╣\n" RESET

          CYAN "║" RESET
          YELLOW "            MATCH %-2d              ROUND %-2d             "
          CYAN "     ║\n" RESET

          CYAN "╚════════════════════════════════════════════════════════════╝\n" RESET

          "\n"

          /* ================= PLAYER 1 ================= */

          GREEN "┌──────────────────── PLAYER 1 ──────────────────────────────┐\n" RESET

          GREEN "│" RESET
          WHITE " %-15s                       SCORE : %-2d           "
          GREEN "│\n" RESET

          GREEN "├────────────────────────────────────────────────────────────┤\n" RESET

          GREEN "│" RESET
          RED " HP    : " RESET
          "%-40s %2d/10                  "
          GREEN "                │\n" RESET

          GREEN "│" RESET
          YELLOW " MONEY : " RESET
          "%-28s %-2d$   "
          CYAN "SPY : " RESET
          "%-10s %-2d "
          GREEN "                      │\n" RESET
                                       
          GREEN "│" RESET
          MAGENTA " LOAN  : " RESET
          "%-48s "
          GREEN "  │\n" RESET

          GREEN "└────────────────────────────────────────────────────────────┘\n" RESET

          "\n"

          /* ================= PLAYER 2 ================= */

          BLUE "┌──────────────────── PLAYER 2 ──────────────────────────────┐\n" RESET

          BLUE "│" RESET
          WHITE " %-15s                       SCORE : %-2d           "
          BLUE "│\n" RESET

          BLUE "├────────────────────────────────────────────────────────────┤\n" RESET

          BLUE "│" RESET
          RED " HP    : " RESET
          "%-40s %2d/10                  "
          BLUE "                │\n" RESET

          BLUE "│" RESET
          YELLOW " MONEY : " RESET
          "%-28s %-2d$   "
          CYAN "SPY : " RESET
          "%-10s %-2d "
          BLUE "                      │\n" RESET
                                
          BLUE "│" RESET
          MAGENTA " LOAN  : " RESET
          "%-48s "
          BLUE "  │\n" RESET

          BLUE "└────────────────────────────────────────────────────────────┘\n" RESET

          "\n"

          /* ================= COMMANDS ================= */

          GOLD "┌──────────────────── COMMANDS ──────────────────────────────┐\n" RESET

          GOLD "│" RESET
          WHITE " WORK(+$2)  ATTACK(-$2/2DMG)  DEFEND(-$1/50%%)      "
          GOLD "         │\n" RESET

          GOLD "│" RESET
          WHITE " LOAN(+$5)  SPY(REVEAL)       ALLIN(HALF MONEY DMG) "
          GOLD "        │\n" RESET

          GOLD "└────────────────────────────────────────────────────────────┘\n" RESET

          "\n"

          CYAN "══════════════════════════════════════════════════════════════\n" RESET,

          match,
          round,

          p1.name,
          p1_score,
          hp_bar1,
          p1.hp,
          money_bar1,
          p1.money,
          spy_bar1,
          p1.spy_count,
          loan1,

          p2.name,
          p2_score,
          hp_bar2,
          p2.hp,
          money_bar2,
          p2.money,
          spy_bar2,
          p2.spy_count,
          loan2
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

        if (p2.hp <= 2 && p1.money >= 4) {
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

        if (p1.hp <= 2 && p2.money >= 4) {
            send_msg(p2_sock, "[TIP] You can finish with ALL-IN!\n");
        }

        if (p1.money >= 7) {
            send_msg(p2_sock, "[TIP] Opponent is rich. Consider DEFEND or SPY. Play Smart!\n");
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
                for (int i = 0; p1_action[i]; i++) {
                    p1_action[i] = tolower(p1_action[i]);
                }

                if (strcmp(p1_action, "spy") == 0 && p1.spy_count <= 0) {

                    send_msg(p1_sock, "[ERROR] Insufficient spy charges!\n");
                    send_msg(p1_sock, "Choose action: ");

                }
                else if (!is_valid_action(p1_action, &p1)) {

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
                for (int i = 0; p2_action[i]; i++) {
                    p2_action[i] = tolower(p2_action[i]);
                }

                if (strcmp(p2_action, "spy") == 0 && p2.spy_count <= 0) {

                    send_msg(p2_sock, "[ERROR] Insufficient spy charges!\n");
                    send_msg(p2_sock, "Choose action: ");

                }
                else if (!is_valid_action(p2_action, &p2)) {

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

            strcat(event1, "[SPY] Both players used spy. No information gained. Both lose 1 spy!\n");
            strcat(event2, "[SPY] Both players used spy. No information gained. Both lose 1 spy!\n");
        }
        else if (p1_spy && !p2_spy) {
            p1.spy_count--;
            p1_spy_count++;
            
            snprintf(buffer, MAX, "[SPY] Opponent chose: %.50s\n", p2_action);
            send_msg(p1_sock, buffer);

            do {
                send_msg(p1_sock, "Choose action: ");
                recv_msg(p1_sock, p1_action);

                if (!is_valid_action(p1_action, &p1)) {

                    if (strcmp(p1_action, "spy") == 0 && p1.spy_count <= 0) {
                        send_msg(p1_sock, "[ERROR] Insufficient spy charges!\n");
                    }
                    else {
                        send_msg(p1_sock, "[ERROR] Invalid action.\n");
                    }
                }

            } while (!is_valid_action(p1_action, &p1));
        }
        else if (p2_spy && !p1_spy) {
            p2.spy_count--;
            p2_spy_count++;
            
            snprintf(buffer, MAX, "[SPY] Opponent chose: %.50s\n", p1_action);
            send_msg(p2_sock, buffer);

            do {
                send_msg(p2_sock, "Choose action: ");
                recv_msg(p2_sock, p2_action);

                if (!is_valid_action(p2_action, &p2)) {

                    if (strcmp(p2_action, "spy") == 0 && p2.spy_count <= 0) {
                        send_msg(p2_sock, "[ERROR] Insufficient spy charges!\n");
                    }
                    else {
                        send_msg(p2_sock, "[ERROR] Invalid action.\n");
                    }
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

            strcat(event1,
            "[INFO] Loan taken: +$5. Pay $7 in 3 turns.\n");

            strcat(event1,
            "[DEBT] You are now in debt!\n");
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

            strcat(event2,
            "[INFO] Loan taken: +$5. Pay $7 in 3 turns.\n");

            strcat(event2,
            "[DEBT] You are now in debt!\n");
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
        
        // ===== LOAN COUNTDOWN (FAIR) =====
        if (p1.loan_timer > 0) {
            p1.loan_timer--;
            if (p1.loan_timer == 0) {
                if (p1.money >= 7) {
                    p1.money -= 7;
                    send_msg(p1_sock, "[INFO] Loan paid successfully.\n");
                } else {
                    p1.hp -= 3;
                    
                    d1 += 3;
                    strcpy(penalty1, "[PENALTY] Failed to pay loan. -3 HP\n");
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
                    
                    d2 += 3;
                    strcpy(penalty2, "[PENALTY] Failed to pay loan. -3 HP\n");
                }
                p2.loan_timer = -1;
            }
        }
        
        if (p1.hp < 0) p1.hp = 0;
        if (p2.hp < 0) p2.hp = 0;
        
        p1_total_damage += d2;  
        p2_total_damage += d1;  
        
        send_msg(p1_sock, "Round resolved!\n");
        send_msg(p2_sock, "Round resolved!\n");

        // default = final action
        snprintf(p1_display, sizeof(p1_display), "%.10s", p1_action);
        snprintf(p2_display, sizeof(p2_display), "%.10s", p2_action);
            
        snprintf(summary, MAX,
        "\n"

        CYAN "╔════════════════════════════════════════════════════════════╗\n" RESET

        CYAN "║" RESET
        YELLOW "                    ROUND %-2d SUMMARY                    "
        CYAN "    ║\n" RESET

        CYAN "╠════════════════════════════════════════════════════════════╣\n" RESET

        GREEN "║" RESET
        WHITE " %-12s " RESET
        CYAN "│" RESET
        WHITE " ACTION : %-10s " RESET
        CYAN "│" RESET
        RED " DAMAGE : -%-2d ❤ " RESET
        GREEN "       ║\n" RESET

        BLUE "║" RESET
        WHITE " %-12s " RESET
        CYAN "│" RESET
        WHITE " ACTION : %-10s " RESET
        CYAN "│" RESET
        RED " DAMAGE : -%-2d ❤ " RESET
        BLUE "       ║\n" RESET

        CYAN "╚════════════════════════════════════════════════════════════╝\n" RESET,
        round,

        p1.name, p1_display, d1,
        p2.name, p2_display, d2
        );
        
        strcpy(prev_summary1, summary);
        strcpy(prev_summary2, summary);
        
        if (strlen(event1) > 0)
            strcat(prev_summary1, event1);

        if (strlen(event2) > 0)
            strcat(prev_summary2, event2);

        if (strlen(penalty1) > 0)
            strcat(prev_summary1, penalty1);

        if (strlen(penalty2) > 0)
            strcat(prev_summary2, penalty2);
                
        if (p1.hp == 0 || p2.hp == 0) {
            break;
        }
        
        round++; 
    }

    if (p1.hp <= 0 && p2.hp <= 0) {
    
    send_msg(p1_sock, "Draw Round!\n");
    send_msg(p2_sock, "Draw Round!\n");

    }
    else if (p1.hp <= 0) {

        p2_score++;

        snprintf(buffer, MAX,
            "\n%s wins this round!\n",
            p2.name);

        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);

    }
    else {

        p1_score++;

        snprintf(buffer, MAX,
            "\n%s wins this round!\n",
            p1.name);

        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
    }
    
    match++;
    
    if (p1_score < 2 && p2_score < 2) {

        snprintf(buffer, MAX,

        "\n"

        CYAN "════════════════════════════════════════════════════════════\n" RESET

        YELLOW "                     NEXT MATCH STARTING\n" RESET

        CYAN "════════════════════════════════════════════════════════════\n" RESET

        "\n"

        GREEN "             CURRENT SCORE: " RESET
        WHITE "%.8s (%d)" RESET
        CYAN "  -  " RESET
        WHITE "%.8s (%d)\n" RESET

        "\n"

        CYAN "════════════════════════════════════════════════════════════\n" RESET,

        p1.name,
        p1_score,
        p2.name,
        p2_score);
        
        send_msg(p1_sock, buffer);
        send_msg(p2_sock, buffer);
    }
    
    if (p1_score < 2 && p2_score < 2) {

        char ready[10];
        int p1_ready = 0, p2_ready = 0;

        send_msg(p1_sock,
        "\n[READY] Press ENTER when ready for the next match...");
        
        send_msg(p2_sock,
        "\n[READY] Press ENTER when ready for the next match...");

        while (!p1_ready || !p2_ready) {

            fd_set readyfds;
            FD_ZERO(&readyfds);

            if (!p1_ready) FD_SET(p1_sock, &readyfds);
            if (!p2_ready) FD_SET(p2_sock, &readyfds);

            int maxfd = (p1_sock > p2_sock ? p1_sock : p2_sock) + 1;

            select(maxfd, &readyfds, NULL, NULL, NULL);

            if (!p1_ready && FD_ISSET(p1_sock, &readyfds)) {

                recv_msg(p1_sock, ready);
                p1_ready = 1;

                if (!p2_ready) {

                    send_msg(p1_sock,
                    "[WAITING] Waiting for other player...\n");

                    send_msg(p2_sock,
                    "\n[INFO] Opponent is ready.\n");
                }
            }

            if (!p2_ready && FD_ISSET(p2_sock, &readyfds)) {

                recv_msg(p2_sock, ready);
                p2_ready = 1;

                if (!p1_ready) {

                    send_msg(p2_sock,
                    "[WAITING] Waiting for other player...\n");

                    send_msg(p1_sock,
                    "\n[INFO] Opponent is ready.\n");
                }
            }
        }

        send_msg(p1_sock,
        "\n[START] Both players are ready!\n");

        send_msg(p2_sock,
        "\n[START] Both players are ready!\n");
        
        send_msg(p1_sock, "\033[2J\033[H");
        send_msg(p2_sock, "\033[2J\033[H");
    }
    
    }
    
    // ===== FINAL MATCH RESULT =====

    if (p1_score == 2) {

        // ================= WINNER =================

        snprintf(buffer, MAX,
        
        "\n"
        GOLD
        "                    ___________\n"
        "                   '._==_==_=_.'\n"
        "                   .-\\:      /-.\n"
        "  CONGRATULATIONS | (|:.     |) | YOU ARE THE WINNER!\n"
        "                   '-|:.     |-'\n"
        "                     \\::.    /\n"
        "                      '::. .'\n"
        "                        ) (\n"
        "                      _.' '._\n"
        "                     `\"\"\"\"\"\"\"`\n"
        RESET

        "\n"
        GOLD
        "╔════════════════════════════════════════════════════╗\n"
        "║                    TOTAL VICTORY                   ║\n"
        "╠════════════════════════════════════════════════════╣\n"
        "║                                                    ║\n"
        "║                  ENEMY ELIMINATED                  ║\n"
        "║                                                    ║\n"
        "║              No comeback was possible.             ║\n"
        "║                                                    ║\n"
        "║             Every round tilted your way.           ║\n"
        "║                                                    ║\n"
        "║                THE MATCH IS YOURS!!!               ║\n"
        "║                                                    ║\n"
        "╚════════════════════════════════════════════════════╝\n"
        RESET
        );

        send_msg(p1_sock, buffer);


        // ================= LOSER =================

        snprintf(buffer, MAX,

        "\n"
        RED
        "╔════════════════════════════════════════════════════╗\n"
        "║                      DEFEAT                        ║\n"
        "╠════════════════════════════════════════════════════╣\n"
        "║                                                    ║\n"
        "║                 YOUR HP HIT ZERO                   ║\n"
        "║                                                    ║\n"
        "║             The opponent broke through.            ║\n"
        "║                                                    ║\n"
        "║               Regroup and try again.               ║\n"
        "║                                                    ║\n"
        "║                  MATCH TERMINATED                  ║\n"
        "║                                                    ║\n"
        "╚════════════════════════════════════════════════════╝\n"
        RESET
        );

        send_msg(p2_sock, buffer);

    }
    else {

        // ================= WINNER =================

        snprintf(buffer, MAX,
        
        "\n"
        GOLD
        "                    ___________\n"
        "                   '._==_==_=_.'\n"
        "                   .-\\:      /-.\n"
        "  CONGRATULATIONS | (|:.     |) | YOU ARE THE WINNER!\n"
        "                   '-|:.     |-'\n"
        "                     \\::.    /\n"
        "                      '::. .'\n"
        "                        ) (\n"
        "                      _.' '._\n"
        "                     `\"\"\"\"\"\"\"`\n"
        RESET

        "\n"
        GOLD
        "╔════════════════════════════════════════════════════╗\n"
        "║                    TOTAL VICTORY                   ║\n"
        "╠════════════════════════════════════════════════════╣\n"
        "║                                                    ║\n"
        "║                  ENEMY ELIMINATED                  ║\n"
        "║                                                    ║\n"
        "║              No comeback was possible.             ║\n"
        "║                                                    ║\n"
        "║             Every round tilted your way.           ║\n"
        "║                                                    ║\n"
        "║                THE MATCH IS YOURS!!!               ║\n"
        "║                                                    ║\n"
        "╚════════════════════════════════════════════════════╝\n"
        RESET
        );

        send_msg(p2_sock, buffer);


        // ================= LOSER =================

        snprintf(buffer, MAX,

        "\n"
        RED
        "╔════════════════════════════════════════════════════╗\n"
        "║                      DEFEAT                        ║\n"
        "╠════════════════════════════════════════════════════╣\n"
        "║                                                    ║\n"
        "║                 YOUR HP HIT ZERO                   ║\n"
        "║                                                    ║\n"
        "║             The opponent broke through.            ║\n"
        "║                                                    ║\n"
        "║               Regroup and try again.               ║\n"
        "║                                                    ║\n"
        "║                  MATCH TERMINATED                  ║\n"
        "║                                                    ║\n"
        "╚════════════════════════════════════════════════════╝\n"
        RESET
        );

        send_msg(p1_sock, buffer);
    }
    
    char final[MAX];

    snprintf(final, MAX,

    "\n"
    "╔══════════════════════════════════════════════════════════════════════════╗\n"
    "║                           WHOLE MATCH STATS                              ║\n"
    "╠══════════════╦═══════╦════════╦═════╦═════╦══════╦═════╦══════╦══════════╣\n"
    "║ PLAYER       ║ SCORE ║ DAMAGE ║ ATK ║ DEF ║ WORK ║ SPY ║ LOAN ║ ALL-IN   ║\n"
    "╠══════════════╬═══════╬════════╬═════╬═════╬══════╬═════╬══════╬══════════╣\n"

    "║ %-12.12s ║ %-5d ║ %-6d ║ %-3d ║ %-3d ║ %-4d ║ %-3d ║ %-4d ║ %-8d ║\n"

    "║ %-12.12s ║ %-5d ║ %-6d ║ %-3d ║ %-3d ║ %-4d ║ %-3d ║ %-4d ║ %-8d ║\n"

    "╚══════════════╩═══════╩════════╩═════╩═════╩══════╩═════╩══════╩══════════╝\n",

    p1.name,
    p1_score,
    p1_total_damage,
    p1_attack,
    p1_defend,
    p1_work,
    p1_spy_count,
    p1_loan,
    p1_allin,

    p2.name,
    p2_score,
    p2_total_damage,
    p2_attack,
    p2_defend,
    p2_work,
    p2_spy_count,
    p2_loan,
    p2_allin
    );

    send_msg(p1_sock, final);
    send_msg(p2_sock, final);
    
    char p1_rematch[10];
    char p2_rematch[10];

    int p1_done = 0;
    int p2_done = 0;

    send_msg(p1_sock,
    "\n[REMATCH] Play again? (yes/no): ");

    send_msg(p2_sock,
    "\n[REMATCH] Play again? (yes/no): ");

    while (!p1_done || !p2_done) {

        fd_set rematchfds;
        FD_ZERO(&rematchfds);

        if (!p1_done) FD_SET(p1_sock, &rematchfds);
        if (!p2_done) FD_SET(p2_sock, &rematchfds);

        int maxfd = (p1_sock > p2_sock ? p1_sock : p2_sock) + 1;

        select(maxfd, &rematchfds, NULL, NULL, NULL);

        // PLAYER 1
        if (!p1_done && FD_ISSET(p1_sock, &rematchfds)) {

            recv_msg(p1_sock, p1_rematch);

            if (strcmp(p1_rematch, "yes") != 0 &&
                strcmp(p1_rematch, "no") != 0) {

                send_msg(p1_sock,
                "[ERROR] Type only yes or no: ");
            }
            else {

                p1_done = 1;

                if (!p2_done) {

                    send_msg(p1_sock,
                    "[WAITING] Waiting for other player...\n");

                    send_msg(p2_sock,
                    "[INFO] Opponent answered.\n");
                }
            }
        }

        // PLAYER 2
        if (!p2_done && FD_ISSET(p2_sock, &rematchfds)) {

            recv_msg(p2_sock, p2_rematch);

            if (strcmp(p2_rematch, "yes") != 0 &&
                strcmp(p2_rematch, "no") != 0) {

                send_msg(p2_sock,
                "[ERROR] Type only yes or no: ");
            }
            else {

                p2_done = 1;

                if (!p1_done) {

                    send_msg(p2_sock,
                    "[WAITING] Waiting for other player...\n");

                    send_msg(p1_sock,
                    "\n[INFO] Opponent answered.\n");
                }
            }
        }
    }

        if (strcmp(p1_rematch, "yes") == 0 &&
            strcmp(p2_rematch, "yes") == 0) {

            send_msg(p1_sock, "\033[2J\033[H");
            send_msg(p2_sock, "\033[2J\033[H");

            send_msg(p1_sock,
            "\n[REMATCH] Starting new game...\n");

            send_msg(p2_sock,
            "\n[REMATCH] Starting new game...\n");
            
            continue;

        }
        else {

            send_msg(p1_sock,
            "\n[GAME] Match ended.\n");

            send_msg(p2_sock,
            "\n[GAME] Match ended.\n");
            
            send_msg(p1_sock,

            "\n"
            "╔════════════════════════════════════════════════════╗\n"
            "║                THANK YOU FOR PLAYING               ║\n"
            "╠════════════════════════════════════════════════════╣\n"
            "║                                                    ║\n"
            "║          Every battle sharpens a player.           ║\n"
            "║                                                    ║\n"
            "║           Wins build confidence. Losses            ║\n"
            "║                 build experience.                  ║\n"
            "║                                                    ║\n"
            "║              Keep improving. Keep                  ║\n"
            "║                    fighting.                       ║\n"
            "║                                                    ║\n"
            "║                See you next match.                 ║\n"
            "║                                                    ║\n"
            "╚════════════════════════════════════════════════════╝\n");


            send_msg(p2_sock,

            "\n"
            "╔════════════════════════════════════════════════════╗\n"
            "║                THANK YOU FOR PLAYING               ║\n"
            "╠════════════════════════════════════════════════════╣\n"
            "║                                                    ║\n"
            "║          Every battle sharpens a player.           ║\n"
            "║                                                    ║\n"
            "║           Wins build confidence. Losses            ║\n"
            "║                 build experience.                  ║\n"
            "║                                                    ║\n"
            "║              Keep improving. Keep                  ║\n"
            "║                    fighting.                       ║\n"
            "║                                                    ║\n"
            "║                See you next match.                 ║\n"
            "║                                                    ║\n"
            "╚════════════════════════════════════════════════════╝\n");
            
            break;
        }
    
    }
    
    close(p1_sock);
    close(p2_sock);
    close(server_sock);

    return 0;
}
