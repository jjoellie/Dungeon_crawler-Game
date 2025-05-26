#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int hp;
    int damage;
} Player;

typedef struct {
    int hp;
    int damage;
} Monster;

void fight(Player *player, Monster *monster) {
    while (player->hp > 0 && monster->hp > 0) {
        int random = rand() % 17; // 0–16
        for (int i = 0; i < 5 && player->hp > 0 && monster->hp > 0; i++) {
            int bit = (random >> i) & 1;
            if (bit == 0) {
                // Monster valt speler aan
                player->hp -= monster->damage;
                printf("Monster valt aan! Speler HP: %d\n", player->hp);
            } else {
                // Speler valt monster aan
                monster->hp -= player->damage;
                printf("Speler valt aan! Monster HP: %d\n", monster->hp);
            }
        }
    }

    if (player->hp <= 0) {
        printf("Speler is verslagen.\n");
    } else if (monster->hp <= 0) {
        printf("Monster is verslagen.\n");
    }
}
