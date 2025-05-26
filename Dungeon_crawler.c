typedef struct Room {
    int id;
    int num_neighbors;
    struct Room** neighbors; // adjacency list
} Room;

// Allocatie op heap
Room* create_room(int id) {
    Room* room = malloc(sizeof(Room));
    room->id = id;
    room->num_neighbors = 0;
    room->neighbors = malloc(sizeof(Room*) * 4); // max 4 buren
    return room;
}

void connect_rooms(Room* a, Room* b) {
    if (a->num_neighbors < 4 && b->num_neighbors < 4) {
        a->neighbors[a->num_neighbors++] = b;
        b->neighbors[b->num_neighbors++] = a;
    }
}

typedef struct Monster {
    int hp;
    int damage;
} Monster;

typedef struct Room {
    int id;
    int numConnections;
    struct Room** connections;
    Monster* monster;  // NULL als er geen monster is
} Room;

typedef struct Player {
    int currentRoomId;
    int hp;
    int damage;
} Player;
void fight(Player* player, Room* room) {
    if (room->monster == NULL) {
        printf("Er is geen monster in deze kamer.\n");
        return;
    }

    printf("Gevecht gestart!\n");

    // Speler en monster vallen elkaar aan
    room->monster->hp -= player->damage;
    player->hp -= room->monster->damage;

    printf("Speler HP: %d\n", player->hp);
    printf("Monster HP: %d\n", room->monster->hp);

    // Check of iemand dood is
    if (room->monster->hp <= 0) {
        printf("Monster is verslagen!\n");
        free(room->monster);
        room->monster = NULL;
    }

    if (player->hp <= 0) {
        printf("Speler is verslagen!\n");
        // Hier zou je kunnen afsluiten of andere game over logica doen
    }
}

