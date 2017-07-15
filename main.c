#include "au.h"

#include "game.h"

int main() {
    AU_Engine* eng = au_init("interplanet", 800, 600, NULL, DEFAULT_CONFIG);
	game_loop(eng);
    au_quit(eng);
    return 0;
}
