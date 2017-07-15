#include "game.h"

typedef struct {
	AU_Circle size;
	float gravity;
} Planet;

typedef struct {
	AU_Circle size;
	AU_Vector speed, accel;
} Player;

static AU_Vector pointer(Player* player, Planet* planet) {
	AU_Vector player_center = (AU_Vector) { player->size.x, player->size.y };
	AU_Vector planet_center = (AU_Vector) { planet->size.x, planet->size.y };
	return au_geom_vec_sub(planet_center, player_center);
}
	
static void player_update(Player* player, Planet* planets, size_t length, bool left, bool right) {
	Planet* closest = planets;
	float closest_len = au_geom_vec_len(pointer(player, planets));
	for(size_t i = 1; i < length; i++) {
		float len = au_geom_vec_len(pointer(player, planets + i));
		if(len < closest_len) {
			closest_len = len;
			closest = planets + i;
		}
	}
	AU_Vector unit_pointer = au_geom_vec_nor(pointer(player, closest));
	AU_Vector gravity = au_geom_vec_scl(unit_pointer, closest->gravity);
	player->accel = gravity;
	player->speed = au_geom_vec_add(player->speed, player->accel);
	const float walk_speed = 0.25;
	AU_Vector unit_left = { unit_pointer.y, -unit_pointer.x };
	if(left) {
		AU_Vector left = au_geom_vec_scl(unit_left, walk_speed);
		player->speed = au_geom_vec_add(player->speed, left);
	} 
	if(right) { 
		AU_Vector right = au_geom_vec_scl(unit_left, -walk_speed);
		player->speed = au_geom_vec_add(player->speed, right);
	}
	player->size.x += player->speed.x;
	player->size.y += player->speed.y;
	if(au_geom_circ_overlaps_circ(player->size, closest->size)) {
		player->size.x -= player->speed.x;
		player->size.y -= player->speed.y;
	}
}

void game_loop(AU_Engine* eng) {
	Player player = { { 400, 400, 32 }, { 0, 0 }, { 0, 0 } };
	Planet planet = { { 100, 400, 128 }, 0.05f };
	while(eng->should_continue) {
		au_begin(eng, AU_WHITE);
		player_update(&player, &planet, 1, eng->current_keys[SDL_SCANCODE_A], eng->current_keys[SDL_SCANCODE_D]);
		au_draw_circle(eng, AU_RED, player.size);
		au_draw_circle(eng, AU_BLUE, planet.size);
		au_end(eng);
	}
}
