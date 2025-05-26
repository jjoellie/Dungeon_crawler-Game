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
 
