#include "game.h"

typedef struct {
	AU_Circle size;
	float gravity;
} Planet;

typedef struct {
	AU_Circle size;
	AU_Vector speed, accel;
} Player;

static void gravity(Player* player, Planet* planet) {
	AU_Vector player_center = (AU_Vector) { player->size.x, player->size.y };
	AU_Vector planet_center = (AU_Vector) { planet->size.x, planet->size.y };
	AU_Vector pointer = au_geom_vec_sub(planet_center, player_center);
	AU_Vector gravity = au_geom_vec_set_len(pointer, planet->gravity);
	player->accel = gravity;
}

void game_loop(AU_Engine* eng) {
	Player player = { { 400, 400, 32 }, { 0, 0 }, { 0, 0 } };
	Planet planet = { { 100, 400, 128 }, 0.05f };
	while(eng->should_continue) {
		au_begin(eng, AU_WHITE);
		gravity(&player, &planet);
		player.speed = au_geom_vec_add(player.speed, player.accel);
		player.size.x += player.speed.x;
		player.size.y += player.speed.y;
		if(au_geom_circ_overlaps_circ(player.size, planet.size)) {
			player.size.x -= player.speed.x;
			player.size.y -= player.speed.y;
		}
		au_draw_circle(eng, AU_RED, player.size);
		au_draw_circle(eng, AU_BLUE, planet.size);
		au_end(eng);
	}
}
