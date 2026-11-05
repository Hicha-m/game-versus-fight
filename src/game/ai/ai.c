#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "game/ai/ai.h"
#include "ai_internal.h"

static i32 difficulty_to_depth(AIDifficulty difficulty)
{
    switch (difficulty)
    {
    case AI_DIFFICULTY_EASY:
        return 2;
    case AI_DIFFICULTY_HARD:
        return 4;
    case AI_DIFFICULTY_EXPERT:
        return 5;
    case AI_DIFFICULTY_MEDIUM:
    default:
        return 3;
    }
}

static AIGameplayMode difficulty_to_gameplay_mode(AIDifficulty difficulty)
{
    switch (difficulty)
    {
    case AI_DIFFICULTY_EASY:
        return AI_MODE_DEFENSIVE;
    case AI_DIFFICULTY_MEDIUM:
        return AI_MODE_BALANCED;
    case AI_DIFFICULTY_HARD:
        return AI_MODE_TACTICAL;
    case AI_DIFFICULTY_EXPERT:
    default:
        return AI_MODE_AGGRESSIVE;
    }
}

static f64 now_ms(void)
{
    struct timespec ts;

    if (timespec_get(&ts, TIME_UTC) != TIME_UTC)
    {
        return 0.0;
    }

    return (f64)ts.tv_sec * 1000.0 + (f64)ts.tv_nsec / 1000000.0;
}

