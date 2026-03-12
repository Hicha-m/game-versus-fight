#include "constants.h"
#include "game.h"
#include "engine.h"
#include "engine/render.h"
#include "engine/input.h"
#include <SDL3/SDL.h>

/**
 * Fixed timestep game loop with accumulator pattern
 * 
 * Architecture patterns from game-architecture.md:
 * - Fixed logic timestep: 16.67ms (60 FPS)
 * - Rendering: Decoupled, interpolated between frames
 * - Determinism: Frame-perfect timing for combat
 * 
 * Accumulator pattern:
 *   accumulator += frame_delta
 *   while (accumulator >= FIXED_DT)
 *       logic_update(FIXED_DT)
 *       accumulator -= FIXED_DT
 *   render_frame(accumulator / FIXED_DT)  // interpolation factor
 */
int main(void) {
	GameConfig config = {0};
	GameState state = {0};
	
	// Initialize engine (SDL3, window, renderer)
	GameError error = engine_init();
	if (error != GAME_OK) {
		SDL_Log("FATAL: engine_init failed: %s", game_error_string(error));
		return 1;
	}

	// Initialize game state
	error = game_create(&config, &state); 
	if (error != GAME_OK) {
		SDL_Log("FATAL: game_create failed: %s", game_error_string(error));
		engine_shutdown();
		return 1;
	}
	
	// Fixed timestep setup
	const uint32_t FIXED_DT_MS = FIXED_TIMESTEP_MS;  // 16 ms = 60 FPS
	const float FIXED_DT_SEC = (float)FIXED_DT_MS / 1000.0f;
	
	uint64_t accumulator = 0;   // Accumulates frame delta time
	uint64_t last_tick = SDL_GetTicks();
	
	// Main game loop
	while (state.running) {
		// Calculate frame delta time
		uint64_t current_tick = SDL_GetTicks();
		uint64_t frame_delta = current_tick - last_tick;
		last_tick = current_tick;
		
		// Cap frame delta to prevent spiral of death (lag handling)
		// If lag spike > 100ms, clamp to max update (allows recovery)
		if (frame_delta > 100) {
			frame_delta = 100;
		}
		
		accumulator += frame_delta;
		
		// LOGIC: Fixed timestep updates
		// This ensures deterministic physics, collision, and AI evaluation
		while (accumulator >= FIXED_DT_MS) {
			// Collect input for this fixed frame
			FrameInput input = {0};
			engine_collect_input(&input);

			// Update game logic at fixed timestep
			error = game_update(&state, &input);
			if (error != GAME_OK) {
				SDL_Log("ERROR: game_update failed: %s", game_error_string(error));
				state.running = false;
				break;
			}

			state.frame_index++;
			accumulator -= FIXED_DT_MS;
		}
		
		// RENDER: Interpolated frame with remainder time
		// Interpolation factor: how far into the next frame we are
		float interpolation_factor = (float)accumulator / (float)FIXED_DT_MS;
		
		error = render_frame(&state, interpolation_factor);
		if (error != GAME_OK) {
			SDL_Log("ERROR: render_frame failed: %s", game_error_string(error));
			state.running = false;
			break;
		}
	}

	// Cleanup
	engine_shutdown();
	game_destroy(&state);
	
	return error == GAME_OK ? 0 : 1;
}
