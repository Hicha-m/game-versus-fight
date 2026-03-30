#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game/ai/ai.h"
#include "ai_internal.h"
#include "utils/log.h"

static bool ai_profile_logging_enabled(void)
{
    static i32 cached = -1;
    const char* env;

    if (cached >= 0) {
        return cached == 1;
    }

    env = getenv("AI_PROFILE");
    if (!env) {
        cached = 1;
        return true;
    }

    if (
        strcmp(env, "0") == 0 ||
        strcmp(env, "false") == 0 ||
        strcmp(env, "FALSE") == 0 ||
        strcmp(env, "off") == 0 ||
        strcmp(env, "OFF") == 0
    ) {
        cached = 0;
        return false;
    }

    cached = 1;
    return true;
}

static u64 ai_estimate_full_tree_nodes(i32 depth, bool* out_overflow)
{
    const u64 max_u64 = ~(u64)0;
    u64 total = 0;
    u64 level_nodes = 1;
    i32 d;

    if (out_overflow) {
        *out_overflow = false;
    }

    if (depth < 0) {
        return 0;
    }

    for (d = 0; d <= depth; ++d) {
        if (total > max_u64 - level_nodes) {
            if (out_overflow) {
                *out_overflow = true;
            }
            return max_u64;
        }
        total += level_nodes;

        if (d < depth) {
            if (level_nodes > max_u64 / 8ULL) {
                if (out_overflow) {
                    *out_overflow = true;
                }
                return max_u64;
            }
            level_nodes *= 8ULL;
        }
    }

    return total;
}

static void ai_log_profile_sample(
    const AIController* ai,
    i32 self_index,
    i32 depth,
    i32 best_action_index,
    f32 dt
)
{
    static f32 log_accumulator_s[MAX_PLAYERS] = {0.0f, 0.0f};
    static u64 sample_counter[MAX_PLAYERS] = {0ULL, 0ULL};
    bool overflow = false;
    u64 full_tree_nodes;
    const u64 max_u64 = ~(u64)0;
    f64 prune_pct = 0.0;
    f32 step_s;

    if (!ai || self_index < 0 || self_index >= MAX_PLAYERS) {
        return;
    }

    if (!ai_profile_logging_enabled()) {
        return;
    }

    step_s = (dt > 0.0f) ? dt : (1.0f / 60.0f);
    log_accumulator_s[self_index] += step_s;

    if (log_accumulator_s[self_index] < 1.0f) {
        return;
    }

    log_accumulator_s[self_index] = 0.0f;
    sample_counter[self_index]++;

    full_tree_nodes = ai_estimate_full_tree_nodes(depth, &overflow);

    if (full_tree_nodes > 0 && !overflow) {
        prune_pct = 100.0 * (1.0 - ((f64)ai->metrics.nodes_expanded / (f64)full_tree_nodes));
        if (prune_pct < 0.0) {
            prune_pct = 0.0;
        }
    }

    log_info(
        "AI[P%d] sample=%llu depth=%d complexity=O(8^d) full_tree=%s%llu nodes=%llu leaves=%llu cutoffs=%llu prune=%.1f%% think=%.3fms mem=%.1fKB action_idx=%d",
        self_index + 1,
        (unsigned long long)sample_counter[self_index],
        depth,
        overflow ? ">=" : "",
        (unsigned long long)(overflow ? max_u64 : full_tree_nodes),
        (unsigned long long)ai->metrics.nodes_expanded,
        (unsigned long long)ai->metrics.leaves_evaluated,
        (unsigned long long)ai->metrics.cutoffs,
        prune_pct,
        ai->metrics.think_time_ms,
        (f32)ai->metrics.estimated_memory_bytes / 1024.0f,
        best_action_index
    );
}

static i32 difficulty_to_depth(AIDifficulty difficulty)
{
    switch (difficulty) {
        case AI_DIFFICULTY_EASY: return 2;
        case AI_DIFFICULTY_HARD: return 4;
        case AI_DIFFICULTY_EXPERT: return 5;
        case AI_DIFFICULTY_MEDIUM:
        default: return 3;
    }
}

