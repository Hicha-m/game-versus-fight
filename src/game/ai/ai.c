#include <math.h>
#include <stdlib.h>
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

/* Poids heuristiques adaptés au mode de gameplay */
void ai_weights_for_mode(AIHeuristicWeights* out_weights, AIGameplayMode mode)
{
    if (!out_weights) {
        return;
    }

    switch (mode) {
        case AI_MODE_DEFENSIVE:
            /* Priorité: sécurité et éviter les dangers */
            out_weights->distance_weight = 0.3f;           /* réduit: moins agressif */
            out_weights->sword_advantage_weight = 4.0f;    /* faible: avoid les combats */
            out_weights->height_advantage_weight = 0.3f;   /* minime */
            out_weights->center_control_weight = 0.2f;     /* ignore */
            out_weights->edge_danger_weight = 3.0f;        /* MODÉRÉ: évite les bords mais pas paniqué */
            out_weights->attack_opportunity_weight = 1.5f; /* minimal: avoid attacking */
            out_weights->stun_penalty_weight = 8.0f;       /* ÉLEVÉ: avoid stun */
            break;

        case AI_MODE_BALANCED:
            /* Mix équilibré attaque/défense */
            out_weights->distance_weight = 2.0f;           /* augmenté pour encourager rapprochement */
            out_weights->sword_advantage_weight = 10.0f;   /* augmenté */
            out_weights->height_advantage_weight = 1.2f;   /* un peu plus agressif */
            out_weights->center_control_weight = 1.2f;     /* augmenté */
            out_weights->edge_danger_weight = 1.5f;        /* RÉDUIT: moins peur des bords */
            out_weights->attack_opportunity_weight = 8.0f; /* augmenté */
            out_weights->stun_penalty_weight = 4.0f;       /* réduit: acceptable */
            break;

        case AI_MODE_TACTICAL:
            /* Attaques intelligentes avec stratégie */
            out_weights->distance_weight = 2.5f;           /* très agressif en rapprochement */
            out_weights->sword_advantage_weight = 18.0f;   /* TRÈS ÉLEVÉ: priorize épée */
            out_weights->height_advantage_weight = 2.0f;   /* très agressif en hauteur */
            out_weights->center_control_weight = 2.0f;     /* contrôle position agressif */
            out_weights->edge_danger_weight = 0.8f;        /* TRÈS FAIBLE: ignore les bords */
            out_weights->attack_opportunity_weight = 14.0f;/* TRÈS ÉLEVÉ: cherche attaques */
            out_weights->stun_penalty_weight = 2.0f;       /* très peu peur du stun */
            break;

        case AI_MODE_AGGRESSIVE:
            /* Maximise les dégâts et les attaques - MODE EXPERT */
            out_weights->distance_weight = 3.0f;           /* MAXIMAL: fonce directement */
            out_weights->sword_advantage_weight = 25.0f;   /* EXTRÊME: valorise épée absolument */
            out_weights->height_advantage_weight = 3.0f;   /* extremement agressif en hauteur */
            out_weights->center_control_weight = 1.0f;     /* ignore positionnement de défense */
            out_weights->edge_danger_weight = 0.2f;        /* IGNORÉ PRESQUE COMPLÈTEMENT */
            out_weights->attack_opportunity_weight = 20.0f;/* EXTRÊME: TOUJOURS attaquer */
            out_weights->stun_penalty_weight = 0.5f;       /* ignore le stun: fonce coûte que coûte */
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

    /* Poids par défaut (mode BALANCED) */
    ai_weights_for_mode(out_weights, AI_MODE_BALANCED);
}

void ai_set_difficulty(AIController* ai, AIDifficulty difficulty)
{
    if (!ai) {
        return;
    }
    ai->difficulty = difficulty;
    ai->search_depth = difficulty_to_depth(difficulty);
    /* Mapper difficulté -> mode de gameplay */
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
    /* Mettre à jour les poids heuristiques pour ce mode */
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
    f32 enemy_distance_bonus;  /* CHANGÉ: bonus au lieu de penalty */
    bool enemy_between_and_close;
    bool in_attack_range;
    f32 score = 0.0f;

    if (!state || !weights) {
        return 0.0f;
    }

    dx = state->enemy_x_bucket - state->self_x_bucket;
    dy = state->enemy_y_bucket - state->self_y_bucket;
    room_progress = (f32)(state->current_room - (state->room_count / 2)) * (f32)state->objective_direction;
    
    /* Calculer la progression spatiale correctement pour les deux directions */
    if (state->objective_direction > 0) {
        /* Joueur 0: avancer vers la droite (x augmente) */
        room_pos_progress = (f32)state->self_x_bucket / (f32)state->room_max_x_bucket;
    } else {
        /* Joueur 1: avancer vers la gauche (x diminue) */
        room_pos_progress = (f32)(state->room_max_x_bucket - state->self_x_bucket) / (f32)state->room_max_x_bucket;
    }
    
    /* CHANGÉ: Récompenser le rapprochement à la place de le pénaliser */
    enemy_distance_bonus = (3.0f - fabsf((f32)dx)) * weights->distance_weight * 0.2f;

    enemy_between_and_close = false;
    if (state->objective_direction > 0) {
        enemy_between_and_close = (state->enemy_x_bucket >= state->self_x_bucket) && (abs(dx) <= 2);
    } else {
        enemy_between_and_close = (state->enemy_x_bucket <= state->self_x_bucket) && (abs(dx) <= 2);
    }

    in_attack_range = (fabsf((f32)dx) <= 2.5f) && (fabsf((f32)dy) <= 1.5f);

    /* Progression objectif - BONUS AUGMENTÉ pour encourager le mouvement */
    score += room_progress * 12.0f;        /* bonus pour progression entre les salles */
    score += room_pos_progress * 15.0f;    /* bonus pour progression au sein de la salle */
    score += enemy_distance_bonus;         /* NOUVEAU: bonus pour se rapprocher */

    /* Avantages d'armes */
    score += (state->self_has_sword ? 1.0f : -1.0f) * weights->sword_advantage_weight;

    /* Avantage de hauteur */
    score += (dy > 0 ? 1.0f : -1.0f) * weights->height_advantage_weight * 0.5f;

    /* Gestion des bords - TRÈS RÉDUITE */
    if (state->self_near_edge && !enemy_between_and_close) {
        /* Seulement pénaliser si pas de combat imminent */
        score -= weights->edge_danger_weight;
    }
    if (state->enemy_near_edge) {
        score += weights->edge_danger_weight * 1.5f;  /* BONUS: ennemi au bord */
        /* BONUS MAXIMAL: ennemi au bord = position favorable pour le tuer */
        if (in_attack_range && state->self_has_sword && state->self_can_thrust) {
            score += weights->attack_opportunity_weight * 3.0f;  /* MAXIMAL */
        }
    }

    /* Opportunités d'attaque - FORTEMENT AMPLIFIÉES */
    if (state->self_can_thrust && fabsf((f32)dx) <= 2.5f) {
        score += weights->attack_opportunity_weight * (enemy_between_and_close ? 1.5f : 1.0f);
        /* SUPER BONUS: si on peut faire un coup et on a l'épée */
        if (state->self_has_sword && in_attack_range) {
            score += weights->attack_opportunity_weight * 2.0f;  /* Augmenté */
        }
    }
    if (state->enemy_can_thrust && fabsf((f32)dx) <= 2.5f) {
        score -= weights->attack_opportunity_weight * 0.8f;  /* Moins pénalisant */
    }

    /* Gestion du sol */
    if (!state->self_grounded) {
        score -= 0.1f;  /* Réduit */
    }
    if (!state->enemy_grounded) {
        score += 0.3f;  /* Augmenté */
        /* Bonus agressif: ennemi en l'air et on peut attaquer = facile à hit */
        if (in_attack_range && state->self_can_thrust) {
            score += weights->attack_opportunity_weight * 0.8f;
        }
    }

    /* Bonus pour les combats favorables - AUGMENTÉ */
    if (state->self_has_sword && !state->enemy_has_sword && in_attack_range) {
        score += weights->sword_advantage_weight * 0.5f;  /* Augmenté */
    }
    
    /* BONUS AGRESSIF: Récompenser constamment la progression et l'engagement */
    score += 1.0f;  /* Bonus de base augmenté pour chaque action */
    
    /* BONUS POUR COMBAT PROCHE: Plus on est proche, mieux c'est */
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

/* Retourne l'ordre d'actions prioritaires selon le mode de gameplay */
static void ai_get_action_priority_order(AIAction* out_actions, AIGameplayMode mode)
{
    static const AIAction aggressive_order[] = {
        AI_ACTION_ADVANCE,          /* Priorité 1: FONCE directement vers l'ennemi */
        AI_ACTION_THRUST,           /* Priorité 2: TOUJOURS attaquer si possible */
        AI_ACTION_LINE_HIGH,        /* Priorité 3: diversifier attaque pour max dégâts */
        AI_ACTION_LINE_LOW,         /* Priorité 4: diversifier attaque */
        AI_ACTION_JUMP,             /* Priorité 5: mobilité offensive */
        AI_ACTION_LINE_MID,         /* Priorité 6: attaque standard */
        AI_ACTION_THROW,            /* Priorité 7: lancer arme si nécessaire */
        AI_ACTION_RETREAT            /* Priorité 8: JAMAIS reculer */
    };

    static const AIAction tactical_order[] = {
        AI_ACTION_ADVANCE,          /* Priorité 1: se positionner agressivement */
        AI_ACTION_THRUST,           /* Priorité 2: attaquer quand possible */
        AI_ACTION_JUMP,             /* Priorité 3: mobilité tactique */
        AI_ACTION_LINE_HIGH,        /* Priorité 4: coups variés */
        AI_ACTION_LINE_MID,         /* Priorité 5: attaque standard */
        AI_ACTION_LINE_LOW,         /* Priorité 6: coups variés */
        AI_ACTION_RETREAT,          /* Priorité 7: reculer seulement si nécessaire */
        AI_ACTION_THROW             /* Priorité 8: jeter arme dernier recours */
    };

    static const AIAction balanced_order[] = {
        AI_ACTION_ADVANCE,          /* Priorité 1: avancer */
        AI_ACTION_THRUST,           /* Priorité 2: attaquer */
        AI_ACTION_JUMP,             /* Priorité 3: mobilité */
        AI_ACTION_LINE_MID,         /* Priorité 4: attaque standard */
        AI_ACTION_LINE_HIGH,        /* Priorité 5: coups variés */
        AI_ACTION_LINE_LOW,         /* Priorité 6: coups variés */
        AI_ACTION_RETREAT,          /* Priorité 7: reculer si nécessaire */
        AI_ACTION_THROW             /* Priorité 8: lancer */
    };

    static const AIAction defensive_order[] = {
        AI_ACTION_RETREAT,          /* Priorité 1: reculer pour sécurité */
        AI_ACTION_JUMP,             /* Priorité 2: éviter danger */
        AI_ACTION_ADVANCE,          /* Priorité 3: avancer très lentement */
        AI_ACTION_LINE_MID,         /* Priorité 4: attaque passive */
        AI_ACTION_LINE_HIGH,        /* Priorité 5: coups défensifs */
        AI_ACTION_LINE_LOW,         /* Priorité 6: coups défensifs */
        AI_ACTION_THRUST,           /* Priorité 7: attaquer dernier recours */
        AI_ACTION_THROW             /* Priorité 8: jamais jeter arme */
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

    (void)dt;
    (void)player_kills;  /* Variable non utilisée dans la nouvelle logique */

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

    /* Utiliser l'ordre d'actions prioritaires selon le mode de gameplay */
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

        /* Logique combative aggravée selon le mode de gameplay */
        bool should_attack = false;
        
        if (self->state.alive && enemy->state.alive && self->state.has_sword && 
            self->state.attack_cooldown <= 0.0f && enemy_is_in_front) {
            
            /* Déterminer si on doit attaquer selon le mode */
            switch (mode) {
                case AI_MODE_AGGRESSIVE:
                    /* TOUJOURS attaquer - très agressif */
                    should_attack = (fabsf(dx) <= attack_range_x * 1.5f && dy <= attack_range_y * 1.5f);
                    break;
                case AI_MODE_TACTICAL:
                    /* Attaquer agressivement dans bonne position */
                    should_attack = (fabsf(dx) <= attack_range_x * 1.2f && dy <= attack_range_y * 1.2f);
                    break;
                case AI_MODE_BALANCED:
                    /* Attaquer quand proche */
                    should_attack = (fabsf(dx) <= attack_range_x && dy <= attack_range_y);
                    break;
                case AI_MODE_DEFENSIVE:
                    /* Attaquer seulement si très proche et sûr */
                    should_attack = (fabsf(dx) <= attack_range_x * 0.8f && dy <= attack_range_y * 0.8f);
                    break;
            }

            if (should_attack) {
                best_cmd.right_pressed = 0;
                best_cmd.left_pressed = 0;
                best_cmd.thrust_pressed = true;

                /* Calcul du placement de l'épée selon la position de l'ennemi */
                f32 self_y = self->state.pos.y;
                f32 enemy_y = enemy->state.pos.y;
                f32 height_diff = enemy_y - self_y;

                if (height_diff < -15.0f) {
                    /* Ennemi bien plus haut: attaque haute */
                    best_cmd.target_sword_line = SWORD_LINE_HIGH;
                } else if (height_diff > 15.0f) {
                    /* Ennemi bien plus bas: attaque basse */
                    best_cmd.target_sword_line = SWORD_LINE_LOW;
                } else {
                    /* Ennemi au même niveau: attaque au milieu */
                    best_cmd.target_sword_line = SWORD_LINE_MID;
                }
                
                /* Mode agressif: alterner les attaques pour combos maximaux */
                if (mode == AI_MODE_AGGRESSIVE && fabsf(dx) < 2.0f) {
                    /* TRÈS PROCHE: alterner high/mid/low pour maximum de dégâts */
                    static i32 combo_counter = 0;
                    combo_counter++;
                    switch (combo_counter % 3) {
                        case 0: best_cmd.target_sword_line = SWORD_LINE_HIGH; break;
                        case 1: best_cmd.target_sword_line = SWORD_LINE_MID; break;
                        case 2: best_cmd.target_sword_line = SWORD_LINE_LOW; break;
                    }
                } else if (mode == AI_MODE_TACTICAL && fabsf(dx) < 1.5f) {
                    /* Mode tactique: aussi alterner mais moins */
                    static i32 tactical_combo = 0;
                    tactical_combo++;
                    if (tactical_combo % 2 == 0) {
                        best_cmd.target_sword_line = (height_diff < 0) ? SWORD_LINE_HIGH : SWORD_LINE_LOW;
                    }
                }
            }
        }
    }

    /* Mouvement intelligent basé sur le mode de gameplay */
    if (best_cmd.right_pressed == 0 && best_cmd.left_pressed == 0) {
        i32 advance_bias = 0;
        
        /* Logique d'avancée agressive selon le mode */
        switch (mode) {
            case AI_MODE_AGGRESSIVE:
                /* EXPLORER EXPERT: FONCER TOUJOURS - agressivité maximale */
                advance_bias = 3;
                break;
            case AI_MODE_TACTICAL:
                /* Mode HARD: avancer agressivement mais intelligemment */
                advance_bias = 2;
                break;
            case AI_MODE_BALANCED:
                /* Mode MEDIUM: avancer normalement */
                advance_bias = 2;
                break;
            case AI_MODE_DEFENSIVE:
                /* Mode EASY: progresser lentement et prudemment */
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
