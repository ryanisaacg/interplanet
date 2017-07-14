#include "au.h"
int main() {
    AU_Engine* eng = au_init("interplanet", 800, 600, NULL, DEFAULT_CONFIG);
    while(eng->should_continue) {
        au_begin(eng, AU_BLACK);
        au_end(eng);
    }
    au_quit(eng);
    return 0;
}