#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NEIGHBORS    4
#define BIT_ROUND_BITS   16
#define MAX_NAME_LEN     16

// Forward declarations
struct Room;
struct Player;
struct Monster;
struct Item;

typedef enum { NONE, MONSTER, ITEM, TREASURE } ContentType;
typedef enum { GOBLIN, TROLL, DRAGON } MonsterType;
typedef enum { POTION, SWORD, ELIXIR } ItemType;
typedef void (*EnterFunc)(struct Room*, struct Player*);

typedef struct RoomListNode {
    struct Room* room;
    struct RoomListNode* next;
} RoomListNode;

typedef struct Monster {
    MonsterType type;
    char name[MAX_NAME_LEN];
    int hp;
    int damage;
} Monster;

typedef struct Item {
    ItemType type;
    char name[MAX_NAME_LEN];
    int hp_restore;
    int damage_boost;
} Item;

typedef struct Room {
    int id;
    RoomListNode* neighbors;
    ContentType content_type;
    void* content;
    int visited;
    EnterFunc enter;
} Room;

typedef struct Player {
    Room* location;
    int hp;
    int damage;
} Player;

// Function prototypes
Room** create_dungeon(int n);
void populate_rooms(Room** rooms, int n);
RoomListNode* add_neighbor(RoomListNode* head, Room* room);
void enter_empty(Room* room, Player* player);
void enter_monster(Room* room, Player* player);
void enter_item(Room* room, Player* player);
void enter_treasure(Room* room, Player* player);
void fight(Player* player, Monster* mon);
int save_game(const char* filename, Player* player, Room** rooms, int n);
Room** load_game(const char* filename, Player* player, int* out_n);
Room* find_neighbor(Room* current, int id);
void cleanup(Player* player, Room** rooms, int n);

