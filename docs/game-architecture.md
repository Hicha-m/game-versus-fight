---
title: 'Game Architecture'
project: 'jeux IA'
game_name: 'Blade Rush'
date: '2026-03-04'
author: 'Hicham'
version: '1.0'
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8, 9]
status: 'complete'
engine: 'SDL3'
platform: 'PC (Windows/Linux)'

# Source Documents
gdd: '_bmad-output/gdd.md'
epics: '_bmad-output/epics.md'
brief: '_bmad-output/game-brief.md'
---

# Game Architecture - Blade Rush

## Document Status

This architecture document has been completed through the GDS Architecture Workflow.

**Steps Completed:** 9 of 9 (Complete)

---

## Executive Summary

**Blade Rush** architecture is designed for **SDL3** targeting **PC (Windows/Linux)**.

**Key Architectural Decisions:**

- Boucle de jeu à **fixed timestep 60 Hz** avec interpolation de rendu pour garantir le frame-perfect.
- Architecture modulaire **engine/combat/ai/arena/utils** avec contrats d'intégration **IP1/IP2** pour isolation Lane A/B.
- IA **MinMax time-budgeted** et génération procédurale **Generate-Validate-Retry** pour respecter performance et fairness.

**Project Structure:** organisation modulaire par responsabilité avec 6 systèmes cœur (moteur, combat, IA, arène, utilitaires, orchestration match).

**Implementation Patterns:** 9 patterns définis pour assurer la cohérence d'implémentation des agents IA.

**Ready for:** Epic implementation phase.

---

## Contexte du Projet

### Vue d'ensemble du Jeu

**Blade Rush** — Jeu de combat d'escrime tactique 1v1 en 2D où deux combattants s'affrontent dans des duels rapides avec système de hauteur d'épée (Haut/Mid/Bas). Inspiré de Nidhogg, enrichi par génération procédurale des arènes et IA MinMax challengeante.

### Portée Technique

**Plateforme:** PC (Windows/Linux)  
**Genre:** Fighting 1v1 — Combat rapide + contrôle territorial  
**Complexité:** Intermédiaire-Avancée

### Systèmes Fondamentaux

| Système | Complexité | Impact Architectural |
|---------|------------|----------------------|
| Moteur de Combat 1v1 | ⚫⚫⚫ Haute | Timing frame-perfect, input latency critique |
| IA MinMax + Alpha-Beta | ⚫⚫⚫ Haute | Budget temps strict <1ms, state discrétisé |
| Génération Procédurale | ⚫⚫ Moyenne-Haute | Algorithmes validation fairness, seed gestion |
| Momentum Dynamique | ⚫⚫ Moyenne | Calculs vitesse progressive, reset gestion |
| Physique 2D | ⚫⚫ Moyenne | Collisions, gravité, déterminisme |
| Rendu SDL3 | ⚫ Basse | Sprites 2D, integer scaling |

### Exigences de Performance

- **60 FPS constant** (aucune chute <55) — non négociable pour combat réactif
- **Input latency <30ms** — fenêtres frame-perfect parades
- **IA <1ms/frame** — toutes difficultés
- **Chargement <2s** — génération procédurale incluse

### Contraintes de Développement

**Stack Technique:**
- **Langage:** C pur (gestion mémoire manuelle)
- **Framework:** SDL3 (rendu, input, audio)
- **Build:** <50 Mo, portable, offline

**Structure Équipe:**
- **Lane A (Hicham):** Gameplay, Combat, UI/UX
- **Lane B (Walid):** IA MinMax, Génération Procédurale
- **Intégration:** IP1 (Arena), IP2 (IA), IP3 (MVP)

### Drivers de Complexité

**Critical Path:**
1. IA MinMax temps réel avec contrainte <1ms
2. Génération procédurale avec fairness garantie
3. Combat frame-perfect avec input latency minimal

**Éléments Nouveaux:**
- Épée évolutive contextuelle (portée variable)
- Momentum progressif (vitesse dynamique)
- Systèmes couplés (momentum + épée + parades)

### Risques Techniques Identifiés

1. **Performance IA** — Profondeur arbre vs budget temps
2. **Fairness Procédurale** — Validation équilibre arènes
3. **Input Latency** — SDL3 VSync/buffering impact
4. **Déterminisme Physique** — Stabilité float collisions
5. **Fuites Mémoire** — Allocation dynamique C

---

## Engine & Framework

### Framework Sélectionné

**SDL3 (Simple DirectMedia Layer 3)** v3.4.0+

**Rationnelle:**
- Framework bas niveau en C pur (conformité projet scolaire)
- Rendu 2D accéléré hardware avec contrôle fine performance
- Support natif Windows/Linux sans dépendances lourdes
- Input handling avec latency minimale (critique pour combat frame-perfect)
- Footprint léger (<50 Mo build final)

### Validation Compatibilité

**Fedora 42 (Distribution de développement):**
```bash
dnf info SDL3-devel
# Résultat: SDL3-devel 3.4.0-3.fc42 (x86_64) - Disponible dans repos "updates"

dnf repoquery --requires SDL3-devel
# Dépendances: libSDL3.so.0, pkg-config, GL/GLU, X11
```