static AIGameplayMode difficulty_to_gameplay_mode(AIDifficulty difficulty)
{
    switch (difficulty) {
        case AI_DIFFICULTY_EASY: return AI_MODE_DEFENSIVE;
        case AI_DIFFICULTY_MEDIUM: return AI_MODE_BALANCED;
        case AI_DIFFICULTY_HARD: return AI_MODE_TACTICAL;
        case AI_DIFFICULTY_EXPERT:
        default: return AI_MODE_AGGRESSIVE;
    }
}

static f64 now_ms(void)
{
    struct timespec ts;

    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) {
        return 0.0;
    }

    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

void ai_weights_for_mode(AIHeuristicWeights* out_weights, AIGameplayMode mode)
{
    if (!out_weights) {
        return;
    }

    switch (mode) {
        case AI_MODE_DEFENSIVE:

            out_weights->distance_weight = 0.3f;
            out_weights->sword_advantage_weight = 4.0f;
            out_weights->height_advantage_weight = 0.3f;
            out_weights->center_control_weight = 0.2f;
            out_weights->edge_danger_weight = 3.0f;
            out_weights->attack_opportunity_weight = 1.5f;
            out_weights->stun_penalty_weight = 8.0f;
            break;

        case AI_MODE_BALANCED:

            out_weights->distance_weight = 2.0f;
            out_weights->sword_advantage_weight = 10.0f;
            out_weights->height_advantage_weight = 1.2f;
            out_weights->center_control_weight = 1.2f;
            out_weights->edge_danger_weight = 1.5f;
            out_weights->attack_opportunity_weight = 8.0f;
            out_weights->stun_penalty_weight = 4.0f;
            break;

        case AI_MODE_TACTICAL:

            out_weights->distance_weight = 2.5f;
            out_weights->sword_advantage_weight = 18.0f;
            out_weights->height_advantage_weight = 2.0f;
            out_weights->center_control_weight = 2.0f;
            out_weights->edge_danger_weight = 0.8f;
            out_weights->attack_opportunity_weight = 14.0f;
            out_weights->stun_penalty_weight = 2.0f;
            break;

        case AI_MODE_AGGRESSIVE:

            out_weights->distance_weight = 3.0f;
            out_weights->sword_advantage_weight = 25.0f;
            out_weights->height_advantage_weight = 3.0f;
            out_weights->center_control_weight = 1.0f;
            out_weights->edge_danger_weight = 0.2f;
            out_weights->attack_opportunity_weight = 20.0f;
            out_weights->stun_penalty_weight = 0.5f;
            break;

        default:
            ai_default_weights(out_weights);
            break;
    }
}

void ai_default_weights(AIHeuristicWeights* out_weights)
{
    if (!out_weights) {
        return;
    }

    ai_weights_for_mode(out_weights, AI_MODE_BALANCED);
}

void ai_set_difficulty(AIController* ai, AIDifficulty difficulty)
{
    if (!ai) {
        return;
    }
    ai->difficulty = difficulty;
    ai->search_depth = difficulty_to_depth(difficulty);

    ai_set_gameplay_mode(ai, difficulty_to_gameplay_mode(difficulty));
}

void ai_set_algorithm(AIController* ai, AIAlgorithm algorithm)
{
    if (!ai) {
        return;
    }
    ai->algorithm = algorithm;
}

void ai_set_gameplay_mode(AIController* ai, AIGameplayMode mode)
{
    if (!ai) {
        return;
    }
    ai->gameplay_mode = mode;

    ai_weights_for_mode(&ai->weights, mode);
}

void ai_reset_metrics(AIController* ai)
{
    if (!ai) {
        return;
    }
    memset(&ai->metrics, 0, sizeof(ai->metrics));
}