int main(int argc, char* argv[]) {
    srand((unsigned)time(NULL));
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num_rooms> OR %s <savefile>\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    Room** rooms;
    int n;
    Player player;

    char* endptr;
    long num = strtol(argv[1], &endptr, 10);
    if (*endptr == '\0' && num > 1) {
        n = (int)num;
        rooms = create_dungeon(n);
        populate_rooms(rooms, n);
        player.hp = 20;
        player.damage = 5;
        player.location = rooms[0];
    } else {
        rooms = load_game(argv[1], &player, &n);
        if (!rooms) {
            fprintf(stderr, "Error: could not load game from '%s'\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    printf("=== Dungeon Crawler ===\n");
    while (1) {
        Room* cur = player.location;
        printf("\nYou are in room %d %s\n", cur->id,
               cur->visited ? "(visited)" : "");
        if (cur->content_type == TREASURE) {
            enter_treasure(cur, &player);
            break;
        }
        cur->enter(cur, &player);
        printf("Doors to rooms:");
        for (RoomListNode* it = cur->neighbors; it; it = it->next)
            printf(" %d", it->room->id);
        printf("\nChoose a door (-1: save & quit): ");

        int choice;
        if (scanf("%d", &choice) != 1) break;
        if (choice == -1) {
            if (save_game("savegame.dat", &player, rooms, n) == 0)
                printf("Game saved. Goodbye!\n");
            else
                fprintf(stderr, "Error saving game\n");
            break;
        }
        Room* dest = find_neighbor(cur, choice);
        if (dest) player.location = dest;
        else printf("Invalid choice, try again.\n");
    }

    cleanup(&player, rooms, n);
    return EXIT_SUCCESS;
}

Room** create_dungeon(int n) {
    Room** rooms = malloc(n * sizeof(Room*));
    if (!rooms) { perror("malloc"); exit(EXIT_FAILURE); }
    for (int i = 0; i < n; ++i) {
        rooms[i] = calloc(1, sizeof(Room));
        if (!rooms[i]) { perror("calloc"); exit(EXIT_FAILURE); }
        rooms[i]->id = i;
        rooms[i]->neighbors = NULL;
        rooms[i]->content_type = NONE;
        rooms[i]->enter = enter_empty;
    }
    // Connect as a tree
    for (int i = 1; i < n; ++i) {
        int j = rand() % i;
        rooms[i]->neighbors = add_neighbor(rooms[i]->neighbors, rooms[j]);
        rooms[j]->neighbors = add_neighbor(rooms[j]->neighbors, rooms[i]);
    }
    // Add random extra edges
    for (int i = 0; i < n; ++i) {
        int deg = 0;
        for (RoomListNode* it = rooms[i]->neighbors; it; it = it->next) deg++;
        int extras = rand() % (MAX_NEIGHBORS - deg + 1);
        for (int e = 0; e < extras; ++e) {
            int j = rand() % n;
            if (j == i) continue;
            // avoid duplicate
            RoomListNode* it2;
            for (it2 = rooms[i]->neighbors; it2; it2 = it2->next)
                if (it2->room == rooms[j]) break;
            if (!it2) {
                rooms[i]->neighbors = add_neighbor(rooms[i]->neighbors, rooms[j]);
                rooms[j]->neighbors = add_neighbor(rooms[j]->neighbors, rooms[i]);
            }
        }
    }
    return rooms;
}

void populate_rooms(Room** rooms, int n) {
    int treasure_room = 1 + rand() % (n - 1);
    rooms[treasure_room]->content_type = TREASURE;
    rooms[treasure_room]->enter = enter_treasure;
    for (int i = 1; i < n; ++i) {
        if (i == treasure_room) continue;
        int r = rand() % 100;
        if (r < 3) {
            Monster* m = malloc(sizeof(Monster));
            m->type = DRAGON; strcpy(m->name, "Dragon"); m->hp = 24; m->damage = 6;
            rooms[i]->content_type = MONSTER; rooms[i]->content = m; rooms[i]->enter = enter_monster;
        } else if (r < 60) {
            Monster* m = malloc(sizeof(Monster));
            if (rand() % 2) { m->type = GOBLIN; strcpy(m->name, "Goblin"); m->hp = 8; m->damage = 5; }
            else { m->type = TROLL; strcpy(m->name, "Troll"); m->hp = 12; m->damage = 3; }
            rooms[i]->content_type = MONSTER; rooms[i]->content = m; rooms[i]->enter = enter_monster;
        } else if (r < 80) {
            Item* it = malloc(sizeof(Item));
            it->type = POTION; strcpy(it->name, "Potion"); it->hp_restore = 10; it->damage_boost = 0;
            rooms[i]->content_type = ITEM; rooms[i]->content = it; rooms[i]->enter = enter_item;
        } else if (r < 95) {
            Item* it = malloc(sizeof(Item));
            it->type = SWORD; strcpy(it->name, "Sword"); it->hp_restore = 0; it->damage_boost = 2;
            rooms[i]->content_type = ITEM; rooms[i]->content = it; rooms[i]->enter = enter_item;
        } else {
            Item* it = malloc(sizeof(Item));
            it->type = ELIXIR; strcpy(it->name, "Elixir"); it->hp_restore = 0; it->damage_boost = 0;
            rooms[i]->content_type = ITEM; rooms[i]->content = it; rooms[i]->enter = enter_item;
        }
        rooms[i]->visited = 0;
    }
}

RoomListNode* add_neighbor(RoomListNode* head, Room* room) {
    RoomListNode* node = malloc(sizeof(RoomListNode));
    node->room = room; node->next = head; return node;
}

void enter_empty(Room* room, Player* player) {
    printf("The room is empty.\n"); room->visited = 1;
}

void enter_monster(Room* room, Player* player) {
    if (room->visited) { enter_empty(room, player); return; }
    Monster* m = room->content;
    printf("You encounter a %s! (hp:%d, dmg:%d)\n", m->name, m->hp, m->damage);
    fight(player, m);
    if (m->hp <= 0) { printf("%s defeated!\n", m->name); free(m); room->content_type = NONE; room->enter = enter_empty; }
    room->visited = 1;
}

void enter_item(Room* room, Player* player) {
    if (room->visited) { enter_empty(room, player); return; }
    Item* it = room->content;
    printf("You find a %s! ", it->name);
    switch (it->type) {
        case POTION: player->hp += it->hp_restore; printf("Restored %d hp (now %d)\n", it->hp_restore, player->hp); break;
        case SWORD: player->damage += it->damage_boost; printf("Damage +%d (now %d)\n", it->damage_boost, player->damage); break;
        case ELIXIR: player->hp*=2; player->damage*=2; printf("HP and damage doubled! (hp=%d, dmg=%d)\n", player->hp, player->damage); break;
        default: break;
    }
    free(it); room->content_type = NONE; room->enter = enter_empty; room->visited = 1;
}

void enter_treasure(Room* room, Player* player) {
    (void)room; (void)player; printf("You found the treasure! You win! \n");
}

void fight(Player* player, Monster* mon) {
    while (player->hp>0 && mon->hp>0) {
        unsigned int rnd = rand() % (1U<<BIT_ROUND_BITS);
        printf("Attack order bits: ");
        for (int i=BIT_ROUND_BITS-1;i>=0;--i) putchar(((rnd>>i)&1)?'1':'0'); putchar('\n');
        for (int i=0;i<BIT_ROUND_BITS && player->hp>0 && mon->hp>0;++i) {
            if (((rnd>>i)&1)==0) { player->hp-=mon->damage; printf("Monster attacks for %d (hp=%d)\n",mon->damage,player->hp); }
            else { mon->hp-=player->damage; printf("You attack for %d (hp=%d)\n",player->damage,mon->hp); }
        }
    }
    if (player->hp<=0) { printf("You died... Game Over.\n"); exit(EXIT_FAILURE); }
}

int save_game(const char* filename, Player* player, Room** rooms, int n) {
    FILE* f=fopen(filename,"wb"); if(!f)return-1;
    fwrite(&n,sizeof(int),1,f);
    int cid=player->location->id; fwrite(&cid,sizeof(int),1,f);
    fwrite(&player->hp,sizeof(int),1,f); fwrite(&player->damage,sizeof(int),1,f);
    for(int i=0;i<n;++i){Room*r=rooms[i]; fwrite(&r->visited,sizeof(int),1,f); fwrite(&r->content_type,sizeof(ContentType),1,f);
        if(r->content_type==MONSTER){Monster*m=r->content; fwrite(&m->type,sizeof(MonsterType),1,f); fwrite(&m->hp,sizeof(int),1,f); fwrite(&m->damage,sizeof(int),1,f);} 
        else if(r->content_type==ITEM){Item*it=r->content; fwrite(&it->type,sizeof(ItemType),1,f);} 
        int deg=0; for(RoomListNode*it=r->neighbors;it;it=it->next)deg++;
        fwrite(&deg,sizeof(int),1,f);
        for(RoomListNode*it=r->neighbors;it;it=it->next){int id=it->room->id; fwrite(&id,sizeof(int),1,f);} }
    fclose(f); return 0;
}

Room** load_game(const char* filename, Player* player, int* out_n) {
    FILE*f=fopen(filename,"rb"); if(!f)return NULL;
    int n; if(fread(&n,sizeof(int),1,f)!=1){fclose(f);return NULL;} *out_n=n;
    Room**rooms=calloc(n,sizeof(Room*)); for(int i=0;i<n;++i){rooms[i]=calloc(1,sizeof(Room)); rooms[i]->id=i; rooms[i]->enter=enter_empty; rooms[i]->neighbors=NULL;}
    int cid,hp,dm; fread(&cid,sizeof(int),1,f); fread(&hp,sizeof(int),1,f); fread(&dm,sizeof(int),1,f); player->hp=hp; player->damage=dm; player->location=rooms[cid];
    for(int i=0;i<n;++i){int vis;ContentType ct; fread(&vis,sizeof(int),1,f); fread(&ct,sizeof(ContentType),1,f); rooms[i]->visited=vis; rooms[i]->content_type=ct;
        if(ct==MONSTER){Monster*m=malloc(sizeof(Monster));MonsterType mt;int mh,md; fread(&mt,sizeof(MonsterType),1,f); fread(&mh,sizeof(int),1,f); fread(&md,sizeof(int),1,f); m->type=mt;m->hp=mh;m->damage=md; strcpy(m->name,(mt==GOBLIN?"Goblin":mt==TROLL?"Troll":"Dragon")); rooms[i]->content=m; rooms[i]->enter=enter_monster;}
        else if(ct==ITEM){Item*it=malloc(sizeof(Item));ItemType itp; fread(&itp,sizeof(ItemType),1,f); it->type=itp; if(itp==POTION)strcpy(it->name,"Potion"),it->hp_restore=10,it->damage_boost=0; else if(itp==SWORD)strcpy(it->name,"Sword"),it->hp_restore=0,it->damage_boost=2; else strcpy(it->name,"Elixir"); rooms[i]->content=it; rooms[i]->enter=enter_item;}
        int deg; fread(&deg,sizeof(int),1,f); for(int j=0;j<deg;++j){int id; fread(&id,sizeof(int),1,f); rooms[i]->neighbors=add_neighbor(rooms[i]->neighbors,rooms[id]);}
    }
    fclose(f); return rooms;
}

Room* find_neighbor(Room* current, int id) {
    for(RoomListNode* it=current->neighbors;it;it=it->next) if(it->room->id==id) return it->room; return NULL;
}

void cleanup(Player* player, Room** rooms, int n) {
    (void)player; if(!rooms) return;
    for(int i=0;i<n;++i){Room*r=rooms[i]; if(r->content) free(r->content); RoomListNode*it=r->neighbors; while(it){RoomListNode*nx=it->next; free(it); it=nx;} free(r);} free(rooms);
}