void ai_weights_for_mode(AIHeuristicWeights *out_weights, AIGameplayMode mode)
{
    if (!out_weights)
    {
        return;
    }

    switch (mode)
    {
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

void ai_default_weights(AIHeuristicWeights *out_weights)
{
    if (!out_weights)
    {
        return;
    }

    ai_weights_for_mode(out_weights, AI_MODE_BALANCED);
}

void ai_set_difficulty(AIController *ai, AIDifficulty difficulty)
{
    if (!ai)
    {
        return;
    }
    ai->difficulty = difficulty;
    ai->search_depth = difficulty_to_depth(difficulty);

    ai_set_gameplay_mode(ai, difficulty_to_gameplay_mode(difficulty));
}

void ai_set_algorithm(AIController *ai, AIAlgorithm algorithm)
{
    if (!ai)
    {
        return;
    }
    ai->algorithm = algorithm;
}

void ai_set_gameplay_mode(AIController *ai, AIGameplayMode mode)
{
    if (!ai)
    {
        return;
    }
    ai->gameplay_mode = mode;

    ai_weights_for_mode(&ai->weights, mode);
}

void ai_reset_metrics(AIController *ai)
{
    if (!ai)
    {
        return;
    }
    memset(&ai->metrics, 0, sizeof(ai->metrics));
}

bool ai_init(AIController *ai)
{
    if (!ai)
    {
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

void ai_shutdown(AIController *ai)
{
    (void)ai;
}

void ai_build_decision_state(AIDecisionState *out_state, const Arena *arena,
                             const CombatState *combat, i32 self_index)
{
    const Fighter *self;
    const Fighter *enemy;
    const Room *room;
    i32 enemy_index;
    i32 room_width_px;
    i32 max_bucket;

    if (!out_state || !arena || !combat || self_index < 0 || self_index >= MAX_PLAYERS)
    {
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

    if (room)
    {
        room_width_px = room->width_tiles * TILE_SIZE;
        max_bucket = room_width_px / (TILE_SIZE * 2);
        if (max_bucket < 1)
        {
            max_bucket = 1;
        }

        out_state->room_max_x_bucket = max_bucket;
        out_state->self_near_edge =
            (self->state.pos.x < 80.0f) || (self->state.pos.x > room_width_px - 80.0f);
        out_state->enemy_near_edge =
            (enemy->state.pos.x < 80.0f) || (enemy->state.pos.x > room_width_px - 80.0f);
    }
    else
    {
        out_state->room_max_x_bucket = 20;
    }
}

f32 ai_evaluate_state(const AIDecisionState *state, const AIHeuristicWeights *weights)
{
    i32 dx;
    i32 dy;
    f32 room_progress;
    f32 room_pos_progress;
    f32 enemy_distance_bonus;
    bool enemy_between_and_close;
    bool in_attack_range;
    f32 score = 0.0f;

    if (!state || !weights)
    {
        return 0.0f;
    }

    dx = state->enemy_x_bucket - state->self_x_bucket;
    dy = state->enemy_y_bucket - state->self_y_bucket;
    room_progress =
        (f32)(state->current_room - (state->room_count / 2)) * (f32)state->objective_direction;

    if (state->objective_direction > 0)
    {

        room_pos_progress = (f32)state->self_x_bucket / (f32)state->room_max_x_bucket;
    }
    else
    {

        room_pos_progress =
            (f32)(state->room_max_x_bucket - state->self_x_bucket) / (f32)state->room_max_x_bucket;
    }

    enemy_distance_bonus = (3.0f - fabsf((f32)dx)) * weights->distance_weight * 0.2f;

    enemy_between_and_close = false;
    if (state->objective_direction > 0)
    {
        enemy_between_and_close = (state->enemy_x_bucket >= state->self_x_bucket) && (abs(dx) <= 2);
    }
    else
    {
        enemy_between_and_close = (state->enemy_x_bucket <= state->self_x_bucket) && (abs(dx) <= 2);
    }

    in_attack_range = (fabsf((f32)dx) <= 2.5f) && (fabsf((f32)dy) <= 1.5f);

    score += room_progress * 12.0f;
    score += room_pos_progress * 15.0f;
    score += enemy_distance_bonus;

    score += (state->self_has_sword ? 1.0f : -1.0f) * weights->sword_advantage_weight;

    score += (dy > 0 ? 1.0f : -1.0f) * weights->height_advantage_weight * 0.5f;

    if (state->self_near_edge && !enemy_between_and_close)
    {

        score -= weights->edge_danger_weight;
    }
    if (state->enemy_near_edge)
    {
        score += weights->edge_danger_weight * 1.5f;

        if (in_attack_range && state->self_has_sword && state->self_can_thrust)
        {
            score += weights->attack_opportunity_weight * 3.0f;
        }
    }

    if (state->self_can_thrust && fabsf((f32)dx) <= 2.5f)
    {
        score += weights->attack_opportunity_weight * (enemy_between_and_close ? 1.5f : 1.0f);

        if (state->self_has_sword && in_attack_range)
        {
            score += weights->attack_opportunity_weight * 2.0f;
        }
    }
    if (state->enemy_can_thrust && fabsf((f32)dx) <= 2.5f)
    {
        score -= weights->attack_opportunity_weight * 0.8f;
    }

    if (!state->self_grounded)
    {
        score -= 0.1f;
    }
    if (!state->enemy_grounded)
    {
        score += 0.3f;

        if (in_attack_range && state->self_can_thrust)
        {
            score += weights->attack_opportunity_weight * 0.8f;
        }
    }

    if (state->self_has_sword && !state->enemy_has_sword && in_attack_range)
    {
        score += weights->sword_advantage_weight * 0.5f;
    }

    score += 1.0f;

    f32 proximity_bonus = (2.5f - fabsf((f32)dx)) * 2.0f;
    if (proximity_bonus > 0.0f)
    {
        score += proximity_bonus;
    }

    return score;
}

PlayerCommand ai_action_to_command(AIAction action)
{
    PlayerCommand cmd;
    memset(&cmd, 0, sizeof(cmd));
    cmd.target_sword_line = SWORD_LINE_MID;
    cmd.set_sword_line = (action == AI_ACTION_LINE_HIGH || action == AI_ACTION_LINE_MID ||
                          action == AI_ACTION_LINE_LOW);

    switch (action)
    {
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
        cmd.set_sword_line = (action == AI_ACTION_LINE_HIGH || action == AI_ACTION_LINE_MID ||
                              action == AI_ACTION_LINE_LOW);
        break;
    case AI_ACTION_THROW:
        cmd.throw_pressed = true;
        break;
    }

    return cmd;
}

static PlayerCommand action_command(AIAction action, i32 index)
{
    PlayerCommand cmd = ai_action_to_command(action);
    if (index == 1)
    {
        bool right = cmd.right_pressed;
        cmd.right_pressed = cmd.left_pressed;
        cmd.left_pressed = right;
    }
    return cmd;
}

static f32 evaluate_combat(const CombatState *c, i32 self)
{
    const FighterState *a = &c->fighters[self].state;
    const FighterState *b = &c->fighters[1 - self].state;
    f32 score = (a->alive ? 0 : -1000) + (b->alive ? 0 : 1000);
    score += (a->has_sword ? 12 : 0) - (b->has_sword ? 12 : 0);
    score += (a->is_stunned ? -30 : 0) + (b->is_stunned ? 30 : 0);
    f32 distance = fabsf(a->pos.x - b->pos.x);
    score -= fminf(distance, 400) * 0.025f;
    if (c->runner_index == self)
        score += a->pos.x * (self == 0 ? 0.03f : -0.03f);
    if (c->runner_index == 1 - self)
        score -= b->pos.x * (self == 0 ? -0.03f : 0.03f);
    return score;
}

static void simulate(CombatState *c, Arena *arena, PlayerCommand cmd, i32 actor)
{
    PlayerCommand idle = {0};
    for (i32 tick = 0; tick < 8; tick++)
    {
        combat_step(c, arena, actor == 0 ? &cmd : &idle, actor == 1 ? &cmd : &idle, FIXED_DT);
        for (i32 i = 0; i < c->event_count; i++)
            if (c->events[i].type == COMBAT_EVENT_DEATH)
                c->runner_index = c->events[i].actor;
        cmd.jump_pressed = cmd.thrust_pressed = cmd.throw_pressed = false;
    }
}

/* Both searches use the production simulation. Alpha-beta differs only by pruning.
 * Budget bounds worst-case work; metrics report visited nodes and peak stack estimate. */
static f32 search(CombatState *state, Arena *arena, i32 self, i32 depth, i32 actor, f32 alpha,
                  f32 beta, bool prune, AIMetrics *metrics, i32 ply)
{
    metrics->nodes_expanded++;
    if (ply > metrics->max_depth_reached)
        metrics->max_depth_reached = ply;
    if (depth <= 0 || metrics->nodes_expanded >= 1024 || !state->fighters[0].state.alive ||
        !state->fighters[1].state.alive)
    {
        metrics->leaves_evaluated++;
        return evaluate_combat(state, self);
    }
    f32 best = actor == self ? -1e9f : 1e9f;
    for (i32 i = 0; i < AI_MAX_ACTIONS; i++)
    {
        CombatState next = *state;
        simulate(&next, arena, action_command((AIAction)i, actor), actor);
        f32 score =
            search(&next, arena, self, depth - 1, 1 - actor, alpha, beta, prune, metrics, ply + 1);
        if (actor == self)
        {
            best = fmaxf(best, score);
            alpha = fmaxf(alpha, best);
        }
        else
        {
            best = fminf(best, score);
            beta = fminf(beta, best);
        }
        if (prune && beta <= alpha)
        {
            metrics->cutoffs++;
            break;
        }
        if (metrics->nodes_expanded >= 1024)
            break;
    }
    return best;
}

static PlayerCommand scripted(const Arena *arena, const CombatState *combat, i32 index)
{
    PlayerCommand cmd = {0};
    const Fighter *self = &combat->fighters[index];
    const Fighter *enemy = &combat->fighters[1 - index];
    if (!self->state.alive)
        return cmd;
    f32 dx = enemy->state.pos.x - self->state.pos.x;
    i32 direction = enemy->state.alive && combat->runner_index != index ? (dx >= 0 ? 1 : -1)
                                                                        : (index == 0 ? 1 : -1);
    if (!self->state.has_sword)
    {
        f32 closest = 1e9f;
        for (i32 i = 0; i < MAX_PLAYERS; i++)
            if (combat->ground_swords[i].active)
            {
                f32 distance = combat->ground_swords[i].pos.x - self->state.pos.x;
                if (fabsf(distance) < closest)
                {
                    closest = fabsf(distance);
                    direction = distance >= 0 ? 1 : -1;
                }
            }
        cmd.down_pressed = true;
    }
    cmd.right_pressed = direction > 0;
    cmd.left_pressed = direction < 0;
    cmd.set_sword_line = true;
    cmd.target_sword_line = (SwordLine)((enemy->state.sword_line + 1) % 3);
    cmd.thrust_pressed = enemy->state.alive && fabsf(dx) < 60 && self->state.attack_cooldown <= 0;
    if (cmd.thrust_pressed)
    {
        cmd.right_pressed = dx >= 0;
        cmd.left_pressed = dx < 0;
    }
    const Room *room = arena_get_current_room_const(arena);
    if (room)
    {
        i32 tx = (i32)((self->state.pos.x + PLAYER_WIDTH / 2 + direction * 48) / TILE_SIZE);
        i32 ty = (i32)((self->state.pos.y + PLAYER_HEIGHT + 4) / TILE_SIZE);
        TileType below = room_get_tile(room, tx, ty);
        TileType ahead = room_get_tile(room, tx, ty - 1);
        cmd.jump_pressed = self->state.grounded &&
                           (below == TILE_EMPTY || below == TILE_HAZARD || ahead == TILE_SOLID);
    }
    return cmd;
}

PlayerCommand ai_think(AIController *ai, const Arena *arena, const CombatState *combat, i32 self,
                       f32 dt, const i32 *kills)
{
    (void)kills;
    PlayerCommand none = {0};
    if (!ai || !ai->enabled || !arena || !combat || self < 0 || self >= MAX_PLAYERS ||
        !combat->fighters[self].state.alive)
        return none;
    ai->decision_timer -= dt;
    if (ai->decision_timer > 0)
        return ai->cached_command;
    ai_reset_metrics(ai);
    f64 start = now_ms();
    PlayerCommand result = scripted(arena, combat, self);
    if (ai->algorithm != AI_ALGO_SCRIPTED)
    {
        Arena snapshot = *arena;
        f32 best = -1e9f;
        i32 depth = ai->search_depth;
        if (depth < 1)
            depth = 1;
        if (depth > 5)
            depth = 5;
        for (i32 i = 0; i < AI_MAX_ACTIONS; i++)
        {
            CombatState next = *combat;
            PlayerCommand cmd = action_command((AIAction)i, self);
            simulate(&next, &snapshot, cmd, self);
            f32 score = search(&next, &snapshot, self, depth - 1, 1 - self, -1e9f, 1e9f,
                               ai->algorithm == AI_ALGO_MINIMAX_ALPHA_BETA, &ai->metrics, 1);
            if (score > best)
            {
                best = score;
                result = cmd;
            }
            if (ai->metrics.nodes_expanded >= 1024)
                break;
        }
        ai->metrics.estimated_memory_bytes =
            (u64)(ai->metrics.max_depth_reached + 1) * sizeof(CombatState) + sizeof(Arena);
    }
    else
        ai->metrics.nodes_expanded = 1;
    ai->metrics.think_time_ms = now_ms() - start;
    ai->cached_command = result;
    ai->decision_timer = 0.22f - (f32)ai->difficulty * 0.05f;
    return result;
}
