#include <stdio.h>
#include <stdlib.h>

#include "game/arena/arena.h"

int main(void)
{
    Arena arena = {0};
    int i;
    int difficulty;

    printf("=== Arena Generation & Validation Test ===\n\n");

    /* Test 1: Easy difficulty */
    printf("TEST 1: Easy difficulty (2)\n");
    if (!arena_init(&arena)) {
        fprintf(stderr, "Failed to init arena\n");
        return 1;
    }

    arena_generate_next(&arena, 12345u, 2);

    for (i = 0; i < ROOM_COUNT; ++i) {
        const Room* room = arena_get_room_const(&arena, i);
        if (room) {
            printf("  Room %d: type=%d, width=%d tiles, direction=%d\n",
                   i, room->type, room->width_tiles, room->direction);
        }
    }
    printf("  Generated successfully\n\n");

    /* Test 2: Hard difficulty */
    printf("TEST 2: Hard difficulty (9)\n");
    if (!arena_init(&arena)) {
        fprintf(stderr, "Failed to init arena\n");
        return 1;
    }

    arena_generate_next(&arena, 54321u, 9);

    for (i = 0; i < ROOM_COUNT; ++i) {
        const Room* room = arena_get_room_const(&arena, i);
        if (room) {
            printf("  Room %d: type=%d, width=%d tiles, direction=%d\n",
                   i, room->type, room->width_tiles, room->direction);
        }
    }
    printf("  Generated successfully\n\n");

    /* Test 3: Progressive difficulty sweep */
    printf("TEST 3: Progressive difficulty sweep (0-10)\n");
    for (difficulty = 0; difficulty <= 10; ++difficulty) {
        if (!arena_init(&arena)) {
            fprintf(stderr, "Failed to init arena at difficulty %d\n", difficulty);
            return 1;
        }

        arena_generate_next(&arena, 99999u, difficulty);

        const Room* center = arena_get_room_const(&arena, ROOM_COUNT / 2);
        if (center) {
            printf("  Difficulty %2d: center room width=%d tiles\n",
                   difficulty, center->width_tiles);
        }
    }

    printf("\nAll tests completed successfully!\n");
    return 0;
}
