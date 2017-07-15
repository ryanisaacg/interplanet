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
	
static void player_update(Player* player, Planet* planets, size_t length, bool left, bool right, bool up, bool down) {
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
	float rad = player->size.radius + closest->size.radius;
	player->accel = gravity;
	player->speed = au_geom_vec_add(player->speed, player->accel);
	bool supported = false;
	if(au_geom_vec_len2(pointer(player, closest)) <= rad * rad) {
		player->speed = au_geom_vec_sub(player->speed, au_geom_vec_scl(unit_pointer, au_geom_vec_dot(player->speed, unit_pointer)));
		supported = true;
		float difference = rad - au_geom_vec_len(pointer(player, closest));
		AU_Vector embed = au_geom_vec_scl(unit_pointer, -difference);
		player->size.x += embed.x;
		player->size.y += embed.y;
	}
	const float walk_speed = 3;
	AU_Vector unit_left = { -unit_pointer.y, unit_pointer.x };
	player->speed = au_geom_vec_sub(player->speed, au_geom_vec_scl(unit_left, au_geom_vec_dot(player->speed, unit_left)));
	if(left) {
		AU_Vector left = au_geom_vec_scl(unit_left, walk_speed);
		player->speed = au_geom_vec_add(player->speed, left);
	} 
	if(right) { 
		AU_Vector right = au_geom_vec_scl(unit_left, -walk_speed);
		player->speed = au_geom_vec_add(player->speed, right);
	}
	if(up && supported) {
		player->speed = au_geom_vec_add(player->speed, au_geom_vec_scl(unit_pointer, -8));
	}
	if(down && !supported) {
		player->speed = au_geom_vec_add(player->speed, au_geom_vec_scl(unit_pointer, 8));
	}
	player->size.x += player->speed.x;
	player->size.y += player->speed.y;
}

void game_loop(AU_Engine* eng) {
	Player player = { { 400, 400, 32 }, { 0, 0 }, { 0, 0 } };
	Planet planets[] = {
		{ { 100, 400, 128 }, 0.25f },
		{ { 400, 100, 128 }, 0.25f },
		{ { 600, 400, 128}, 0.25f },
	};
	size_t num_planets = sizeof(planets) / sizeof(planets[0]);
	while(eng->should_continue) {
		au_begin(eng, AU_WHITE);
		player_update(&player, planets, num_planets, eng->current_keys[SDL_SCANCODE_A], eng->current_keys[SDL_SCANCODE_D], 
				eng->current_keys[SDL_SCANCODE_W] && !eng->previous_keys[SDL_SCANCODE_W], 
				eng->current_keys[SDL_SCANCODE_S] && !eng->previous_keys[SDL_SCANCODE_S]);
		eng->camera.x = player.size.x - eng->camera.width / 2;
		eng->camera.y = player.size.y - eng->camera.height / 2;
		au_draw_circle(eng, AU_RED, player.size);
		for(size_t i = 0; i < num_planets; i++) {
			au_draw_circle(eng, AU_BLUE, planets[i].size);
		}
		au_end(eng);
	}
}
