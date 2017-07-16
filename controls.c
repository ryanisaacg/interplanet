#include "controls.h"
#include "game.h"

void controls_loop(AU_Engine* eng) {
	AU_Texture controls = au_load_texture(eng, "assets/assets/controls.png");
	AU_Sprite spr = au_sprite_new(au_tex_region(controls));
	spr.transform.flip_y = true;
	const int keys[] = { SDL_SCANCODE_SPACE, SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_RETURN };
	while(eng->should_continue) {
		au_begin(eng, AU_BLACK);
		au_draw_sprite(eng, &spr);
		for(int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
			if(eng->current_keys[i]) {
				game_loop(eng);
				break;
			}
		}
		au_end(eng);
	}
}
		