bool ai_init(AIController* ai)
{
    if (!ai) {
        return false;
    }

    memset(ai, 0, sizeof(*ai));
    ai->enabled = true;
    ai->difficulty = AI_DIFFICULTY_MEDIUM;
    ai->algorithm = AI_ALGO_MINIMAX_ALPHA_BETA;
    ai->gameplay_mode = AI_MODE_BALANCED;
    ai->search_depth = difficulty_to_depth(ai->difficulty);
    ai_weights_for_mode(&ai->weights, ai->gameplay_mode);
    return true;
}

void ai_shutdown(AIController* ai)
{
    (void)ai;
}

void ai_build_decision_state(
    AIDecisionState* out_state,
    const Arena* arena,
    const CombatState* combat,
    i32 self_index
)
{
    const Fighter* self;
    const Fighter* enemy;
    const Room* room;
    i32 enemy_index;
    i32 room_width_px;
    i32 max_bucket;

    if (!out_state || !arena || !combat || self_index < 0 || self_index >= MAX_PLAYERS) {
        return;
    }

    memset(out_state, 0, sizeof(*out_state));

    enemy_index = (self_index == 0) ? 1 : 0;

    self = &combat->fighters[self_index];
    enemy = &combat->fighters[enemy_index];
    room = arena_get_current_room_const(arena);

    out_state->current_room = arena->current_room;
    out_state->room_count = arena->room_count;
    out_state->objective_direction = (self_index == 0) ? 1 : -1;

    out_state->self_x_bucket = (i32)(self->state.pos.x / (TILE_SIZE * 2));
    out_state->self_y_bucket = (i32)(self->state.pos.y / (TILE_SIZE * 2));
    out_state->enemy_x_bucket = (i32)(enemy->state.pos.x / (TILE_SIZE * 2));
    out_state->enemy_y_bucket = (i32)(enemy->state.pos.y / (TILE_SIZE * 2));

    out_state->self_grounded = self->state.grounded;
    out_state->enemy_grounded = enemy->state.grounded;

    out_state->self_has_sword = self->state.has_sword;
    out_state->enemy_has_sword = enemy->state.has_sword;

    out_state->self_sword_line = self->state.sword_line;
    out_state->enemy_sword_line = enemy->state.sword_line;

    out_state->self_can_thrust = (self->state.attack_cooldown <= 0.0f);
    out_state->enemy_can_thrust = (enemy->state.attack_cooldown <= 0.0f);

    if (room) {
        room_width_px = room->width_tiles * TILE_SIZE;
        max_bucket = room_width_px / (TILE_SIZE * 2);
        if (max_bucket < 1) {
            max_bucket = 1;
        }

        out_state->room_max_x_bucket = max_bucket;
        out_state->self_near_edge = (self->state.pos.x < 80.0f) || (self->state.pos.x > room_width_px - 80.0f);
        out_state->enemy_near_edge = (enemy->state.pos.x < 80.0f) || (enemy->state.pos.x > room_width_px - 80.0f);
    } else {
        out_state->room_max_x_bucket = 20;
    }
}

