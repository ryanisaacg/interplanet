#include "game.h"

#include <stdio.h>

typedef struct {
	AU_Circle size;
	float gravity;
	AU_Sprite sprite;
} Planet;

typedef struct {
	AU_Circle size;
	AU_Vector speed, accel;
	bool slamming;
	int health, iframes;
} Player;

typedef struct {
	AU_Circle size;
	AU_AnimatedSprite sprite;
} Enemy;

static AU_AnimationManager load_player_textures(AU_Engine* eng);
static AU_AnimationManager load_enemy_textures(AU_Engine* eng, AU_Texture tex);
static AU_Sound jump_sound, hurt_sound, hit_sound, land_sound;
static int player_jump, player_pound, player_stand, player_walk;

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

	
static void player_update(Player* player, Planet* planets, size_t length, bool left, bool right, bool up, bool down, AU_AnimatedSprite* sprite) {
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
	supported = au_geom_vec_len2(pointer(player, closest)) - 10 <= rad * rad;
	if(supported && player->slamming) {
		player->slamming = false;
		au_sound_play(land_sound, 1);
	}
	const float walk_speed = 3;
	AU_Vector unit_left = { -unit_pointer.y, unit_pointer.x };
	player->speed = au_geom_vec_sub(player->speed, au_geom_vec_scl(unit_left, au_geom_vec_dot(player->speed, unit_left)));
	if(left) {
		AU_Vector left = au_geom_vec_scl(unit_left, walk_speed);
		player->speed = au_geom_vec_add(player->speed, left);
		sprite->transform.flip_x = false;
	} 
	if(right) { 
		AU_Vector right = au_geom_vec_scl(unit_left, -walk_speed);
		player->speed = au_geom_vec_add(player->speed, right);
		sprite->transform.flip_x = true;
	}
	if(up && supported) {
		player->speed = au_geom_vec_add(player->speed, au_geom_vec_scl(unit_pointer, -8));
		au_sound_play(jump_sound, 1);
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
	if(supported) {
		if(left || right) {
			au_anim_manager_switch(&sprite->animations, player_walk);
		} else {
			au_anim_manager_switch(&sprite->animations, player_stand);
		}
	} else {
		if(player->slamming) {
			au_anim_manager_switch(&sprite->animations, player_pound);
		} else {
			au_anim_manager_switch(&sprite->animations, player_jump);
		}
	}
	sprite->transform.rotation = atan2(unit_pointer.y, unit_pointer.x) * 180 / M_PI + 90;
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
			au_sound_play(hit_sound, 1);
			return true;
		} else {
			if(player->iframes <= 0) {
				player->health--;
				player->iframes = 60;
				au_sound_play(hurt_sound, 1);
			}
		}
	}
	enemy->sprite.transform.rotation = atan2(unit_pointer.y, unit_pointer.x) * 180 / M_PI + 90;
	enemy->sprite.transform.x = enemy->size.x - enemy->sprite.transform.width / 2;
	enemy->sprite.transform.y = enemy->size.y - enemy->sprite.transform.height / 2;
	enemy->sprite.transform.origin_x = enemy->sprite.transform.x + enemy->sprite.transform.width / 2;
	enemy->sprite.transform.origin_y = enemy->sprite.transform.y + enemy->sprite.transform.height / 2;
	enemy->sprite.transform.flip_x = true;
	return false;
}

