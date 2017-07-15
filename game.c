#include "game.h"

typedef struct {
	AU_Circle size;
	float gravity;
} Planet;

typedef struct {
	AU_Circle size;
	AU_Vector speed, accel;
	bool slamming;
	int health, iframes;
} Player;

typedef struct {
	AU_Circle size;
} Enemy;

static AU_Vector pointer(Player* player, Planet* planet) {
	AU_Vector player_center = (AU_Vector) { player->size.x, player->size.y };
	AU_Vector planet_center = (AU_Vector) { planet->size.x, planet->size.y };
	return au_geom_vec_sub(planet_center, player_center);
}

static Planet* closest_planet(Player* player, Planet* planets, size_t length) {
	Planet* closest = planets;
	float closest_len = au_geom_vec_len(pointer(player, planets));
	for(size_t i = 1; i < length; i++) {
		float len = au_geom_vec_len(pointer(player, planets + i));
		if(len < closest_len) {
			closest_len = len;
			closest = planets + i;
		}
	}
	return closest;
}

	
static void player_update(Player* player, Planet* planets, size_t length, bool left, bool right, bool up, bool down) {
	Planet* closest = closest_planet(player, planets, length);
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
	supported = au_geom_vec_len2(pointer(player, closest)) - 5 <= rad * rad;
	player->slamming = player->slamming && !supported;
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
	if(down && !supported && !player->slamming) {
		player->speed = au_geom_vec_add(player->speed, au_geom_vec_scl(unit_pointer, 8));
		player->slamming = true;
	}
	player->size.x += player->speed.x;
	player->size.y += player->speed.y;
	if(player->iframes > 0) {
		player->iframes--;
	}
}

static bool enemy_update(Enemy* enemy, Planet* planets, size_t length, Player* player) {
	Planet* closest = closest_planet((Player*)enemy, planets, length);
	AU_Vector unit_pointer = au_geom_vec_nor(pointer((Player*)enemy, closest));
	float rad = enemy->size.radius + closest->size.radius;
	float len_squared = au_geom_vec_len2(pointer((Player*)enemy, closest));
	if(len_squared <= rad * rad) {
		float difference = rad + 32 - sqrt(len_squared);
		AU_Vector embed = au_geom_vec_scl(unit_pointer, -difference);
		enemy->size.x += embed.x;
		enemy->size.y += embed.y;
	}
	AU_Vector unit_left = { -unit_pointer.y, unit_pointer.x };
	AU_Vector right = au_geom_vec_scl(unit_left, -1);
	enemy->size.x += right.x;
	enemy->size.y += right.y;
	if(au_geom_circ_overlaps_circ(player->size, enemy->size)) {
		if(player->slamming) {
			return true;
		} else {
			if(player->iframes <= 0) {
				player->health--;
				player->iframes = 60;
			}
		}
	}
	return false;
}

void game_loop(AU_Engine* eng) {
	Player player = { { 400, 400, 32 }, { 0, 0 }, { 0, 0 }, false, 100 };
	const size_t num_planets = 500;
	Planet* planets = malloc(sizeof(Planet) * num_planets);
	planets[0] = (Planet) { { 100, 400, 128 }, 0.25f };
	for(size_t i = 1; i < num_planets; i++) {
		planets[i].size.x = au_util_randf_range(-5000, 5000);
		planets[i].size.y = au_util_randf_range(-5000, 5000);
		planets[i].size.radius = 128;
		planets[i].gravity = 0.25f;
	}
	size_t num_enemies = 500;
	size_t enemy_capacity = 500;
	Enemy* enemies = malloc(sizeof(Enemy) * num_enemies);
	enemies[0] = (Enemy) { { 300, 400, 16 } };
	for(size_t i = 1; i < num_enemies; i++) {
		enemies[i].size.x = au_util_randf_range(-5000, 5000);
		enemies[i].size.y = au_util_randf_range(-5000, 5000);
		enemies[i].size.radius = 16;
	}
	while(eng->should_continue) {
		au_begin(eng, AU_WHITE);
		player_update(&player, planets, num_planets, eng->current_keys[SDL_SCANCODE_A], eng->current_keys[SDL_SCANCODE_D], 
				eng->current_keys[SDL_SCANCODE_W] && !eng->previous_keys[SDL_SCANCODE_W], 
				eng->current_keys[SDL_SCANCODE_S] && !eng->previous_keys[SDL_SCANCODE_S]);
		eng->camera.x = player.size.x - eng->camera.width / 2;
		eng->camera.y = player.size.y - eng->camera.height / 2;
		AU_Color col = AU_BLUE;
		if(player.iframes > 0) col.r = 0.5f;
		au_draw_circle(eng, col, player.size);
		for(size_t i = 0; i < num_planets; i++) {
			au_draw_circle(eng, AU_GREEN, planets[i].size);
		}
		for(size_t i = 0; i < num_enemies; i++) {
			bool killed = enemy_update(enemies + i, planets, num_planets, &player);
			if(killed) {
				num_enemies--;
				enemies[i] = enemies[num_enemies];
				i--;
				continue;
			}
			au_draw_circle(eng, AU_RED, enemies[i].size);
		}
		au_end(eng);
	}
}