f32 ai_evaluate_state(
    const AIDecisionState* state,
    const AIHeuristicWeights* weights
)
{
    i32 dx;
    i32 dy;
    f32 room_progress;
    f32 room_pos_progress;
    f32 enemy_distance_bonus;
    bool enemy_between_and_close;
    bool in_attack_range;
    f32 score = 0.0f;

    if (!state || !weights) {
        return 0.0f;
    }

    dx = state->enemy_x_bucket - state->self_x_bucket;
    dy = state->enemy_y_bucket - state->self_y_bucket;
    room_progress = (f32)(state->current_room - (state->room_count / 2)) * (f32)state->objective_direction;

    if (state->objective_direction > 0) {

        room_pos_progress = (f32)state->self_x_bucket / (f32)state->room_max_x_bucket;
    } else {

        room_pos_progress = (f32)(state->room_max_x_bucket - state->self_x_bucket) / (f32)state->room_max_x_bucket;
    }

    enemy_distance_bonus = (3.0f - fabsf((f32)dx)) * weights->distance_weight * 0.2f;

    enemy_between_and_close = false;
    if (state->objective_direction > 0) {
        enemy_between_and_close = (state->enemy_x_bucket >= state->self_x_bucket) && (abs(dx) <= 2);
    } else {
        enemy_between_and_close = (state->enemy_x_bucket <= state->self_x_bucket) && (abs(dx) <= 2);
    }

    in_attack_range = (fabsf((f32)dx) <= 2.5f) && (fabsf((f32)dy) <= 1.5f);

    score += room_progress * 12.0f;
    score += room_pos_progress * 15.0f;
    score += enemy_distance_bonus;

    score += (state->self_has_sword ? 1.0f : -1.0f) * weights->sword_advantage_weight;

    score += (dy > 0 ? 1.0f : -1.0f) * weights->height_advantage_weight * 0.5f;

    if (state->self_near_edge && !enemy_between_and_close) {

        score -= weights->edge_danger_weight;
    }
    if (state->enemy_near_edge) {
        score += weights->edge_danger_weight * 1.5f;

        if (in_attack_range && state->self_has_sword && state->self_can_thrust) {
            score += weights->attack_opportunity_weight * 3.0f;
        }
    }

    if (state->self_can_thrust && fabsf((f32)dx) <= 2.5f) {
        score += weights->attack_opportunity_weight * (enemy_between_and_close ? 1.5f : 1.0f);

        if (state->self_has_sword && in_attack_range) {
            score += weights->attack_opportunity_weight * 2.0f;
        }
    }
    if (state->enemy_can_thrust && fabsf((f32)dx) <= 2.5f) {
        score -= weights->attack_opportunity_weight * 0.8f;
    }

    if (!state->self_grounded) {
        score -= 0.1f;
    }
    if (!state->enemy_grounded) {
        score += 0.3f;

        if (in_attack_range && state->self_can_thrust) {
            score += weights->attack_opportunity_weight * 0.8f;
        }
    }

    if (state->self_has_sword && !state->enemy_has_sword && in_attack_range) {
        score += weights->sword_advantage_weight * 0.5f;
    }

    score += 1.0f;

    f32 proximity_bonus = (2.5f - fabsf((f32)dx)) * 2.0f;
    if (proximity_bonus > 0.0f) {
        score += proximity_bonus;
    }

    return score;
}

PlayerCommand ai_action_to_command(AIAction action)
{
    PlayerCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.target_sword_line = SWORD_LINE_MID;

    switch (action) {
        case AI_ACTION_ADVANCE:
            cmd.right_pressed = 1;
            break;
        case AI_ACTION_RETREAT:
            cmd.left_pressed = -1;
            break;
        case AI_ACTION_JUMP:
            cmd.jump_pressed = true;
            break;
        case AI_ACTION_THRUST:
            cmd.thrust_pressed = true;
            break;
        case AI_ACTION_LINE_HIGH:
            cmd.target_sword_line = SWORD_LINE_HIGH;
            break;
        case AI_ACTION_LINE_LOW:
            cmd.target_sword_line = SWORD_LINE_LOW;
            break;
        case AI_ACTION_LINE_MID:
            cmd.target_sword_line = SWORD_LINE_MID;
            break;
        case AI_ACTION_THROW:
            cmd.throw_pressed = true;
            break;
    }

    return cmd;
}

