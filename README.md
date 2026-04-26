**RISK RUIN GAME**

A turn-based multiplayer strategy game built in C using socket programming.

**Description**

This game allows two players to compete by managing money, health, and strategy.
Players can attack, defend, take loans, and use spy mechanics to outplay each other.

The game emphasizes:

* Risk vs reward decisions
* Resource management
* Prediction and mind games

**Features**

* Real-time multiplayer (server-client)
* Loan system with pressure mechanics
* Spy system (reveals opponent move)
* Turn-based combat system

**How to Run**

**Compile**

gcc server.c -o server
gcc client.c -o client

**Run**

Terminal 1:
./server 5001

Terminal 2(PLAYER 1):
./client localhost 5001

Terminal 2(PLAYER 2):
./client localhost 5001

**Requirements**

* Linux / Ubuntu terminal
* GCC compiler

**Author**

JOHN MATHEW C. HEMADY

LOUIS MARTIN L. SAMBO

SHANDY MARK R. HOSMILLO