void game_loop(AU_Engine* eng) {
	//Load game assets
	AU_AnimatedSprite player_sprite = au_sprite_anim_new(load_player_textures(eng));
	AU_Texture texture = au_load_texture(eng, "assets/assets/enemy.png"); 
	jump_sound = au_sound_load("assets/assets/jump.wav");
	hurt_sound = au_sound_load("assets/assets/hurt.wav");
	land_sound = au_sound_load("assets/assets/land.wav");
	hit_sound = au_sound_load("assets/assets/kill.wav");
	au_sound_play(au_sound_load("assets/assets/background.wav"), 0);
	AU_Texture planet_texture = au_load_texture(eng, "assets/assets/planet0.png");
	//Create game entities
	Player player = { { 400, 400, 16 }, { 0, 0 }, { 0, 0 }, false, 100, 0};
	const size_t num_planets = 500;
	Planet* planets = malloc(sizeof(Planet) * num_planets);
	AU_Sprite first = au_sprite_new(au_tex_region(planet_texture));
	first.transform.x = 100 - first.transform.width / 2;
	first.transform.y = 400 - first.transform.height / 2;
	first.transform.origin_x = 100;
	first.transform.origin_y = 400;
	first.transform.rotation = au_util_randf_range(0, 360);
	planets[0] = (Planet) { { 100, 400, 128 }, 0.25f, first };
	float depth = 1;
	for(size_t i = 1; i < num_planets; i++) {
		planets[i].size.x = au_util_randf_range(-5000, 5000);
		planets[i].size.y = au_util_randf_range(-5000, 5000);
		planets[i].size.radius = 128;
		planets[i].gravity = 0.25f;
		AU_Sprite sprite = au_sprite_new(au_tex_region(planet_texture));
		sprite.transform.x = planets[i].size.x - sprite.transform.width / 2;
		sprite.transform.y = planets[i].size.y - sprite.transform.height / 2;
		sprite.transform.origin_x = planets[i].size.x;
		sprite.transform.origin_y = planets[i].size.y;
		sprite.transform.rotation = au_util_randf_range(0, 360);
		sprite.transform.depth = depth;
		depth -= 1.0f / num_planets;
		planets[i].sprite = sprite;
	}
	size_t num_enemies = 500;
	size_t enemy_capacity = 500;
	Enemy* enemies = malloc(sizeof(Enemy) * num_enemies);
	enemies[0] = (Enemy) { { 300, 400, 16 }, au_sprite_anim_new(load_enemy_textures(eng, texture)) };
	for(size_t i = 1; i < num_enemies; i++) {
		enemies[i].size.x = au_util_randf_range(-5000, 5000);
		enemies[i].size.y = au_util_randf_range(-5000, 5000);
		enemies[i].size.radius = 16;
		enemies[i].sprite = au_sprite_anim_new(load_enemy_textures(eng, texture));
	}
	//Main loop
	while(eng->should_continue) {
		au_begin(eng, AU_BLACK);
		player_update(&player, planets, num_planets, eng->current_keys[SDL_SCANCODE_A], eng->current_keys[SDL_SCANCODE_D], 
				eng->current_keys[SDL_SCANCODE_W] && !eng->previous_keys[SDL_SCANCODE_W], 
				eng->current_keys[SDL_SCANCODE_S] && !eng->previous_keys[SDL_SCANCODE_S],
				&player_sprite);
		eng->camera.x = player.size.x - eng->camera.width / 2;
		eng->camera.y = player.size.y - eng->camera.height / 2;
		AU_Color col = AU_WHITE;
		if(player.iframes > 0) col.g = col.b = 0;
		player_sprite.transform.x = player.size.x - player_sprite.transform.width / 2;
		player_sprite.transform.y = player.size.y - player_sprite.transform.height / 2;
		player_sprite.transform.origin_x = player_sprite.transform.x + player_sprite.transform.width / 2;
		player_sprite.transform.origin_y = player_sprite.transform.y + player_sprite.transform.height / 2;
		player_sprite.transform.color = col;
		au_draw_sprite_animated(eng, &player_sprite);
		for(size_t i = 0; i < num_planets; i++) {
			au_draw_sprite(eng, &(planets[i].sprite));
		}
		for(size_t i = 0; i < num_enemies; i++) {
			bool killed = enemy_update(enemies + i, planets, num_planets, &player);
			if(killed) {
				num_enemies--;
				enemies[i] = enemies[num_enemies];
				i--;
				continue;
			}
			au_draw_sprite_animated(eng, &(enemies[i].sprite));
		}
		au_draw_rect_depth(eng, AU_RED, (AU_Rectangle) { player.size.x - 16, player.size.y - 20, player.health / 4, 4 }, 0);
		printf("%s\n", Mix_GetError());
		au_end(eng);
	}
}

static AU_AnimationManager load_player_textures(AU_Engine* eng) {
	AU_Texture jump = au_load_texture(eng, "assets/assets/jump.png");
	AU_Texture pound = au_load_texture(eng, "assets/assets/pound.png");
	AU_Texture stand = au_load_texture(eng, "assets/assets/stand_cycle.png");
	AU_Texture walk = au_load_texture(eng, "assets/assets/walk_cycle.png");
	AU_Animation jump_anim = au_anim_new(au_tex_region(jump), 1);
	AU_Animation pound_anim = au_anim_new(au_tex_region(pound), 1);
	AU_TextureRegion stand_region = au_tex_region(stand);
	AU_TextureRegion walk_region = au_tex_region(walk);
	AU_TextureRegion stand_frames[4];
	for(size_t i = 0; i < 4; i++) {
		AU_TextureRegion frame = stand_region;
		frame.rect = (AU_Rectangle) { i * 32, 0, 32, 32 };
		stand_frames[i] = frame;
	}
	AU_Animation stand_anim = au_anim_from_array(stand_frames, 4, 10);
	AU_TextureRegion walk_frames[4];
	for(size_t i = 0; i < 4; i++) {
		AU_TextureRegion frame = walk_region;
		frame.rect = (AU_Rectangle) { i * 32, 0, 32, 32};
		walk_frames[i] = frame;
	}
	AU_Animation walk_anim = au_anim_from_array(walk_frames, 4, 10);
	AU_AnimationManager manager = au_anim_manager_new();
	player_jump = au_anim_manager_register(&manager, &jump_anim);
	player_pound = au_anim_manager_register(&manager, &pound_anim);
	player_walk = au_anim_manager_register(&manager, &walk_anim);
	player_stand = au_anim_manager_register(&manager, &stand_anim);
	au_anim_manager_switch(&manager, player_jump);
	return manager;
}

static AU_AnimationManager load_enemy_textures(AU_Engine* eng, AU_Texture tex) {
	AU_TextureRegion region = au_tex_region(tex);
	AU_TextureRegion frames[4];
	for(size_t i = 0; i < 4; i++) {
		AU_TextureRegion frame = region;
		frame.rect = (AU_Rectangle) { i * 32, 0, 32, 32};
		frames[i] = frame;
	}
	AU_AnimationManager manager = au_anim_manager_new();
	AU_Animation anim = au_anim_from_array(frames, 4, 10);	
	au_anim_manager_switch(&manager, au_anim_manager_register(&manager, &anim));
	return manager;
}