static void ai_apply_action_to_state(AIDecisionState* s, AIAction action, bool self_turn)
{
    i32* x_bucket;
    bool* has_sword;
    SwordLine* line;
    bool* can_thrust;
    bool* near_edge;

    if (self_turn) {
        x_bucket = &s->self_x_bucket;
        has_sword = &s->self_has_sword;
        line = &s->self_sword_line;
        can_thrust = &s->self_can_thrust;
        near_edge = &s->self_near_edge;
    } else {
        x_bucket = &s->enemy_x_bucket;
        has_sword = &s->enemy_has_sword;
        line = &s->enemy_sword_line;
        can_thrust = &s->enemy_can_thrust;
        near_edge = &s->enemy_near_edge;
    }

    switch (action) {
        case AI_ACTION_ADVANCE:
            *x_bucket += 1;
            break;
        case AI_ACTION_RETREAT:
            *x_bucket -= 1;
            break;
        case AI_ACTION_JUMP:
            break;
        case AI_ACTION_THRUST:
            if (*can_thrust) {
                *can_thrust = false;
            }
            break;
        case AI_ACTION_LINE_HIGH:
            *line = SWORD_LINE_HIGH;
            break;
        case AI_ACTION_LINE_MID:
            *line = SWORD_LINE_MID;
            break;
        case AI_ACTION_LINE_LOW:
            *line = SWORD_LINE_LOW;
            break;
        case AI_ACTION_THROW:
            *has_sword = false;
            break;
    }

    if (*x_bucket < 0) {
        *x_bucket = 0;
    } else if (*x_bucket > s->room_max_x_bucket) {
        *x_bucket = s->room_max_x_bucket;
    }

    *near_edge = (*x_bucket < 2 || *x_bucket > s->room_max_x_bucket - 2);
}

static f32 alphabeta(
    const AIDecisionState* state,
    i32 depth,
    f32 alpha,
    f32 beta,
    bool maximizing,
    const AIHeuristicWeights* weights,
    AIMetrics* metrics,
    i32 root_depth
)
{
    static const AIAction actions[] = {
        AI_ACTION_ADVANCE,
        AI_ACTION_RETREAT,
        AI_ACTION_JUMP,
        AI_ACTION_THRUST,
        AI_ACTION_LINE_HIGH,
        AI_ACTION_LINE_MID,
        AI_ACTION_LINE_LOW,
        AI_ACTION_THROW
    };

    i32 i;
    f32 best;

    if (metrics) {
        i32 depth_reached = root_depth - depth;
        metrics->nodes_expanded++;
        if (depth_reached > metrics->max_depth_reached) {
            metrics->max_depth_reached = depth_reached;
        }
    }

    if (depth <= 0) {
        if (metrics) {
            metrics->leaves_evaluated++;
        }
        return ai_evaluate_state(state, weights);
    }

    if (maximizing) {
        best = -1000000.0f;
        for (i = 0; i < (i32)(sizeof(actions) / sizeof(actions[0])); ++i) {
            AIDecisionState next = *state;
            f32 score;
            ai_apply_action_to_state(&next, actions[i], true);
            score = alphabeta(&next, depth - 1, alpha, beta, false, weights, metrics, root_depth);
            if (score > best) {
                best = score;
            }
            if (score > alpha) {
                alpha = score;
            }
            if (beta <= alpha) {
                if (metrics) {
                    metrics->cutoffs++;
                }
                break;
            }
        }
        return best;
    } else {
        best = 1000000.0f;
        for (i = 0; i < (i32)(sizeof(actions) / sizeof(actions[0])); ++i) {
            AIDecisionState next = *state;
            f32 score;
            ai_apply_action_to_state(&next, actions[i], false);
            score = alphabeta(&next, depth - 1, alpha, beta, true, weights, metrics, root_depth);
            if (score < best) {
                best = score;
            }
            if (score < beta) {
                beta = score;
            }
            if (beta <= alpha) {
                if (metrics) {
                    metrics->cutoffs++;
                }
                break;
            }
        }
        return best;
    }
}

