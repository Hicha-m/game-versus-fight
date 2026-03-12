#include "utils.h"
#include <math.h>

/**
 * Clamp float value within [min_value, max_value]
 */
float math_clampf(float value, float min_value, float max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

/**
 * Clamp integer value within [min_value, max_value]
 */
int32_t math_clampi(int32_t value, int32_t min_value, int32_t max_value) {
	if (value < min_value) return min_value;
	if (value > max_value) return max_value;
	return value;
}

/**
 * Linear interpolation between two Vec2f points
 * Pattern: Used for rendering interpolation in fixed timestep architecture
 */
Vec2f vec2f_lerp(Vec2f a, Vec2f b, float t) {
	t = math_clampf(t, 0.0f, 1.0f);
	return (Vec2f){
		.x = a.x + (b.x - a.x) * t,
		.y = a.y + (b.y - a.y) * t
	};
}

/**
 * Vector addition
 */
Vec2f vec2f_add(Vec2f a, Vec2f b) {
	return (Vec2f){.x = a.x + b.x, .y = a.y + b.y};
}

/**
 * Vector subtraction
 */
Vec2f vec2f_sub(Vec2f a, Vec2f b) {
	return (Vec2f){.x = a.x - b.x, .y = a.y - b.y};
}

/**
 * Scalar multiplication
 */
Vec2f vec2f_mul(Vec2f v, float scalar) {
	return (Vec2f){.x = v.x * scalar, .y = v.y * scalar};
}

/**
 * Euclidean distance between two points
 */
float vec2f_distance(Vec2f a, Vec2f b) {
	Vec2f diff = vec2f_sub(b, a);
	return vec2f_length(diff);
}

/**
 * Vector magnitude
 */
float vec2f_length(Vec2f v) {
	return sqrtf(v.x * v.x + v.y * v.y);
}

/**
 * Simple Linear Congruential Generator (LCG) for reproducible RNG
 * Pattern: Deterministic seeding for procedural arena generation
 * Used for: Map generation with seed-based replay
 */
static uint32_t rng_state = 1;

void math_seed_rng(uint32_t seed) {
	rng_state = seed != 0 ? seed : 1;
}

uint32_t math_random_u32(void) {
	// MINSTD Lehmer RNG: x = (a*x) mod m
	// Parameters chosen for good distribution and reproducibility
	rng_state = (rng_state * 1103515245u + 12345u);
	return rng_state;
}
