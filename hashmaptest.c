#include <stdio.h>
#include <stdlib.h>
#include "hashmap.h"

int main() {
    struct HashMap* map = HashMap_init();

    HashMap_put(map, "Chris", "CoolDude");
    HashMap_put(map, "Layla", "K00l Kat");
    HashMap_put(map, "Celia", "Cool lady");
    HashMap_put(map, "Hudson M0", "Yeeetman");
    HashMap_put(map, "Chris", "Danowtch");
    HashMap_put(map, "Layla", "My baby");

    HashMap_print(map);

    printf("%s\n", (char*)HashMap_get(map, "Chris"));

    HashMap_free(map);
}