static void ai_get_action_priority_order(AIAction* out_actions, AIGameplayMode mode)
{
    static const AIAction aggressive_order[] = {
        AI_ACTION_ADVANCE,
        AI_ACTION_THRUST,
        AI_ACTION_LINE_HIGH,
        AI_ACTION_LINE_LOW,
        AI_ACTION_JUMP,
        AI_ACTION_LINE_MID,
        AI_ACTION_THROW,
        AI_ACTION_RETREAT
    };

    static const AIAction tactical_order[] = {
        AI_ACTION_ADVANCE,
        AI_ACTION_THRUST,
        AI_ACTION_JUMP,
        AI_ACTION_LINE_HIGH,
        AI_ACTION_LINE_MID,
        AI_ACTION_LINE_LOW,
        AI_ACTION_RETREAT,
        AI_ACTION_THROW
    };

    static const AIAction balanced_order[] = {
        AI_ACTION_ADVANCE,
        AI_ACTION_THRUST,
        AI_ACTION_JUMP,
        AI_ACTION_LINE_MID,
        AI_ACTION_LINE_HIGH,
        AI_ACTION_LINE_LOW,
        AI_ACTION_RETREAT,
        AI_ACTION_THROW
    };

    static const AIAction defensive_order[] = {
        AI_ACTION_RETREAT,
        AI_ACTION_JUMP,
        AI_ACTION_ADVANCE,
        AI_ACTION_LINE_MID,
        AI_ACTION_LINE_HIGH,
        AI_ACTION_LINE_LOW,
        AI_ACTION_THRUST,
        AI_ACTION_THROW
    };

    if (!out_actions) {
        return;
    }

    switch (mode) {
        case AI_MODE_AGGRESSIVE:
            memcpy(out_actions, aggressive_order, sizeof(aggressive_order));
            break;
        case AI_MODE_TACTICAL:
            memcpy(out_actions, tactical_order, sizeof(tactical_order));
            break;
        case AI_MODE_BALANCED:
            memcpy(out_actions, balanced_order, sizeof(balanced_order));
            break;
        case AI_MODE_DEFENSIVE:
            memcpy(out_actions, defensive_order, sizeof(defensive_order));
            break;
    }
}

