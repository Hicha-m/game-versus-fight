#include <math.h>
#include <string.h>
#include <time.h>

#include "game/ai/ai.h"
#include "ai_internal.h"

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

static f64 now_ms(void)
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

void ai_default_weights(AIHeuristicWeights* out_weights)
{
    if (!out_weights) {
        return;
    }

    out_weights->distance_weight = 1.5f;
    out_weights->sword_advantage_weight = 10.0f;
    out_weights->height_advantage_weight = 1.0f;
    out_weights->center_control_weight = 1.0f;
    out_weights->edge_danger_weight = 4.0f;
    out_weights->attack_opportunity_weight = 6.0f;
    out_weights->stun_penalty_weight = 8.0f;
}

void ai_set_difficulty(AIController* ai, AIDifficulty difficulty)
{
    if (!ai) {
        return;
    }
    ai->difficulty = difficulty;
    ai->search_depth = difficulty_to_depth(difficulty);
}

void ai_set_algorithm(AIController* ai, AIAlgorithm algorithm)
{
    if (!ai) {
        return;
    }
    ai->algorithm = algorithm;
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
    ai->search_depth = difficulty_to_depth(ai->difficulty);
    ai_default_weights(&ai->weights);
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

    if (!out_state || !arena || !combat || self_index < 0 || self_index >= MAX_PLAYERS) {
        return;
    }

    memset(out_state, 0, sizeof(*out_state));

    enemy_index = (self_index == 0) ? 1 : 0;

    self = &combat->fighters[self_index];
    enemy = &combat->fighters[enemy_index];
    room = arena_get_current_room_const(arena);

    out_state->current_room = arena->current_room;

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
        out_state->self_near_edge = (self->state.pos.x < 80.0f) || (self->state.pos.x > room_width_px - 80.0f);
        out_state->enemy_near_edge = (enemy->state.pos.x < 80.0f) || (enemy->state.pos.x > room_width_px - 80.0f);
    }
}

f32 ai_evaluate_state(
    const AIDecisionState* state,
    const AIHeuristicWeights* weights
)
{
    i32 dx;
    i32 dy;
    i32 center;
    f32 score = 0.0f;

    if (!state || !weights) {
        return 0.0f;
    }

    dx = state->enemy_x_bucket - state->self_x_bucket;
    dy = state->enemy_y_bucket - state->self_y_bucket;
    center = (state->self_x_bucket + state->enemy_x_bucket) / 2;

    score -= fabsf((f32)dx) * weights->distance_weight * 0.4f;
    score += (state->self_has_sword ? 1.0f : -1.0f) * weights->sword_advantage_weight;
    score += (dy > 0 ? 1.0f : -1.0f) * weights->height_advantage_weight * 0.5f;
    score -= fabsf((f32)(center - 10)) * weights->center_control_weight * 0.1f;

    if (state->self_near_edge) {
        score -= weights->edge_danger_weight;
    }
    if (state->enemy_near_edge) {
        score += weights->edge_danger_weight;
    }

    if (state->self_can_thrust && fabsf((f32)dx) <= 2.0f) {
        score += weights->attack_opportunity_weight;
    }
    if (state->enemy_can_thrust && fabsf((f32)dx) <= 2.0f) {
        score -= weights->attack_opportunity_weight;
    }

    if (!state->self_grounded) {
        score -= 0.2f;
    }
    if (!state->enemy_grounded) {
        score += 0.2f;
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
            cmd.move_x = 1;
            break;
        case AI_ACTION_RETREAT:
            cmd.move_x = -1;
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

    *near_edge = (*x_bucket < 2 || *x_bucket > 18);
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

PlayerCommand ai_think(
    AIController* ai,
    const Arena* arena,
    const CombatState* combat,
    i32 self_index,
    f32 dt
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

    AIHeuristicWeights weights;
    AIDecisionState state;
    PlayerCommand best_cmd;
    f32 best_score = -1000000.0f;
    i32 best_action_index = 0;
    i32 depth;
    i32 i;
    f64 start_ms;
    f64 end_ms;

    (void)dt;

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

    for (i = 0; i < (i32)(sizeof(actions) / sizeof(actions[0])); ++i) {
        AIDecisionState next = state;
        f32 score;
        ai_apply_action_to_state(&next, actions[i], true);
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
        ai->metrics.think_time_ms = end_ms - start_ms;
        ai->metrics.estimated_memory_bytes = ai->metrics.nodes_expanded * (u64)sizeof(AIDecisionState);
    }

    best_cmd = ai_action_to_command(actions[best_action_index]);

    if (best_cmd.move_x == 0 && combat) {
        const Fighter* self = &combat->fighters[self_index];
        const Fighter* enemy = &combat->fighters[(self_index == 0) ? 1 : 0];
        if (enemy->state.pos.x > self->state.pos.x + 8.0f) {
            best_cmd.move_x = 1;
        } else if (enemy->state.pos.x < self->state.pos.x - 8.0f) {
            best_cmd.move_x = -1;
        }
    }

    return best_cmd;
}