**Versions SDL3 Compatibles:**
- Stable actuelle : **3.4.2** (GitHub releases)
- Fedora 42 : **3.4.0** (repos updates)
- Fallback : Compilation source depuis [github.com/libsdl-org/SDL](https://github.com/libsdl-org/SDL)

### Installation SDL3

**Fedora (recommandé pour développement):**
```bash
sudo dnf install SDL3-devel SDL3_image-devel SDL3_mixer-devel
```

**Ubuntu/Debian (alternative):**
```bash
sudo apt install libsdl3-dev libsdl3-image-dev libsdl3-mixer-dev
```

**Dépendances Vérifiées:**
- libSDL3.so.0 (core rendering)
- libSDL3_image.so (PNG/BMP loading)
- libSDL3_mixer.so (audio OGG/WAV)
- pkg-config, X11, GL/GLU (système)

### Décisions Fournies par SDL3

| Composant | Solution | Notes |
|-----------|----------|-------|
| **Rendu 2D** | SDL_Renderer + GPU acceleration | Sprites, textures, transformations |
| **Gestion Fenêtre** | SDL_Window API | Multi-résolution, fullscreen, borderless |
| **Input/Événements** | SDL_Event avec polling | Clavier, souris, contrôleur support |
| **Audio** | SDL_mixer (extension) | WAV, OGG multicanal |
| **Timing** | SDL_GetTicks64, SDL_Delay | Boucle 60 FPS avec delta time |
| **Images** | SDL_image (extension) | PNG, BMP format support |

### Décisions Restantes

SDL3 fournit les **primitives** mais l'architecture doit définir :

1. **Boucle de Jeu** — Fixed timestep 16.67ms vs variable delta
2. **Système d'Entités** — Structs C vs pattern abstrait
3. **Gestion Mémoire** — Pools statiques vs malloc/free dynamique
4. **Architecture Combat** — Représentation état joueur/IA/épée
5. **Arbre MinMax** — Profondeur, branching, cache memoization
6. **Génération Procédurale** — Algorithme, structures tiles, validation
7. **Organisation Code** — Structure répertoires, séparation concerns
8. **Contrats Lane A/B** — IP1 (Arena interface), IP2 (IA interface)

### Structure Projet Proposée

```
blade-rush/
├── src/
│   ├── main.c                 # Boucle jeu, SDL3 init
│   ├── engine/
│   │   ├── game.h/.c          # État jeu, update/render
│   │   ├── input.h/.c         # Gestion input clavier
│   │   ├── render.h/.c        # Rendu SDL3 (sprites, HUD)
│   │   └── timing.h/.c        # Boucle 60 FPS fixe
│   ├── combat/                # Lane A (Hicham)
│   │   ├── player.h/.c        # État joueur, mouvements
│   │   ├── attacks.h/.c       # Logique attaque/parade
│   │   ├── collision.h/.c     # Détection épées
│   │   └── physics.h/.c       # Gravité, saut, momentum
│   ├── ai/                    # Lane B (Walid)
│   │   ├── minmax.h/.c        # Arbre MinMax + alpha-beta
│   │   ├── state.h/.c         # État discrétisé pour IA
│   │   └── evaluator.h/.c     # Fonction eval position
│   ├── arena/                 # Lane B (Walid)
│   │   ├── generator.h/.c     # Algo génération procédurale
│   │   ├── tiles.h/.c         # Types tiles, représentation
│   │   └── validator.h/.c     # Vérification fairness
│   └── utils/
│       ├── memory.h/.c        # Allocation/déallocation
│       ├── debug.h/.c         # Hitboxes, state IA visuel
│       └── constants.h        # Defines globales
├── assets/
│   ├── sprites/               # PNG pixel art
│   ├── sfx/                   # OGG audio effects
│   └── music/                 # OGG background tracks
├── include/                   # Headers externes SDL3
├── Makefile                   # Build fix
└── README.md                  # Documentation
```

**Distribution MVP (Portable):**
Option A - Build statique SDL3:
```bash
gcc -static src/*.c -lSDL3 -lm -o blade-rush_static
# Résultat: ~2-5 Mo additionnel mais zéro dépendances
```

Option B - AppImage (recommandé):
```bash
# Après build, créer AppImage avec SDL3 embarquée
./appimagetool blade-rush.AppDir blade-rush.AppImage
# Résultat: Binaire unique + SDL3, fonctionne toutes distros
```

---

## Décisions Architecturales Fondamentales

### Synthèse des Décisions Critiques

| # | Domaine | Décision | Rationale | Implications |
|---|---------|----------|-----------|--------------|
| 1 | Physique | Custom Controller (C pur) | Contrôle déterministe, perf garantie, déterminisme | Gravité, collisions simple, validation fairness garantie |
| 2 | Mémoire | Malloc/Free Dynamique | Flexibilité arènes procédurale, gestion soignée | Patterns allocation claire, prévention fuites |
| 3 | Boucle Jeu | **Fixed Timestep 16.67ms (60 FPS)** | Non-négociable : combat frame-perfect, timing parades déterministe | Logique découplée rendu, accumulator pattern obligatoire |
| 4 | Input | **Buffering Custom 2-3 frames** | Feel compétitif standard (SF6, Tekken), égalité joueur/IA, réduction frustration | Buffer circulaire, 33-50ms fenêtre timing |
| 5 | Config/Save | JSON | Structure, parseable, professionnel, versionnable | Dépendance JSON library (cJSON ou jsmn), schéma versionnée |
| 6 | État Jeu | **Structs Modulaires (Vec2, Stats, Character)** | Maintenabilité, extensibilité, séparation concerns | Type-safe, searchable codebase, patterns clairs |
| 7 | Debug | ImGui Overlay + Debug Panel | Profiling temps réel IA/physique, state inspection | ImGui dependency (header-only possible), FPS counters, IA state display |
| 8 | Code | Modulaire src/{engine,combat,ai,arena,utils}/ | Parallélisable Lane A/B, séparation concerns nette | Contrats interfaces IP1/IP2 appliqués |

---

### 1. Système Physique — Custom Controller

**Décision:** Implémentation custom de physique 2D en C pur, pas de Box2D.

**Rationale:**
- **Déterminisme garanti** — Physique simple, seed-based, exactement reproductible
- **Performance** — Aucune overhead library externe, <1ms total physique/collision par frame
- **Contrôle total** — Gravité, collision, saut adaptés à mécanique combat unique
- **Footprint** — Pas de dépendance, C pur simplifie distribution

**Scope Physique Implémenté:**

```c
// Mouvements
- Déplacement horizontal (vélocité, accélération)
- Gravité simple (V = gt) avec landing detection
- Saut (impulsion verticale, contrôle aérien, double-saut optionnel IA)
- Plateformes/trous (raycasting simple)

// Collisions
- AABB (Axis-Aligned Bounding Boxes) pour personnages
- Raycast détection hit épée vs opponent
- Wall bounce (rebond limites map)
- Ground detection pour parade/attaque au sol vs air
```

**Implications:**
- Collisions bouclent chaque FIXED_DT → déterministe
- Pas de contact persistant complexe
- Gap detection simple pour saut IA automatique
- Testable offline (seed déterministe)

**Scope Exclu Intentionnellement:**
- Physique ragdoll
- Contact persistence callbacks
- Complex rigid body simulation

---

### 2. Gestion Mémoire — Malloc/Free Dynamique

**Décision:** Allocation mémoire dynamique via malloc/free standard C, patterns attention particulière.

**Rationale:**
- **Flexibilité** — Arènes procédurale de taille variable (50-200 units)
- **Simplicité** — Pas de dimensionnement pré-alloué, allocation JIT
- **Professional** — Patterns clairs + static analysis tool (valgrind, AddressSanitizer)

**Patterns de Gestion Mémoire Obligatoires:**

```c
// Pattern 1: Ownership clair
typedef struct { void* data; size_t capacity; } Pool;
Pool* pool_create(size_t item_size, int count);
void pool_destroy(Pool* p);  // Doit libérer données

// Pattern 2: Initialize/Cleanup explicite
void character_init(Character* c, float x, float y);
void character_cleanup(Character* c);

// Pattern 3: Allocation avec erreur handling
Arena* arena_generate(uint32_t seed) {
    Arena* a = malloc(sizeof(Arena));
    if (!a) return NULL;  // Error case
    
    a->tiles = calloc(width * height, sizeof(uint8_t));
    if (!a->tiles) {
        free(a);
        return NULL;  // Cleanup on partial failure
    }
    return a;
}

// Pattern 4: Systematic cleanup
void game_shutdown() {
    arena_destroy(current_arena);
    player_cleanup(&player);
    ai_cleanup(&opponent);
    // ... etc
}
```

**Audit Mémoire Obligatoire:**
- Avant release MVP: `valgrind --leak-check=full ./blade-rush`
- Zéro fuites toléré
- AddressSanitizer CI si possible

---

### 3. Boucle de Jeu — Fixed Timestep 16.67ms + Interpolation Rendu

**Décision:** Logique jeu tourne à timestep fixe 16.67ms (60 FPS), rendu variable avec interpolation visuelle.

**Rationale:**

**Why Fixed Timestep Mandatory:**
- **Déterminisme** — Chaque frame logique dure EXACTEMENT 16.67ms indépendamment CPU
- **Combat Frame-Perfect** — Parades fenêtre définie (startup 2 frames = 33.34ms)
- **IA Synchronisation** — MinMax évalue états aux mêmes timestamps
- **Seed Replay** — Match identical replay possible
- **Collision Cohérente** — Position et hitboxes synchronisées (pas de désync)

**Problème Delta Time (rejeté après analyse):**
```
Frame 1: actual_frame_time = 16ms  → logic dt = 0.016s
Frame 2: lag spike             → actual_frame_time = 30ms  → logic dt = 0.030s
Frame 3: recover              → actual_frame_time = 16ms  → logic dt = 0.016s

Résultat: 
- Velocity, momentum, collision timing INCONSISTENT
- Parade qui marchait frame 1 échoue frame 2 sur lag
- IA décide différemment selon lag
- Position joueur et hitbox épée DÉSYNCHRONISÉES
- Gameplay imprévisible
```

**Risque Hybride Delta/Fixed :**
```c
// Position joueur delta time
player.x += player.vx * delta_time;  // Avance rapide si lag

// Collision check fixed 60 Hz
check_collision_at(player.x_previous);  // Hitbox EN RETARD

// Résultat: Épée traverse adversaire, parade échoue mystérieusement
```

**Solution Professionnelle : Fixed Logic + Interpolated Rendering**

Cette approche = **Standard industrie fighting games** (SF6, Tekken 8, Skullgirls).

```c
#define FIXED_DT (1.0f / 60.0f)  // 0.01667s = 16.67ms

// État logique (double buffered pour interpolation)
typedef struct {
    Vec2 pos;
    Vec2 vel;
    // ... autres états
} CharacterState;

CharacterState player_current;   // État frame N
CharacterState player_previous;  // État frame N-1

int main() {
    uint64_t accumulator = 0;
    uint64_t last_ticks = SDL_GetTicks64();
    
    while (game_running) {
        uint64_t current_ticks = SDL_GetTicks64();
        uint64_t frame_delta = current_ticks - last_ticks;
        last_ticks = current_ticks;
        
        accumulator += frame_delta;
        
        // 1. LOGIQUE: Fixed 60 Hz (déterministe)
        while (accumulator >= FIXED_DT * 1000) {
            // Save état précédent pour interpolation
            player_previous = player_current;
            
            // Update logique toujours à FIXED_DT
            update_input(FIXED_DT);
            update_game(FIXED_DT);
            update_physics(FIXED_DT);
            check_collisions();
            update_ai(FIXED_DT);
            update_momentum(FIXED_DT);
            
            accumulator -= FIXED_DT * 1000;
        }
        
        // 2. RENDU: Variable (60-240 FPS selon écran) avec interpolation
        float alpha = (float)accumulator / (FIXED_DT * 1000);  // 0.0 to 1.0
        
        // Interpolate positions entre frame N-1 et N
        Vec2 player_render_pos = vec2_lerp(player_previous.pos, 
                                            player_current.pos, 
                                            alpha);
        
        render_game(player_render_pos, alpha);  // Rendu fluide
        
        // 3. VSync optionnel (si souhaité)
        // SDL_GL_SetSwapInterval(1);  // VSync ON
    }
}

// Helper interpolation
Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}
```

**Avantages de cette Approche:**

| Aspect | Bénéfice |
|--------|----------|
| **Logique** | Fixed 60 Hz = déterministe, reproductible |
| **Collision** | Hitboxes et positions synchronisées (pas de désync) |
| **IA** | État évalué aux mêmes timestamps (cohérent) |
| **Combat** | Frame-perfect timing garanti |
| **Rendu** | Fluide 120 FPS+ possible via interpolation |
| **Fluidité** | Même qualité delta time SANS risques |

**Implications:**
- Accumulator pattern (standard industrie, bien documenté)
- État double-buffered (previous/current) pour interpolation
- Rendu découplé logique (peut tourner 120 FPS sur écran haute fréquence)
- Multi-update si lag (normal dans arcade games, rattrapage automatique)
- **Absolument non-négociable pour fighting game professionnel**

**Comparaison Visuelle:**

```
Fixed sans interpolation (rigide):
Logic: |--60Hz--|--60Hz--|--60Hz--|
Render:|--60Hz--|--60Hz--|--60Hz--|
       Saccadé si écran 120Hz

Fixed avec interpolation (fluide):
Logic: |--60Hz--|--60Hz--|--60Hz--|
Render:|120Hz|120Hz|120Hz|120Hz|120Hz|120Hz|
       Fluide même sur écran haute fréquence
```

**Note Performance:** Interpolation = simple lerp mathématique (~10 cycles CPU), négligeable vs coût collision/IA.

---

### 4. Input System — Buffering Custom 2-3 Frames

**Décision:** Input buffer circulaire, fenêtre 33-50ms pour actions (2-3 frames FIXED_DT).

**Rationale:**

**Timing Window Problem (Fighting Games Standard):**

```
Timeline (ms):  0      16.67   33.34   50      66.67
Frames:         |---1---|---2---|---3---|---4---|
Attack startup:                   [==================]
                                  frame 2 startup 6 frames

Sans Buffer: Joueur doit appuyer EXACTLY frame 2 (16-33ms window)
Avec Buffer: Joueur peut appuyer frames 0-3 (50ms window)
```

**Architecture Input Buffer:**

```c
#define INPUT_BUFFER_SIZE 4  // 4 frames = ~67ms

typedef struct {
    uint8_t key[256];  // bitfield keys
    uint32_t timestamp_ms;
} InputFrame;

typedef struct {
    InputFrame frames[INPUT_BUFFER_SIZE];
    int head, tail;
} InputBuffer;

// Main loop
void update_game(float dt) {
    // 1. Lectura input SDL3 dans buffer
    input_buffer_push(&ibuf, sdl_keys_pressed);
    
    // 2. Combat logic check buffer
    if (should_parade()) {
        // Vérifier 2 frames AVANT attack:
        InputFrame* buffered = input_buffer_at(&ibuf, -2);
        if (buffered && buffered->key[PARADE]) {
            parade_success = true;  // Buffered input!
        }
    }
    
    // 3. IA aussi check input buffer
    // (pour égalité vs joueur humain)
}
```

**Implications:**
- **Input Feel** — Moins frustrant, plus forgiving
- **Compétitivité Standard** — SF6, Tekken, Granblue utilisent buffering
- **IA Égale** — IA n'a aussi que buffer accès, pas de cheating
- **Technical Debt Minimum** — Implementé proprement dès départ
- **Testable** — Buffer deterministic, peut être rejoué

**Buffering Rules:**
- Clavier input stocké chaque FIXED_DT
- Fenêtre recherche: up to 3 frames back
- Clear buffer après action lancée (prevent spam)
- IA access même buffer (égalité timing)

---

### 5. Config & Persistence — JSON

**Décision:** Fichiers configuraton JSON (settings, keybinds, high scores).

**Rationale:**
- **Structure** — Schema versionnée, évolutif
- **Human-Readable** — UTF-8, peut être édité texte
- **Professional** — Standard industrie pour config
- **Parseable** — Nombreuses libraries C disponibles

**Format JSON Config:**

```json
{
  "version": "1.0",
  "settings": {
    "difficulty": "normal",
    "rounds": 1,
    "fullscreen": false,
    "resolution": [1920, 1080],
    "vsync": true
  },
  "controls": {
    "move_left": "A",
    "move_right": "D",
    "attack": "J",
    "parry": "shift",
    "height_up": "W",
    "height_down": "S",
    "jump": "space"
  },
  "highscores": [
    {"player": "Hicham", "score": 2500, "timestamp": "2026-03-04T14:32:00Z"}
  ]
}
```

**Library Choix:** cJSON (public domain, simple, C89) ou jsmn (minimal, tokenizer).

**Recommendation:** cJSON pour facilité vs jsmn pour footprint minimum.

---

### 6. État Jeu — Structs Modulaires

**Décision:** Structs type-safe modulaires (Vec2, Stats, Character, etc.).

**Implications:**
- Code searchable (`typedef Vec2` → find all position usages)
- Type-safe → compiler catches bugs
- Pattern extension clear (new field = add to struct)
- Cache-friendly (contiguous memory)

**Exemple Structs:**

```c
// Primitives
typedef struct { float x, y; } Vec2;
typedef struct { float x, y, w, h; } Rect;

// Combat
typedef enum { HEIGHT_LOW=0, HEIGHT_MID=1, HEIGHT_HIGH=2 } EpeeHeight;

typedef struct {
    float base_range;
    float evolved_range;  // base + 10% per parry success
} Epee;

typedef struct {
    Vec2 pos;
    Vec2 vel;
    Epee sword;
    EpeeHeight current_height;
    float momentum;  // 0.0 to 1.0
    int parry_counter;
    int alive;  // bool equivalent
} Character;

typedef struct {
    Character player;
    Character ai;
    Arena* arena;
    int frame_count;
    float accumulator;
} GameState;
```

**Avantages:**
- Extensible sans breaking existing code
- Debuggable dans GDB/ImGui
- Testable unitairement (pass struct à fonction test)

---

### 7. Debug — ImGui Overlay + Debug Panel

**Décision:** ImGui intégré pour visualisation runtime (hitboxes, state IA, FPS).

**Rationale:**
- **Profiling Temps Réel** — Voir où CPU va (IA, physique, rendu)
- **State Inspection** — Visualiser player/ai pos, momentum, épée
- **Development Speed** — Debug visuel >> console logs
- **Professional Tools** — Standard industrie for game dev

**Debug Features Implémentées:**

```c
// Debug overlay (F11 toggle)
- FPS counter
- Frame time breakdown (update vs render)
- Player/AI position et velocity
- Hitbox visualization (rectangles SDL)
- IA decision tree display (current node, eval score)
- Momentum bar
- Épée height indicator

// Debug panel (ImGui window)
- Difficulty slider real-time
- Speed multiplier (0.5x to 2x)
- Freeze frame + step-frame-by-frame
- State export JSON (for replay)
- Arena seed display
- Input buffer content
```

**Implementation:**
- ImGui single-header library (imgui_sdl3_binding.h)
- Render post-game logic, pre-SDL present
- FPS cheap, always available
- Disable in release build via `#ifdef DEBUG`

---

### 8. Organisation Code — Structure Modulaire

**Décision:** Organisation physique source tree par concern, contrats d'interface IP1/IP2.

**Structure:**

```
blade-rush/
├── src/
│   ├── main.c                  # Entry point, boucle jeu principale
│   │
│   ├── engine/                 # Cœur SDL3
│   │   ├── engine.h/.c         # Init SDL3, cleanup
│   │   ├── render.h/.c         # SDL_Renderer API, draw sprites/rects
│   │   ├── input.h/.c          # SDL_Event polling, input buffer
│   │   ├── timing.h/.c         # Accumulator, fixed timestep loop
│   │   └── debug.h/.c          # ImGui integration, overlay debug
│   │
│   ├── combat/                 # Lane A — Gameplay (Hicham)
│   │   ├── character.h/.c      # Character structs, initialization
│   │   ├── physics.h/.c        # Gravité, mouvements, collisions
│   │   ├── attacks.h/.c        # Logique attaque, hit detection
│   │   ├── parry.h/.c          # Parade logic, height matching
│   │   └── momentum.h/.c       # Momentum système
│   │
│   ├── ai/                     # Lane B — Intelligence (Walid)
│   │   ├── minmax.h/.c         # Arbre MinMax + alpha-beta pruning
│   │   ├── evaluator.h/.c      # Fonction évaluation position
│   │   ├── state.h/.c          # Discrétisation state pour IA
│   │   ├── difficulty.h/.c     # Paramètres difficulté (profondeur, epsilon)
│   │   └── cache.h/.c          # Memoization MinMax tree
│   │
│   ├── arena/                  # Lane B — Génération (Walid)
│   │   ├── generator.h/.c      # Algorithme génération procédurale
│   │   ├── tiles.h/.c          # Enum types tiles, structures
│   │   ├── validator.h/.c      # Fairness checks (balance spawn, pas trop trous)
│   │   └── seed.h/.c           # PRNG seed-based, déterministe
│   │
│   ├── game/                   # Game state container
│   │   ├── game.h/.c           # GameState struct, high-level update
│   │   ├── config.h/.c         # Config loading JSON, defaults
│   │   └── state.h/.c          # Game mode state machine
│   │
│   └── utils/                  # Helpers generique
│       ├── memory.h/.c         # Allocation wrapper, audit
│       ├── math.h              # Vec2 ops, AABB functions
│       ├── constants.h         # Defines globales (FIXED_DT, etc)
│       └── logging.h/.c        # Printf-style logging avec timestamps
│
├── assets/
│   ├── sprites/                # PNG pixel art
│   │   ├── player_idle.png
│   │   ├── ai_idle.png
│   │   ├── effects.png
│   │   └── ui.png
│   ├── sfx/                    # OGG audio effects
│   └── music/                  # OGG background tracks
│
├── include/                    # External headers (SDL3)
├── Makefile                    # Build configuration
├── README.md                   # Documentation
└── .gitignore                  # C artifacts
```

**Principes Organisation:**
- **Separation of Concerns** — Chaque module responsible d'une thing
- **Lane A/B Parallel** — Lane A ne touch pas `ai/`, `arena/`
- **IP1/IP2 Contrats** — Interfaces définies en headers publics
- **No Circular Dependencies** — DAG de includes
- **Module Init/Cleanup** — Chaque module .c a `_init()`, `_cleanup()`

**Integration Points (IP1, IP2):**

```c
// IP1: Arena Interface (Lane B → Lane A)
// FILE: arena/generator.h
typedef struct Arena Arena;
Arena* arena_generate(uint32_t seed, ArenaSizeType size);
void arena_destroy(Arena* a);
const uint8_t* arena_get_tiles(Arena* a);
int arena_get_width(Arena* a), arena_get_height(Arena* a);

// IP2: IA Interface (Lane B → Lane A)
// FILE: ai/minmax.h
typedef struct AIState AIState;
AIState* ai_state_create(int difficulty);
Action ai_get_action(AIState* state, const GameState* game);
void ai_state_destroy(AIState* state);
```

**Compile Check at IP1/IP2:**
```bash
# Lane A compiles independently (with mock Arena, AIState stubs)
gcc src/engine/*.c src/combat/*.c src/game/*.c -o test_lane_a

# Lane B compiles independently
gcc src/ai/*.c src/arena/*.c -o test_lane_b

# Full integration
gcc src/**/*.c -o blade-rush
```

---

### Décisions Interdépendances

```
Fixed Timestep (3)
    ↓
Buffering Custom (4)   ←→ Input System (combat + ai identique)
    ↓                       ↓
Custom Physics (1) ←→ Combat Logic
    ↓
Malloc/Free (2) ←→ Arena Dynamic Allocation (arena/)
    ↓
Struct Modulaires (6) ←→ Type Safety across IP1/IP2
    ↓
Debug ImGui (7) ←→ State visualization (GameState inspection)
    ↓
JSON Config (5) ←→ Game settings, high scores
```

---
## Préoccupations Transversales (Cross-cutting Concerns)

Ces patterns s'appliquent à **TOUS les systèmes** et sont **obligatoires** pour tout code (Lane A, Lane B, tous modules). Ils garantissent cohérence et maintenabilité.

### 1. Error Handling Strategy

**Approche Professionnelle Hybride :** Return codes + Assertions + Logs + Result structs situationnels.

**Pattern Basé Pratiques Industrie (Linux Kernel, SDL3, SQLite) :**

```c
// Pattern 1: Return codes (fonctions critiques)
// Retourne: 0 = success, -1 = error
int arena_generate(Arena** out, uint32_t seed, ArenaSizeType size) {
    *out = malloc(sizeof(Arena));
    if (!*out) {
        log_error("arena.c: malloc failed for Arena");
        return -1;  // Critical error
    }
    
    (*out)->tiles = calloc(width * height, sizeof(uint8_t));
    if (!(*out)->tiles) {
        log_error("arena.c: malloc failed for tiles");
        free(*out);
        *out = NULL;
        return -1;  // Critical error, cleanup done
    }
    
    return 0;  // Success
}

// Pattern 2: Result struct (fonctions avec fallback)
typedef struct {
    int success;       // 0 = unrecoverable, 1 = ok (ou recovered)
    void* data;        // Données si success
    const char* error; // Message si échec non-récupéré
} Result;

Result texture_load(const char* path) {
    Result r = {0};
    SDL_Surface* surf = IMG_Load(path);
    
    if (!surf) {
        log_warn("texture: %s not found, using placeholder", path);
        r.data = load_placeholder_texture();  // Fallback
        r.success = 1;  // Recovered
        r.error = NULL;
    } else {
        r.data = surf;
        r.success = 1;
        r.error = NULL;
    }
    
    return r;
}

// Pattern 3: Assertions (bugs programmation, dev only)
void character_update(Character* c, float dt) {
    assert(c != NULL);  // Invariant: jamais NULL
    assert(dt > 0.0f && dt < 1.0f);  // Invariant: dt raisonnable
    
    // ... logique
}

// Pattern 4: Logs (errors runtime)
if (config_load("config.json") != 0) {
    log_error("config.c: failed to load config.json");
    // Utilise defaults
}
```

**Gestion Erreurs Spécifiques Blade Rush :**

| Erreur | Type | Stratégie | Rationale |
|--------|------|-----------|-----------|
| **Malloc fail** | Critique | Quitter proprement avec dialog | Pas de mémoire = impossible continuer |
| **Texture load fail** | Récupérable | Fallback placeholder (carré coloré) | Jeu fonctionnel visuellement dégradé |
| **IA MinMax timeout** | Récupérable | Action par défaut safe (reculer + parade) | Évite freeze, survivabilité garantie |
| **Config JSON parse fail** | Récupérable | Defaults hardcodés | Jeu fonctionne avec settings par défaut |
| **Arena generation fail** | Critique | Retry 3x puis quitter | Map nécessaire pour jouer |
| **SDL3 init fail** | Critique | Dialog puis exit(1) | Pas de rendu = impossible jouer |

**Error Display au Joueur :**

```c
// Fonction dialog erreur critique
void show_error_dialog(const char* title, const char* message) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        title,
        message,
        NULL
    );
    log_error("FATAL: %s - %s", title, message);
}

// Usage
if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    show_error_dialog(
        "Blade Rush - Initialization Failed",
        "Cannot initialize SDL3. Check graphics drivers."
    );
    return 1;
}
```

**Règles Obligatoires :**
1. ✅ Toutes fonctions publiques retournent int (0=ok, -1=error) ou Result
2. ✅ Assertions pour invariants (compiled out en release avec -DNDEBUG)
3. ✅ Logs ERROR pour runtime failures
4. ✅ Cleanup systématique sur erreurs (free memory, close files)
5. ✅ Pas de global error state (risque confusion multi-context)

---

### 2. Logging Strategy — Format Professionnel Industrie

**Format Unifié (Obligatoire) :**

```
[YYYY-MM-DD HH:MM:SS][LEVEL][module.c] Message
[2026-03-04 16:31:22][INFO][arena.c] Arena generated (seed=123456)
[2026-03-04 16:31:22][ERROR][memory.c] malloc failed: out of memory
[2026-03-04 16:31:23][DEBUG][ai.c] MinMax depth=3, eval=0.75, action=ATTACK
[2026-03-04 16:31:23][TRACE][physics.c] Collision: player@(120.5, 80.0) vs ai@(125.0, 80.0)
```

**Niveaux de Log :**

| Niveau | Usage | Build | Destination |
|--------|-------|-------|-------------|
| **ERROR** | Erreurs critiques, échecs non-récupérables | Toujours | Console + Fichier |
| **WARN** | Warnings, situations anormales mais gérées | Toujours | Console + Fichier |
| **INFO** | Milestones normaux (init, matches, génération) | Toujours | Console + Fichier |
| **DEBUG** | Diagnostics détaillés (état IA, collisions) | Debug build | Console + Fichier |
| **TRACE** | Extremely verbose (chaque frame, positions) | Debug build | Fichier uniquement |

**Implémentation :**

```c
// utils/logging.h
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3,
    LOG_TRACE = 4
} LogLevel;

// Initialisation logging
int log_init(const char* log_file_path);  // "blade-rush.log"
void log_shutdown(void);

// Fonction principale
void log_message(LogLevel level, const char* module, const char* fmt, ...);

// Macros convenientes (capture __FILE__ automatiquement)
#define log_error(fmt, ...) log_message(LOG_ERROR, __FILE__, fmt, ##__VA_ARGS__)
#define log_warn(fmt, ...)  log_message(LOG_WARN, __FILE__, fmt, ##__VA_ARGS__)
#define log_info(fmt, ...)  log_message(LOG_INFO, __FILE__, fmt, ##__VA_ARGS__)

// DEBUG et TRACE compiled out en release
#ifdef DEBUG
    #define log_debug(fmt, ...) log_message(LOG_DEBUG, __FILE__, fmt, ##__VA_ARGS__)
    #define log_trace(fmt, ...) log_message(LOG_TRACE, __FILE__, fmt, ##__VA_ARGS__)
#else
    #define log_debug(fmt, ...) ((void)0)  // Noop
    #define log_trace(fmt, ...) ((void)0)  // Noop
#endif
```

**Implémentation Interne :**

```c
// utils/logging.c
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static FILE* log_file = NULL;
static LogLevel min_level = LOG_INFO;  // Niveau minimum en release

int log_init(const char* path) {
    log_file = fopen(path, "a");  // Append mode
    if (!log_file) {
        fprintf(stderr, "WARNING: Cannot open log file %s\n", path);
        return -1;
    }
    
#ifdef DEBUG
    min_level = LOG_TRACE;  // Tout en debug
#endif
    
    log_info("=== Blade Rush Started ===");
    return 0;
}

void log_shutdown(void) {
    if (log_file) {
        log_info("=== Blade Rush Shutdown ===");
        fclose(log_file);
        log_file = NULL;
    }
}

void log_message(LogLevel level, const char* module, const char* fmt, ...) {
    if (level > min_level) return;  // Filtrage niveau
    
    // Timestamp
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    // Level string
    const char* level_str[] = {"ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
    
    // Extract filename from full path
    const char* filename = strrchr(module, '/');
    if (!filename) filename = module;
    else filename++;
    
    // Format message
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    // Output to console
    if (level <= LOG_INFO) {  // ERROR, WARN, INFO → console
        fprintf(stdout, "[%s][%s][%s] %s\n", 
                timestamp, level_str[level], filename, buffer);
        fflush(stdout);
    }
    
    // Output to file (tous niveaux)
    if (log_file) {
        fprintf(log_file, "[%s][%s][%s] %s\n", 
                timestamp, level_str[level], filename, buffer);
        fflush(log_file);  // Force write (important si crash)
    }
}
```

**Usage Exemples :**

```c
// main.c
log_info("Blade Rush v1.0 starting...");
log_info("Platform: %s", SDL_GetPlatform());

// arena.c
log_info("Arena generated (seed=%u, size=%dx%d)", seed, width, height);
log_debug("Arena validation: gaps=%d, platforms=%d", gaps, platforms);

// ai.c
log_debug("MinMax: depth=%d, alpha=%.2f, beta=%.2f, eval=%.3f", 
          depth, alpha, beta, eval_score);
log_trace("MinMax: exploring node children=%d", child_count);

// memory.c
if (ptr == NULL) {
    log_error("malloc failed: requested %zu bytes", size);
}
```

**Destinations :**

1. **Console (stdout)** — Développement temps réel
   - ERROR, WARN, INFO affichés
   - DEBUG, TRACE silencieux (pas de spam console)

2. **Fichier blade-rush.log** — Post-mortem debugging
   - Tous niveaux écrits
   - Append mode (historique multi-sessions)
   - Flush systématique (survit aux crashes)

**Rotation Log (Optionnel MVP+) :**
```c
// Si log > 10 MB, renommer blade-rush.log → blade-rush.old.log
#define MAX_LOG_SIZE (10 * 1024 * 1024)  // 10 MB
```

---

### 3. Configuration Management

**Structure Adoptée :** JSON runtime + Constants hardcodés.

**Fichiers Configuration :**

```
config.json           # User settings + balancing + IA params
constants.h           # Valeurs compile-time (FIXED_DT, hitbox sizes)
```

**config.json Structure :**

```json
{
  "version": "1.0",
  
  "gameplay": {
    "momentum_gain_rate": 0.05,
    "momentum_max": 1.0,
    "sword_evolve_threshold": 3,
    "sword_evolve_bonus": 0.10,
    "parry_window_frames": 2
  },
  
  "ai": {
    "easy": {
      "minmax_depth": 2,
      "epsilon": 0.3,
      "reaction_delay_ms": 200
    },
    "normal": {
      "minmax_depth": 3,
      "epsilon": 0.1,
      "reaction_delay_ms": 100
    },
    "hard": {
      "minmax_depth": 4,
      "epsilon": 0.05,
      "reaction_delay_ms": 50
    },
    "very_hard": {
      "minmax_depth": 5,
      "epsilon": 0.0,
      "reaction_delay_ms": 0
    }
  },
  
  "user": {
    "resolution": [1920, 1080],
    "fullscreen": false,
    "vsync": true,
    "master_volume": 0.8,
    "music_volume": 0.6,
    "sfx_volume": 1.0,
    
    "controls": {
      "move_left": "A",
      "move_right": "D",
      "attack": "J",
      "parry": "LeftShift",
      "height_up": "W",
      "height_down": "S",
      "jump": "Space"
    }
  }
}
```

**constants.h (Valeurs Compile-Time) :**

```c
// constants.h
#ifndef CONSTANTS_H
#define CONSTANTS_H

// Timing
#define FIXED_DT (1.0f / 60.0f)  // 16.67ms
#define FIXED_DT_MS (FIXED_DT * 1000.0f)

// Gameplay Physics
#define GRAVITY 980.0f  // pixels/s²
#define PLAYER_BASE_SPEED 200.0f  // pixels/s
#define PLAYER_JUMP_VELOCITY -400.0f  // pixels/s (up)

// Hitboxes
#define PLAYER_WIDTH 32.0f
#define PLAYER_HEIGHT 64.0f
#define SWORD_BASE_RANGE 48.0f

// Input Buffer
#define INPUT_BUFFER_SIZE 4  // 4 frames = ~67ms

// Logging
#define LOG_FILE_PATH "blade-rush.log"
#define MAX_LOG_SIZE (10 * 1024 * 1024)  // 10 MB

#endif
```

**Chargement Config :**

```c
// game/config.h
typedef struct {
    // Gameplay
    float momentum_gain_rate;
    float momentum_max;
    int sword_evolve_threshold;
    float sword_evolve_bonus;
    
    // IA (par difficulté)
    struct {
        int minmax_depth;
        float epsilon;
        int reaction_delay_ms;
    } ai[4];  // easy, normal, hard, very_hard
    
    // User
    int resolution[2];
    int fullscreen;
    int vsync;
    float volumes[3];  // master, music, sfx
    
    // Controls (SDL_Scancode)
    SDL_Scancode controls[8];
} GameConfig;

int config_load(const char* path, GameConfig* out);
int config_save(const char* path, const GameConfig* cfg);
void config_set_defaults(GameConfig* cfg);
```

**Fallback Defaults (Erreur Load) :**

```c
void config_set_defaults(GameConfig* cfg) {
    cfg->momentum_gain_rate = 0.05f;
    cfg->momentum_max = 1.0f;
    cfg->sword_evolve_threshold = 3;
    cfg->sword_evolve_bonus = 0.10f;
    
    // IA defaults
    cfg->ai[EASY].minmax_depth = 2;
    cfg->ai[NORMAL].minmax_depth = 3;
    // ... etc
    
    log_warn("config: using default settings");
}
```

**Usage Library JSON :** cJSON (public domain, simple).

---

### 4. Event/Signal System

**Approche pour Blade Rush :** Minimaliste — Direct calls + Callbacks optionnels.

**Rationale :**
- Blade Rush = jeu simple, pas de systèmes async complexes
- Pas de UI événementielle (menu simple)
- Pas de networking (pas d'events distants)
- **Direct calls suffisent** pour la majorité

**Pattern Callback Situationnel (où nécessaire) :**

```c
// Événement: Character Death
typedef void (*OnCharacterDeath_fn)(Character* who, Character* killer);

// Game state stocke callbacks optionnels
typedef struct {
    OnCharacterDeath_fn on_character_death;
    // Ajoutez si besoin: on_round_end, on_arena_generated, etc.
} GameCallbacks;

// Usage dans combat logic
void handle_character_death(Character* dead, Character* killer) {
    log_info("Character death: %s killed by %s", 
             dead->name, killer->name);
    
    // Callback si défini
    if (game.callbacks.on_character_death) {
        game.callbacks.on_character_death(dead, killer);
    }
    
    // Update scores, respawn, etc.
    update_score(killer);
    respawn_character(dead);
}
```

**Pas de Event Queue Complex** — Overhead inutile pour Blade Rush.

**Règle :** Appels directs partout sauf callbacks explicites où découplage nécessaire.

---

### 5. Debug & Development Tools

**Outils Debug Intégrés (Compilés avec -DDEBUG) :**

**A. Overlay Debug Visuel (Toggle F11) :**

```c
// engine/debug.c
void render_debug_overlay(GameState* game) {
    #ifdef DEBUG
    if (!debug_overlay_enabled) return;
    
    // 1. FPS Counter (top-left)
    char fps_text[32];
    snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", current_fps);
    render_text(10, 10, fps_text, COLOR_GREEN);
    
    // 2. Frame Time Graph (top-right)
    render_frame_time_graph(frame_times_history, 60);
    
    // 3. Hitboxes Visualization
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 128);  // Red
    SDL_RenderDrawRect(renderer, &player.hitbox);
    SDL_RenderDrawRect(renderer, &ai.hitbox);
    
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 128);  // Green
    SDL_RenderDrawRect(renderer, &player.sword_hitbox);
    SDL_RenderDrawRect(renderer, &ai.sword_hitbox);
    
    // 4. Position & Velocity Display
    char pos_text[128];
    snprintf(pos_text, sizeof(pos_text), 
             "Player: (%.1f, %.1f) vel:(%.1f, %.1f) momentum:%.2f",
             player.pos.x, player.pos.y, player.vel.x, player.vel.y, player.momentum);
    render_text(10, 40, pos_text, COLOR_YELLOW);
    
    // 5. IA State Display
    snprintf(pos_text, sizeof(pos_text), 
             "AI: action=%s eval=%.2f depth=%d",
             action_names[ai_current_action], ai_eval_score, ai_search_depth);
    render_text(10, 60, pos_text, COLOR_CYAN);
    
    // 6. Input Buffer Visualization
    render_input_buffer(&input_buffer);
    
    // 7. Collision Markers
    for (int i = 0; i < collision_count; i++) {
        SDL_RenderDrawPoint(renderer, collisions[i].x, collisions[i].y);
    }
    #endif
}
```

**B. ImGui Debug Panel (Toggle F12) :**

```c
// engine/debug_ui.c (ImGui integration)
void render_debug_panel(GameState* game) {
    #ifdef DEBUG
    if (!debug_panel_enabled) return;
    
    ImGui::Begin("Blade Rush Debug Panel");
    
    // Sliders runtime tweaking
    ImGui::SliderFloat("Momentum Rate", &game->config.momentum_gain_rate, 0.0f, 0.2f);
    ImGui::SliderInt("Sword Evolve", &game->config.sword_evolve_threshold, 1, 10);
    ImGui::SliderInt("AI Difficulty", &current_ai_difficulty, 0, 3);
    ImGui::SliderFloat("Game Speed", &game_speed_multiplier, 0.1f, 2.0f);
    
    // Buttons
    if (ImGui::Button("Reset Game")) {
        reset_game_state();
    }
    if (ImGui::Button("Regenerate Arena")) {
        regenerate_arena_new_seed();
    }
    if (ImGui::Button("Freeze Frame")) {
        game_paused = !game_paused;
    }
    if (ImGui::Button("Step Frame")) {
        step_single_frame();
    }
    
    // Logs Viewer (derniers 100 messages)
    ImGui::BeginChild("Logs", ImVec2(0, 200), true);
    for (int i = 0; i < log_history_count; i++) {
        ImGui::TextUnformatted(log_history[i]);
    }
    ImGui::EndChild();
    
    // Memory Stats
    ImGui::Text("Memory: %zu allocations active, %zu bytes", 
                active_allocs, total_bytes_allocated);
    
    // Arena Info
    ImGui::Text("Arena Seed: %u", current_arena_seed);
    
    ImGui::End();
    #endif
}
```

**C. Cheats Dev (Touches rapides, DEBUG only) :**

```c
// engine/debug.c
void handle_debug_keys(SDL_Event* e) {
    #ifdef DEBUG
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
            case SDLK_F1:
                player_invincible = !player_invincible;
                log_debug("Invincibility: %s", player_invincible ? "ON" : "OFF");
                break;
            case SDLK_F2:
                instant_win_round();
                log_debug("Instant win triggered");
                break;
            case SDLK_F3:
                regenerate_arena_new_seed();
                log_debug("Arena regenerated");
                break;
            case SDLK_F4:
                game_speed_multiplier = (game_speed_multiplier == 1.0f) ? 0.5f : 1.0f;
                log_debug("Slow motion: %.1fx", game_speed_multiplier);
                break;
        }
    }
    #endif
}
```

**Activation/Désactivation :**

```bash
# Build Debug (tous outils actifs)
make DEBUG=1

# Build Release (aucun debug, optimisé)
make
```

```c
// Makefile
ifdef DEBUG
    CFLAGS += -DDEBUG -g -O0
else
    CFLAGS += -DNDEBUG -O2
endif
```

**Outils Inclus :**

| Outil | Touche | Description |
|-------|--------|-------------|
| Debug Overlay | F11 | FPS, hitboxes, positions, IA state |
| Debug Panel | F12 | ImGui sliders, buttons, logs viewer |
| Invincibility | F1 | Player cannot die |
| Instant Win | F2 | Win round immédiatement |
| Regen Arena | F3 | Nouveau seed arena |
| Slow Motion | F4 | Toggle 0.5x speed |

**Release Build :** Tous ces outils compiled-out (macros `#ifdef DEBUG`), zéro overhead.

---

## Structure du Projet

### Pattern d'Organisation

**Pattern Adopté :** Domain-Driven (par système de jeu) avec frontières strictes.

**Rationale :**
- Séparation Lane A/B claire (combat/ vs ai/arena/)
- Contrats IP1/IP2 via headers publics minimalistes
- Parallélisable (Hicham/Walid sans conflits merge)
- Module `engine/` générique réutilisable

### Structure Complète Répertoires & Fichiers

```
blade-rush/
├── src/
│   ├── main.c                      # Entry point, boucle principale
│   │
│   ├── engine/                     # Cœur SDL3 (GÉNÉRIQUE, réutilisable)
│   │   ├── engine.h / engine.c     # Init/shutdown SDL3
│   │   ├── render.h / render.c     # SDL_Renderer, draw sprites/primitives
│   │   ├── input.h / input.c       # SDL_Event, input buffer
│   │   ├── timing.h / timing.c     # Fixed timestep accumulator
│   │   ├── audio.h / audio.c       # SDL_mixer, SFX/music
│   │   └── debug.h / debug.c       # ImGui overlay, debug panel
│   │
│   ├── combat/                     # Lane A — Gameplay (Hicham)
│   │   ├── character.h / character.c   # Character structs, update
│   │   ├── physics.h / physics.c       # Gravité, mouvement, AABB
│   │   ├── attacks.h / attacks.c       # Attaque, hit detection
│   │   ├── parry.h / parry.c           # Parade, height matching
│   │   ├── momentum.h / momentum.c     # Momentum gain/loss
│   │   └── sword.h / sword.c           # Épée évolutive, range
│   │
│   ├── ai/                         # Lane B — Intelligence (Walid)
│   │   ├── minmax.h / minmax.c         # Arbre MinMax + alpha-beta
│   │   ├── evaluator.h / evaluator.c   # Fonction eval position
│   │   ├── state.h / state.c           # Discrétisation GameState
│   │   ├── difficulty.h / difficulty.c # Params difficulté
│   │   └── cache.h / cache.c           # Memoization MinMax
│   │
│   ├── arena/                      # Lane B — Génération (Walid)
│   │   ├── generator.h / generator.c   # Algo procédural (IP1)
│   │   ├── tiles.h / tiles.c           # Tiles privés, rendering
│   │   ├── validator.h / validator.c   # Fairness checks
│   │   └── seed.h / seed.c             # PRNG déterministe
│   │
│   ├── game/                       # État jeu, config, UI
│   │   ├── game.h / game.c             # GameState, update
│   │   ├── config.h / config.c         # Config JSON (cJSON)
│   │   ├── menu.h / menu.c             # Menu principal
│   │   ├── hud.h / hud.c               # HUD scores/postures
│   │   └── match.h / match.c           # Match flow, rounds
│   │
│   └── utils/                      # Helpers génériques
│       ├── memory.h / memory.c         # Malloc tracking
│       ├── math.h / math.c             # Vec2, lerp, AABB
│       ├── logging.h / logging.c       # Log unifié
│       └── constants.h                 # Defines globales
│
├── assets/
│   ├── sprites/
│   │   ├── characters/
│   │   │   ├── player_idle.png, player_walk.png, player_attack_*.png
│   │   │   ├── player_parry.png, player_death.png
│   │   │   └── ai_*.png (même states)
│   │   ├── effects/
│   │   │   ├── slash.png, sparks.png, dust.png
│   │   ├── arena/
│   │   │   ├── tile_ground.png, tile_platform.png, tile_hole.png
│   │   │   └── background.png
│   │   └── ui/
│   │       ├── hud_health.png, icon_sword_*.png, menu_button.png
│   │
│   ├── sfx/
│   │   ├── attack_whoosh.ogg, parry_clang.ogg, hit_impact.ogg
│   │   ├── death.ogg, respawn.ogg, footstep.ogg, jump.ogg
│   │   └── menu_select.ogg, menu_confirm.ogg
│   │
│   └── music/
│       ├── menu_theme.ogg, combat_theme.ogg, victory_theme.ogg
│
├── include/                        # Headers externes (SDL3, cJSON, ImGui)
├── build/                          # Objets compilés (.o)
├── bin/                            # Exécutable final (blade-rush)
├── docs/                           # Documentation (architecture.md, api.md)
├── tests/                          # Tests unitaires (optionnel MVP)
├── Makefile
├── .gitignore
├── README.md
├── config.json                     # Généré 1ère exec
└── blade-rush.log                  # Généré runtime
```

### Mapping Systèmes → Emplacements

| Système | Emplacement | Responsabilité | Lane | Dépendances |
|---------|-------------|----------------|------|-------------|
| **Rendu SDL3** | engine/render.c | Primitives graphiques génériques | Shared | utils/, SDL3 |
| **Input Buffer** | engine/input.c | Buffer circulaire inputs | Shared | utils/, SDL3 |
| **Timing** | engine/timing.c | Fixed timestep accumulator | Shared | utils/, SDL3 |
| **Audio** | engine/audio.c | SFX/music playback | Shared | utils/, SDL3_mixer |
| **Debug Tools** | engine/debug.c | ImGui overlay | Shared | utils/, ImGui |
| **Physique 2D** | combat/physics.c | Gravité, collisions | Lane A | engine/, arena/ (via IP1) |
| **Attaque/Parade** | combat/attacks.c, parry.c | Hit detection, matching | Lane A | engine/, utils/ |
| **Momentum** | combat/momentum.c | Vitesse modifiers | Lane A | utils/ |
| **Épée Évolutive** | combat/sword.c | Range evolution | Lane A | utils/ |
| **Characters** | combat/character.c | Player/AI update | Lane A | engine/, arena/ (via IP1) |
| **IA MinMax** | ai/minmax.c | Arbre recherche | Lane B | game/ (via forward) |
| **Evaluator** | ai/evaluator.c | Eval position | Lane B | utils/ |
| **Discrétisation** | ai/state.c | GameState → AIState | Lane B | game/ (via forward) |
| **Difficulté** | ai/difficulty.c | Profondeur, epsilon | Lane B | - |
| **Cache IA** | ai/cache.c | Memoization | Lane B | utils/ |
| **Génération Arena** | arena/generator.c | Algorithme procédural (IP1) | Lane B | utils/ |
| **Tiles** | arena/tiles.c | Implémentation privée | Lane B | utils/ |
| **Validator** | arena/validator.c | Fairness | Lane B | utils/ |
| **Game State** | game/game.c | État global high-level | Shared | engine/, combat/, ai/, arena/ |
| **Config** | game/config.c | JSON load/save | Shared | utils/, cJSON |
| **Menu/HUD** | game/menu.c, hud.c | UI jeu | Shared | engine/ |
| **Match Flow** | game/match.c | Rounds, victoire | Shared | combat/, game/ |
| **Logging** | utils/logging.c | Format unifié | Shared | - |
| **Math** | utils/math.c | Vec2, AABB | Shared | - |
| **Memory** | utils/memory.c | Tracking malloc | Shared | - |

### Conventions de Nommage

#### Fichiers et Répertoires

| Type | Convention | Exemple |
|------|------------|---------|
| Source C | snake_case.c | `physics.c`, `minmax.c` |
| Headers | snake_case.h | `character.h`, `arena.h` |
| Répertoires | snake_case/ | `combat/`, `ai/`, `engine/` |
| Assets | snake_case_descriptif.ext | `player_idle.png`, `attack_whoosh.ogg` |

#### Éléments Code C

| Élément | Convention | Exemple |
|---------|------------|---------|
| Structs/Typedefs | PascalCase | `Character`, `Vec2`, `GameState` |
| Fonctions | snake_case | `arena_generate()`, `character_update()` |
| Variables | snake_case | `player_pos`, `current_momentum` |
| Constantes | UPPER_SNAKE | `FIXED_DT`, `PLAYER_BASE_SPEED` |
| Enum Values | UPPER_SNAKE | `HEIGHT_LOW`, `ACTION_ATTACK` |
| Macros | UPPER_SNAKE | `LOG_ERROR`, `DEBUG` |

#### Nommage Spécifique

| Type | Convention | Exemple |
|------|------------|---------|
| Fonctions Publiques | `module_action()` | `arena_generate()`, `ai_get_action()` |
| Fonctions Privées | `static internal_action()` | `static minmax_recursive()` |
| Init/Cleanup | `module_init()`, `module_cleanup()` | `engine_init()`, `game_cleanup()` |
| Structs Opaques | Forward declare dans .h | `typedef struct Arena Arena;` |
| Guards Headers | `MODULE_H` | `#ifndef PHYSICS_H` |

### Frontières Architecturales Strictes

#### 1. Isolation Module `engine/` — Générique & Réutilisable

**Règle Absolue :** `engine/` = couche SDL3 pure, **ZÉRO connaissance** du jeu.

**Interdictions Strictes :**

```c
// ❌ INTERDIT dans engine/*.c ou engine/*.h
#include "game/game.h"
#include "combat/character.h"
#include "ai/minmax.h"
#include "arena/generator.h"

// ✅ AUTORISÉ dans engine/*.c
#include <SDL3/SDL.h>
#include "utils/math.h"      // OK: utils = générique
#include "utils/logging.h"   // OK: utils = générique
```

**Interface Engine Agnostique :**

```c
// engine/render.h - Aucune référence jeu
typedef struct Renderer Renderer;

Renderer* renderer_create(SDL_Window* window);
void renderer_draw_sprite(Renderer* r, SDL_Texture* tex, Vec2 pos, Vec2 size);
void renderer_draw_rect(Renderer* r, Rect bounds, Color color);
void renderer_present(Renderer* r);
void renderer_destroy(Renderer* r);

// Pas de "Character", "Arena", "AI" dans engine/
```

**Flux Dépendances :**

```
game/ → engine/  ✅ OK: game utilise engine
engine/ → game/  ❌ INTERDIT: casse réutilisabilité
```

**Vérification :** `engine/` doit pouvoir être extrait dans un autre projet sans modifications.

---

#### 2. Interface `arena/` → `combat/` — API Abstraite (IP1)

**Problème à Éviter : Fuite d'Implémentation**

```c
// ❌ INTERDIT (exposition détails internes)
// arena/tiles.h
extern Tile tiles[WIDTH][HEIGHT];  // Accès direct = couplage

// combat/physics.c
#include "arena/tiles.h"
if (tiles[x][y] == TILE_HOLE) { ... }  // Accès direct arrays
```

**Solution Professionnelle : Query API (IP1)**

```c
// arena/generator.h - Header public IP1 (Lane B → Lane A)
typedef struct Arena Arena;  // Opaque, détails cachés

// API Query Abstraite
Arena* arena_generate(uint32_t seed, ArenaSizeType size);
void arena_destroy(Arena* a);

bool arena_is_solid(const Arena* a, int x, int y);
bool arena_is_hole(const Arena* a, int x, int y);
bool arena_is_platform(const Arena* a, int x, int y);
int arena_get_width(const Arena* a);
int arena_get_height(const Arena* a);
Vec2 arena_get_spawn_left(const Arena* a);
Vec2 arena_get_spawn_right(const Arena* a);

// arena/tiles.c - Implémentation PRIVÉE (pas dans .h)
struct Arena {
    int width, height;
    Tile* tiles;  // Privé, jamais exposé
    Vec2 spawn_left, spawn_right;
};

bool arena_is_solid(const Arena* a, int x, int y) {
    // Implémentation interne, peut changer sans casser combat/
    if (x < 0 || x >= a->width) return false;
    return a->tiles[y * a->width + x] == TILE_GROUND;
}
```

**Usage Lane A (combat/physics.c) :**

```c
// combat/physics.c
#include "arena/generator.h"  // Uniquement header public

void physics_update(Character* c, const Arena* arena, float dt) {
    // Via API abstraite uniquement
    if (arena_is_hole(arena, (int)c->pos.x, (int)(c->pos.y + 1))) {
        c->falling = true;
    }
    // Jamais accès direct c->arena->tiles
}
```

**Avantages :**
- ✅ Lane B peut changer représentation tiles sans casser Lane A
- ✅ Tests unitaires physics.c avec mock Arena
- ✅ Optimisations cache transparentes

---

#### 3. Headers Publics IP1/IP2 — Zéro Dépendances Circulaires

**Règle : Headers minimalistes, forward declarations**

```c
// ✅ CORRECT: combat/character.h (header public)
#ifndef CHARACTER_H
#define CHARACTER_H

#include "utils/math.h"  // OK: utils générique

// Forward declarations (pas d'includes modules)
typedef struct Arena Arena;      // Défini dans arena/
typedef struct AIState AIState;  // Défini dans ai/

typedef struct {
    Vec2 pos, vel;
    float momentum;
    int height;  // 0=low, 1=mid, 2=high
} Character;

void character_init(Character* c, Vec2 spawn_pos);
void character_update(Character* c, const Arena* arena, float dt);

#endif
```

**Interdictions Strictes :**

```c
// ❌ INTERDIT: Dépendances circulaires

// combat/character.h
#include "ai/state.h"  // ❌ combat dépend ai

// ai/minmax.h
#include "combat/attacks.h"  // ❌ ai dépend combat

// arena/generator.h
#include "combat/physics.h"  // ❌ arena dépend combat
```

**Pattern Header Public (IP2 - IA) :**

```c
// ai/minmax.h - Header public IP2 (Lane B → Game)
#ifndef MINMAX_H
#define MINMAX_H

// Forward declaration uniquement
typedef struct GameState GameState;

typedef enum {
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_ATTACK,
    ACTION_PARRY,
    ACTION_CHANGE_HEIGHT
} Action;

typedef struct AIState AIState;  // Opaque

AIState* ai_state_create(int difficulty);
void ai_state_destroy(AIState* state);
Action ai_get_action(AIState* state, const GameState* game);

#endif
```

**Graphe Dépendances Autorisé :**

```
Couche Bottom:  utils/           (pas de dépendances)
                  ↓
Couche Core:    engine/          (→ utils/ uniquement)
                  ↓
Couche Domain:  combat/, ai/, arena/  (→ engine/, utils/)
                  ↓ (via forwards)
Couche Top:     game/            (→ tous modules via includes)
                  ↓
Entry Point:    main.c           (→ game/)
```

**Vérification Automatisée (Makefile) :**

```makefile
# Target: vérifier frontières architecturales
check-architecture:
	@echo "Checking engine/ isolation..."
	@! grep -rn "#include \"game/" src/engine/ && echo "✅ engine clean" || \
	   (echo "❌ engine depends on game!"; exit 1)
	@! grep -rn "#include \"combat/" src/engine/ || \
	   (echo "❌ engine depends on combat!"; exit 1)
	@! grep -rn "#include \"ai/" src/engine/ || \
	   (echo "❌ engine depends on ai!"; exit 1)
	
	@echo "Checking circular dependencies..."
	@! grep -rn "#include \"ai/" src/combat/ || \
	   (echo "❌ combat → ai circular!"; exit 1)
	@! grep -rn "#include \"combat/" src/ai/ || \
	   (echo "❌ ai → combat circular!"; exit 1)
	@! grep -rn "#include \"combat/" src/arena/ || \
	   (echo "❌ arena → combat dependency!"; exit 1)
	
	@echo "✅ Architecture boundaries respected"

.PHONY: check-architecture
```

**Intégration CI :**
```bash
# Pre-commit hook
make check-architecture || exit 1
```

---

#### 4. Règles Lane A/B (Parallélisation)

| Règle | Description | Vérification |
|-------|-------------|--------------|
| **Lane Isolation** | Lane A (`combat/`) ne modifie jamais Lane B (`ai/`, `arena/`) | Git branch protection |
| **IP Read-Only** | Lane A consomme `arena.h`, `minmax.h` en read-only | Code review obligatoire |
| **Shared Coordination** | `engine/`, `game/`, `utils/` = communication requise | PR approval 2+ |
| **No Circular Includes** | Headers publics = forward declarations uniquement | `make check-architecture` |
| **Compilation Indépendante** | `combat/` compile sans `ai/`, et vice-versa | CI jobs séparés |

**Test Compilation Indépendante :**

```makefile
# Vérifier Lane A compile sans Lane B
test-lane-a:
	gcc -c src/combat/*.c src/engine/*.c src/utils/*.c -I include/

# Vérifier Lane B compile sans Lane A
test-lane-b:
	gcc -c src/ai/*.c src/arena/*.c src/utils/*.c -I include/
```

---

### Ordre Compilation & Linking

```makefile
# Ordre de compilation (bottom-up)
OBJS = \
	build/utils/math.o \
	build/utils/logging.o \
	build/utils/memory.o \
	build/engine/render.o \
	build/engine/input.o \
	build/engine/timing.o \
	build/engine/audio.o \
	build/engine/debug.o \
	build/combat/character.o \
	build/combat/physics.o \
	build/combat/attacks.o \
	build/combat/parry.o \
	build/combat/momentum.o \
	build/combat/sword.o \
	build/ai/minmax.o \
	build/ai/evaluator.o \
	build/ai/state.o \
	build/ai/difficulty.o \
	build/ai/cache.o \
	build/arena/generator.o \
	build/arena/tiles.o \
	build/arena/validator.o \
	build/arena/seed.o \
	build/game/game.o \
	build/game/config.o \
	build/game/menu.o \
	build/game/hud.o \
	build/game/match.o \
	build/main.o

blade-rush: $(OBJS)
	gcc $(OBJS) -o bin/blade-rush $(LDFLAGS)
```

---

**Résumé des Frontières Critiques :**

| Frontière | Règle | Vérification |
|-----------|-------|--------------|
| **engine/ isolation** | Zéro include game-specific | `make check-architecture` |
| **arena/ → combat/ (IP1)** | Query API, pas accès direct tiles | Code review |
| **ai/ → game/ (IP2)** | Forward declarations, opaque structs | Headers minimalistes |
| **Lane A ↔ Lane B** | Aucun include croisé combat ↔ ai/arena | CI pre-merge |
| **Headers publics** | Forward declares, pas circular includes | Compilation warnings |

---

## Implementation Patterns

Ces patterns garantissent une implémentation cohérente entre tous les agents IA, tout en respectant les contraintes 60 FPS, low-latency input et séparation Lane A / Lane B.

### Novel Patterns

#### Momentum Dynamique — Modifier Stack Pattern

**Purpose:** Centraliser les modifications de momentum (parade, hit, dash, mort) sans disperser de logique dans le code combat.

**Components:**
- `momentum.c/.h` : stockage des modifiers actifs
- `momentum_modifier_t` : type, intensité, durée, source
- `character_update()` : lecture de la valeur finale agrégée

**Data Flow:**
1. Un événement gameplay ajoute ou retire un modifier.
2. Le système momentum expire les modifiers temporels.
3. La valeur finale est recalculée puis consommée par mouvement/attaque.
4. À la mort, stack reset atomiquement.

**Implementation Guide:**

```c
typedef enum {
    MOMENTUM_MOD_PARRY_BONUS,
    MOMENTUM_MOD_HIT_PENALTY,
    MOMENTUM_MOD_DASH_BONUS
} momentum_mod_type_t;

typedef struct {
    momentum_mod_type_t type;
    float delta;
    int remaining_frames;
} momentum_modifier_t;

float momentum_compute(const momentum_t* momentum) {
    float value = momentum->base_value;
    for (int i = 0; i < momentum->count; ++i) {
        value += momentum->mods[i].delta;
    }
    return value;
}
```

**Usage:** À utiliser pour toute règle qui influence vitesse, récupération ou agressivité via le momentum.

#### Épée Évolutive — State-Based Attribute Scaling

**Purpose:** Faire évoluer la portée et les attributs d'épée selon le compteur de parades, sans if dispersés.

**Components:**
- `sword.c/.h` : machine d'état de niveau d'épée
- `sword_level_t` : états discrets (`SWORD_LV1..LVn`)
- table de stats : portée, vitesse d'attaque, recovery

**Data Flow:**
1. Le compteur de parades est mis à jour par le système combat.
2. Le niveau d'épée est dérivé par règle centralisée.
3. Les hitboxes lisent la portée depuis la table de stats.

**Implementation Guide:**

```c
typedef enum {
    SWORD_LV1,
    SWORD_LV2,
    SWORD_LV3
} sword_level_t;

typedef struct {
    float range;
    int startup_frames;
} sword_stats_t;

static const sword_stats_t SWORD_TABLE[] = {
    [SWORD_LV1] = { .range = 24.0f, .startup_frames = 5 },
    [SWORD_LV2] = { .range = 30.0f, .startup_frames = 5 },
    [SWORD_LV3] = { .range = 36.0f, .startup_frames = 4 }
};

float sword_get_range(sword_level_t level) {
    return SWORD_TABLE[level].range;
}
```

**Usage:** À utiliser pour toute mécanique de progression contextuelle de l'arme.

#### Input Buffering — Ring Buffer with Timestamps

**Purpose:** Garantir la précision frame-perfect sur fenêtres de 2–3 frames et éviter les inputs perdus.

**Components:**
- `input_buffer.c/.h` : buffer circulaire taille fixe
- `input_sample_t` : masque input + index frame
- consommateur gameplay : recherche dernier input valide

**Data Flow:**
1. Chaque frame logique pousse un sample input.
2. Les actions lisent la fenêtre de validité (`current_frame - N`).
3. Le dernier input compatible déclenche l'action.

**Implementation Guide:**

```c
#define INPUT_BUFFER_SIZE 3

typedef struct {
    uint32_t input_mask;
    uint32_t frame_index;
} input_sample_t;

typedef struct {
    input_sample_t samples[INPUT_BUFFER_SIZE];
    int head;
} input_buffer_t;

bool input_buffer_find_recent(const input_buffer_t* b, uint32_t now, uint32_t wanted, uint32_t window) {
    for (int i = 0; i < INPUT_BUFFER_SIZE; ++i) {
        int idx = (b->head - i + INPUT_BUFFER_SIZE) % INPUT_BUFFER_SIZE;
        const input_sample_t* s = &b->samples[idx];
        if ((s->input_mask & wanted) && (now - s->frame_index <= window)) {
            return true;
        }
    }
    return false;
}
```

**Usage:** Obligatoire pour parry, changement de garde et déclenchements à fenêtre courte.

#### IA MinMax Temps-Réel — Time-Budgeted Search

**Purpose:** Imposer un budget strict par frame IA avec arrêt propre et fallback robuste.

**Components:**
- `minmax.c/.h` : recherche itérative interrompable
- timer haute résolution : suivi budget
- fallback action : meilleure action courante

**Data Flow:**
1. Démarrage recherche avec budget (`0.8ms` cible).
2. MinMax + alpha-beta avance tant que budget disponible.
3. Si budget dépassé, retour immédiat de la meilleure action accumulée.

**Implementation Guide:**

```c
ai_action_t minmax_decide(const game_state_t* state, double budget_ms) {
    uint64_t start = timer_now_us();
    ai_action_t best = AI_ACTION_IDLE;

    for (int depth = 1; depth <= MAX_DEPTH; ++depth) {
        if (timer_elapsed_us(start) > (uint64_t)(budget_ms * 1000.0)) {
            break;
        }
        ai_action_t current = search_depth(state, depth, -INF_SCORE, INF_SCORE, start, budget_ms);
        best = current;
    }

    return best;
}
```

**Usage:** Obligatoire pour toutes difficultés IA en runtime.

#### Génération Procédurale — Generate → Validate → Retry

**Purpose:** Produire des arènes variées mais systématiquement jouables et fair.

**Components:**
- `generator.c/.h` : génération brute depuis seed
- `validator.c/.h` : règles de jouabilité/fairness
- orchestrateur retry : boucle max tentatives

**Data Flow:**
1. Génération d'une arène candidate.
2. Validation indépendante (spawns, gaps, symétrie, reachability).
3. En échec, nouveau seed et nouvelle tentative.
4. En succès, commit de l'arène.

**Implementation Guide:**

```c
bool arena_build_valid(arena_t* out, uint32_t seed) {
    for (int attempt = 0; attempt < 32; ++attempt) {
        arena_t candidate;
        arena_generate(&candidate, seed + (uint32_t)attempt);

        validation_result_t vr = arena_validate(&candidate);
        if (vr.ok) {
            *out = candidate;
            return true;
        }
    }
    return false;
}
```

**Usage:** Obligatoire pour IP1 afin de garantir stabilité inter-lanes.

### Communication Patterns

**Pattern:** Références directes via interfaces étroites + callbacks ciblés (pas d'event bus global).

**Example:**

```c
typedef struct game_services {
    const arena_api_t* arena;
    const ai_api_t* ai;
} game_services_t;

void combat_system_update(combat_system_t* combat, const game_services_t* services) {
    arena_snapshot_t snapshot = services->arena->get_snapshot();
    combat_apply_arena_constraints(combat, &snapshot);
}
```

### Entity Patterns

**Creation:** Factory + Object Pooling ciblé pour entités fréquentes (VFX, sparks, slash).

**Example:**

```c
entity_t* entity_factory_spawn(entity_factory_t* f, entity_type_t type) {
    if (type == ENTITY_SPARK) {
        return spark_pool_acquire(&f->spark_pool);
    }
    return malloc_entity(type);
}
```

### State Patterns

**Pattern:** State Machine explicite pour combat et animation gameplay critique.

**Example:**

```c
typedef enum {
    PLAYER_IDLE,
    PLAYER_ATTACK_STARTUP,
    PLAYER_ATTACK_ACTIVE,
    PLAYER_RECOVERY,
    PLAYER_PARRY
} player_state_t;

void player_transition(player_t* p, player_state_t next) {
    if (is_transition_valid(p->state, next)) {
        p->state = next;
        p->state_frame = 0;
    }
}
```

### Data Patterns

**Access:** Data manager central pour config + chargement deterministic via seed.

**Example:**

```c
const gameplay_config_t* config_get_gameplay(const data_manager_t* dm) {
    return &dm->config.gameplay;
}

uint32_t match_seed(const match_context_t* ctx) {
    return hash_u32(ctx->match_id ^ ctx->player_a_id ^ ctx->player_b_id);
}
```

### Consistency Rules

| Pattern | Convention | Enforcement |
| ------- | ---------- | ----------- |
| Momentum Modifier Stack | Toute mutation passe par `momentum_add/remove/reset` | Revue code + grep hooks |
| Épée Évolutive | Portée uniquement via `sword_get_range(level)` | Tests unitaires combat |
| Input Buffer | Lecture inputs critiques via buffer 3 frames | Tests déterminisme replay |
| MinMax Time-Budgeted | Toute décision IA reçoit `budget_ms` explicite | Profiling CI (`<1ms`) |
| Generate-Validate-Retry | `validator` séparé de `generator` | Tests fuzz seeds |
| Communication Inter-lanes | Pas d'accès direct structures internes lane opposée | Validation headers + CI |
| State Machine | Transitions via fonctions dédiées, pas de mutation brute | Assertions debug |
| Entity Creation | Entités haute fréquence passent par pool | Instrumentation allocations |

---

## Architecture Validation

### Validation Summary

| Check | Result | Notes |
| ------ | ------ | ----- |
| Decision Compatibility | PASS | SDL3 + patterns choisis cohérents sans conflit structurel détecté |
| GDD Coverage | PASS | Tous les systèmes critiques GDD sont couverts architecturalement |
| Pattern Completeness | PASS | Patterns standard + novel documentés avec exemples concrets |
| Epic Mapping | PASS | Intégrations IP1/IP2 explicites, lane boundaries respectées |
| Document Completeness | PASS | Sections requises présentes, pas de placeholder/TODO détecté |

### Coverage Report

**Systems Covered:** 6/6  
**Patterns Defined:** 9  
**Decisions Made:** 8

### Issues Resolved

- Clarification et formalisation des patterns novel (momentum, épée, IA, génération)
- Alignement explicite communication inter-lanes via interfaces étroites
- Consolidation des règles de cohérence pour limiter les divergences d'implémentation agent

### Validation Date

2026-03-04

---

## Development Environment

### Prerequisites

- GCC 13+ ou Clang 16+
- `make`, `pkg-config`, `git`
- SDL3 (`SDL3`, `SDL3_image`, `SDL3_mixer`)
- Linux Fedora/Ubuntu (ou équivalent) avec libs graphiques standards

### AI Tooling (MCP Servers)

Aucun MCP serveur spécifique moteur n'a été sélectionné pendant ce workflow. Vous pouvez en ajouter plus tard via une recherche ciblée sur des intégrations SDL3/C.

### Setup Commands

```bash
# Fedora
sudo dnf install gcc make pkg-config SDL3-devel SDL3_image-devel SDL3_mixer-devel

# Ubuntu/Debian
sudo apt install build-essential pkg-config libsdl3-dev libsdl3-image-dev libsdl3-mixer-dev

# Build
make
```

### First Steps

1. Cloner/ouvrir le projet puis vérifier les dépendances SDL3 installées.
2. Lancer `make` et corriger tout warning bloquant d'include/linking.
3. Valider les interfaces d'intégration IP1/IP2 entre Lane A et Lane B.
4. Démarrer l'implémentation par epic en suivant les patterns définis dans ce document.