PlayerCommand ai_think(
    AIController* ai,
    const Arena* arena,
    const CombatState* combat,
    i32 self_index,
    f32 dt,
    const i32* player_kills
)
{
    AIHeuristicWeights weights;
    AIDecisionState state;
    PlayerCommand best_cmd;
    f32 best_score = -1000000.0f;
    i32 best_action_index = 0;
    i32 depth;
    i32 i;
    f64 start_ms;
    f64 end_ms;

    (void)player_kills;

    if (!arena || !combat || self_index < 0 || self_index >= MAX_PLAYERS) {
        PlayerCommand none;
        memset(&none, 0, sizeof(none));
        none.target_sword_line = SWORD_LINE_MID;
        return none;
    }

    if (ai) {
        weights = ai->weights;
        depth = ai->search_depth;
        if (depth <= 0) {
            depth = difficulty_to_depth(ai->difficulty);
            ai->search_depth = depth;
        }
        ai_reset_metrics(ai);
    } else {
        ai_default_weights(&weights);
        depth = difficulty_to_depth(AI_DIFFICULTY_MEDIUM);
    }

    ai_build_decision_state(&state, arena, combat, self_index);

    start_ms = now_ms();

    AIGameplayMode mode = ai ? ai->gameplay_mode : AI_MODE_BALANCED;
    AIAction prioritized_actions[8];
    ai_get_action_priority_order(prioritized_actions, mode);

    for (i = 0; i < (i32)(sizeof(prioritized_actions) / sizeof(prioritized_actions[0])); ++i) {
        AIDecisionState next = state;
        f32 score;
        ai_apply_action_to_state(&next, prioritized_actions[i], true);
        score = alphabeta(
            &next,
            depth - 1,
            -1000000.0f,
            1000000.0f,
            false,
            &weights,
            ai ? &ai->metrics : NULL,
            depth
        );
        if (score > best_score) {
            best_score = score;
            best_action_index = i;
        }
    }

    end_ms = now_ms();

    if (ai) {
        if (start_ms > 0.0 && end_ms >= start_ms) {
            ai->metrics.think_time_ms = end_ms - start_ms;
        } else {
            ai->metrics.think_time_ms = 0.0;
        }
        ai->metrics.estimated_memory_bytes = ai->metrics.nodes_expanded * (u64)sizeof(AIDecisionState);
        ai_log_profile_sample(ai, self_index, depth, best_action_index, dt);
    }

    best_cmd = ai_action_to_command(prioritized_actions[best_action_index]);

    if (combat) {
        const Fighter* self = &combat->fighters[self_index];
        const Fighter* enemy = &combat->fighters[(self_index == 0) ? 1 : 0];
        f32 dx = enemy->state.pos.x - self->state.pos.x;
        f32 dy = fabsf(enemy->state.pos.y - self->state.pos.y);
        f32 attack_range_x = SWORD_THRUST_RANGE_X * ATTACK_RANGE_ARMED_MULTIPLIER;
        f32 attack_range_y = SWORD_HIT_RANGE_Y * ATTACK_HEIGHT_ARMED_MULTIPLIER;
        bool enemy_is_in_front =
            (self->state.facing_right && dx >= 0.0f) ||
            (!self->state.facing_right && dx <= 0.0f);

        bool should_attack = false;

        if (self->state.alive && enemy->state.alive && self->state.has_sword &&
            self->state.attack_cooldown <= 0.0f && enemy_is_in_front) {

            switch (mode) {
                case AI_MODE_AGGRESSIVE:

                    should_attack = (fabsf(dx) <= attack_range_x * 1.5f && dy <= attack_range_y * 1.5f);
                    break;
                case AI_MODE_TACTICAL:

                    should_attack = (fabsf(dx) <= attack_range_x * 1.2f && dy <= attack_range_y * 1.2f);
                    break;
                case AI_MODE_BALANCED:

                    should_attack = (fabsf(dx) <= attack_range_x && dy <= attack_range_y);
                    break;
                case AI_MODE_DEFENSIVE:

                    should_attack = (fabsf(dx) <= attack_range_x * 0.8f && dy <= attack_range_y * 0.8f);
                    break;
            }

            if (should_attack) {
                best_cmd.right_pressed = 0;
                best_cmd.left_pressed = 0;
                best_cmd.thrust_pressed = true;

                f32 self_y = self->state.pos.y;
                f32 enemy_y = enemy->state.pos.y;
                f32 height_diff = enemy_y - self_y;

                if (height_diff < -15.0f) {

                    best_cmd.target_sword_line = SWORD_LINE_HIGH;
                } else if (height_diff > 15.0f) {

                    best_cmd.target_sword_line = SWORD_LINE_LOW;
                } else {

                    best_cmd.target_sword_line = SWORD_LINE_MID;
                }

                if (mode == AI_MODE_AGGRESSIVE && fabsf(dx) < 2.0f) {

                    static i32 combo_counter = 0;
                    combo_counter++;
                    switch (combo_counter % 3) {
                        case 0: best_cmd.target_sword_line = SWORD_LINE_HIGH; break;
                        case 1: best_cmd.target_sword_line = SWORD_LINE_MID; break;
                        case 2: best_cmd.target_sword_line = SWORD_LINE_LOW; break;
                    }
                } else if (mode == AI_MODE_TACTICAL && fabsf(dx) < 1.5f) {

                    static i32 tactical_combo = 0;
                    tactical_combo++;
                    if (tactical_combo % 2 == 0) {
                        best_cmd.target_sword_line = (height_diff < 0) ? SWORD_LINE_HIGH : SWORD_LINE_LOW;
                    }
                }
            }
        }
    }

    if (best_cmd.right_pressed == 0 && best_cmd.left_pressed == 0) {
        i32 advance_bias = 0;

        switch (mode) {
            case AI_MODE_AGGRESSIVE:

                advance_bias = 3;
                break;
            case AI_MODE_TACTICAL:

                advance_bias = 2;
                break;
            case AI_MODE_BALANCED:

                advance_bias = 2;
                break;
            case AI_MODE_DEFENSIVE:

                advance_bias = 1;
                break;
        }

        if (self_index == 0) {
            best_cmd.right_pressed = advance_bias;
        } else {
            best_cmd.left_pressed = -advance_bias;
        }
    }

    return best_cmd;
}
