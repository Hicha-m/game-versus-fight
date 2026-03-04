# Blade Rush - Development Epics

**Project:** jeux IA  
**Game:** Blade Rush  
**Team:** Hicham (Lane A) + Walid (Lane B)  
**Strategy:** Parallel Lanes with Integration Points

---

## Epic Overview

| # | Epic Name | Lane | Scope | Dependencies | Est. Stories |
|---|-----------|------|-------|--------------|--------------|
| A1 | Core Duel Engine | Lane A (Gameplay) | Mouvement, postures, combat, collisions | Aucune | 8-10 |
| A2 | Match Flow & UX/UI | Lane A (Gameplay) | Menu, HUD, paramètres, scoring | A1 | 6-8 |
| A3 | Art/Audio Integration | Lane A (Gameplay) | Sprites, SFX, musiques, polish | A1, A2 | 5-7 |
| B1 | Arena Procedural Generation | Lane B (Systems) | Générateur, tiles, validation fairness | Aucune | 6-8 |
| B2 | IA Min-Max & Difficulty | Lane B (Systems) | Min-Max, alpha-beta, 4 difficultés | Aucune | 8-10 |
| B3 | Balancing & AI Tuning | Lane B (Systems) | Tuning IA, tests fairness | B1, B2 | 4-6 |

---

## Lane A — Gameplay & Engine (Hicham)

### Epic A1: Core Duel Engine

#### Goal

Construire le moteur de duel 1v1 complet incluant mouvement, système de postures d'épée, attaques/parades, collisions, et respawn territorial.

#### Scope

**Includes:**
- Mouvement 2D (gauche/droite, saut)
- Gestion collision personnage/environnement
- Système 3 hauteurs d'épée (haut/mid/bas)
- Attaque avec détection de touche selon posture
- Parade/blocage selon opposition de postures
- Désarmement si parade réussie
- Respawn immédiat après mort
- Système de scoring territorial (avancement gauche/droite)
- Condition de victoire (atteindre extrémité opposée)
- IA dummy (ne fait rien) pour tests

**Excludes:**
- IA compétitive (Epic B2)
- Génération procédurale (Epic B1)
- UI/menus (Epic A2)
- Assets finaux art/audio (Epic A3)
- Zones spéciales boost/slow (ajoutées plus tard)

#### Dependencies

Aucune. Epic de fondation.

#### Deliverable

Duel jouable 1v1 humain vs "IA dummy" sur map simple hard-codée, avec placeholder visuals (carrés colorés). Gameplay loop complet validé.

#### Stories

1. En tant que joueur, je peux me déplacer gauche/droite sur une map 2D
2. En tant que joueur, je peux sauter et retomber avec gravité
3. En tant que joueur, je peux changer la hauteur de mon épée (haut/mid/bas)
4. En tant que joueur, je peux attaquer et toucher l'adversaire selon ma posture
5. En tant que joueur, je peux parer une attaque adverse si ma posture bloque la sienne
6. En tant que joueur, je peux désarmer l'adversaire avec une parade réussie
7. En tant que joueur, je respawn immédiatement après mort avec scoring territorial
8. En tant que joueur, je gagne la partie en atteignant l'extrémité opposée
9. En tant que développeur, je peux tester contre une IA dummy
10. En tant que développeur, le collision system gère trous/obstacles basiques

---

### Epic A2: Match Flow & UX/UI

#### Goal

Créer l'expérience match complète de bout en bout : menu principal, sélection paramètres, HUD en match, et écran de victoire.

#### Scope

**Includes:**
- Menu principal (Start, Options, Quit)
- Sélection difficulté IA (Facile/Normal/Difficile/Très Difficile)
- Sélection archétype map (Small/Medium/Large) ou map de base
- HUD en match : barres posture, icône avantage territorial, timer round
- Écran fin de round (stats: temps, kills, avantage)
- Écran victoire finale
- Gestion input clavier rebindable (post-MVP: manette optionnelle)

**Excludes:**
- Assets art/audio finaux (Epic A3)
- Génération procédurale détaillée (Epic B1)

#### Dependencies

- **A1:** Doit avoir le core duel fonctionnel pour flow match

#### Deliverable

Expérience match complète : menu → sélection paramètres → match → écran victoire, avec UI fonctionnelle (placeholder visuals acceptables).

#### Stories

1. En tant que joueur, je peux lancer le jeu et voir un menu principal
2. En tant que joueur, je peux choisir la difficulté IA avant le match
3. En tant que joueur, je peux choisir un archétype d'arène ou une map de base
4. En tant que joueur, je vois en HUD ma posture actuelle et celle de l'adversaire
5. En tant que joueur, je vois l'avantage territorial en temps réel
6. En tant que joueur, je vois le temps de round écoulé
7. En tant que joueur, je vois un écran de fin de round avec mes stats
8. En tant que joueur, je vois un écran de victoire finale après avoir gagné

---

### Epic A3: Art/Audio Integration & Polish

#### Goal

Intégrer les assets finaux pixel art moderne et audio électro/métallique, et polir l'expérience audiovisuelle MVP.

#### Scope

**Includes:**
- Sprites finaux personnages (2 couleurs, ~20-25 frames par perso)
- Sprites environnement (tiles, obstacles, trous, zones)
- Effets visuels (slash, étincelles, poussière)
- SFX métalliques (attaque, parade, mort, respawn, pas, saut, UI)
- Musiques (menu, combat, victoire)
- Synchronisation feedback visuel/sonore avec gameplay
- Écrans de chargement/transition si nécessaire
- Palette finale (contraste fort, lisibilité)

**Excludes:**
- Cosmétiques post-MVP (biomes, thèmes visuels)
- Voix longues (seulement grunts + annonces UI)

#### Dependencies

- **A1:** Core gameplay doit être stable
- **A2:** UI/flow doivent être en place

#### Deliverable

Expérience audiovisuelle MVP complète avec style pixel art moderne, SFX métalliques, musique électro, et polish visuel compétitif.

#### Stories

1. En tant que joueur, je vois des sprites pixel art finaux pour les personnages
2. En tant que joueur, je vois des animations fluides (idle, marche, attaque, parade, mort)
3. En tant que joueur, je vois des effets visuels clairs lors d'attaque/parade
4. En tant que joueur, j'entends des SFX métalliques percutants lors du combat
5. En tant que joueur, j'entends une musique électro/synthwave en background
6. En tant que joueur, les retours audio/visuels sont synchronisés et immédiats
7. En tant que joueur, la palette respecte le contraste fort (rouge vs bleu)

---

## Lane B — IA & Procedural Systems (Walid)

### Epic B1: Arena Procedural Generation

#### Goal

Construire le système de génération procédurale d'arènes 2D avec validation de fairness automatique et archétypes contrôlés.

#### Scope

**Includes:**
- Générateur d'arènes procédurales (pre-match generation)
- 3 archétypes : Small, Medium, Large
- Tiles : sol, obstacle, trou, zone boost, zone slow
- Validation fairness automatique (spawn symétriques, pas de segment injuste, distribution équilibrée)
- Régénération automatique si validation échoue
- Maps de base hard-codées pour tests
- Seed fixe pour reproductibilité
- Interface publique : `Arena* generate_arena(ArenaSeed seed, ArchetypeType type)`

**Excludes:**
- Cosmétiques visuels (Epic A3)
- Balancing fin (Epic B3)

#### Dependencies

Aucune. Epic de fondation.

#### Deliverable

Système générateur d'arènes procédurales stable avec API claire consommable par Lane A. Validation fairness garantie.

#### Stories

1. En tant que système, je génère une arène Small avec contraintes (courte, agressive)
2. En tant que système, je génère une arène Medium avec contraintes (équilibrée)
3. En tant que système, je génère une arène Large avec contraintes (longue, stratégique)
4. En tant que système, je place des obstacles espacés selon règles fairness
5. En tant que système, je place des trous limités (max 3) sans être injustes
6. En tant que système, je place des zones boost/slow réparties équitablement
7. En tant que système, je valide spawn symétriques et distance équitable au 1er obstacle
8. En tant que système, je régénère l'arène automatiquement si validation échoue
9. En tant que développeur, je peux charger une map de base pour tests
10. En tant que développeur, l'interface `generate_arena()` est stable et documentée

---

### Epic B2: IA Min-Max & Difficulty System

#### Goal

Implémenter une IA compétitive Min-Max avec alpha-beta pruning et 4 niveaux de difficulté calibrés.

#### Scope

**Includes:**
- Modèle d'état discret pour IA (danger_bot, obstacle_between, zone_type)
- Min-Max avec alpha-beta pruning
- 4 niveaux de difficulté :
  - Facile : profondeur 1, randomness ±20%
  - Normal : profondeur 2, randomness ±10%
  - Difficile : profondeur 3, randomness ±5%
  - Très Difficile : profondeur 3, randomness 0%
- Évaluation locale (pas de vision globale complète)
- Temps de calcul < 1 ms, branching factor ≤ 5
- Interface publique : `Action ai_get_action(GameState* state, DifficultyLevel diff)`

**Excludes:**
- Balancing fin des poids d'évaluation (Epic B3)
- IA réseau/apprentissage (hors MVP)

#### Dependencies

Aucune (peut utiliser mock GameState pour tests). Intégration avec A1 à IP2.

#### Deliverable

IA Min-Max fonctionnelle et compétitive avec 4 difficultés, respectant budget de performance, consommable par Lane A via API stable.

#### Stories

1. En tant qu'IA, je représente l'état du jeu en modèle discret local
2. En tant qu'IA, j'évalue les actions possibles selon danger_bot, obstacle, zone
3. En tant qu'IA, j'implémente Min-Max avec alpha-beta pruning
4. En tant qu'IA, je respecte le budget temps < 1 ms par décision
5. En tant qu'IA, je propose 4 difficultés calibrées (profondeur + randomness)
6. En tant qu'IA Facile, je suis battable facilement (profondeur 1)
7. En tant qu'IA Normal, je suis un défi raisonnable (profondeur 2)
8. En tant qu'IA Difficile, je challenge les joueurs experts (profondeur 3, faible random)
9. En tant qu'IA Très Difficile, je joue de manière optimale (profondeur 3, 0% random)
10. En tant que développeur, l'interface `ai_get_action()` est stable et documentée

---

### Epic B3: Balancing & AI Tuning

#### Goal

Affiner les paramètres IA, valider la fairness des générations procédurales, et ajuster le gameplay selon feedback.

#### Scope

**Includes:**
- Tuning poids d'évaluation IA (danger, distance, avantage territorial)
- Ajustement temps de réaction IA si nécessaire
- Tests de fairness sur échantillon large de maps générées
- Ajustements paramètres archétypes (densité obstacles, largeur trous)
- Validation balance global MVP

**Excludes:**
- Nouveaux systèmes (tout doit exister déjà)

#### Dependencies

- **B1:** Générateur doit être complet
- **B2:** IA doit être fonctionnelle
- **A1:** Gameplay de base pour tester balance

#### Deliverable

Balance MVP validé : IA compétitive mais fair, arènes toujours jouables, gameplay loop satisfaisant.

#### Stories

1. En tant que tuner, j'ajuste les poids d'évaluation IA selon playtests
2. En tant que tuner, je valide que Facile est accessible aux débutants
3. En tant que tuner, je valide que Très Difficile est un vrai challenge
4. En tant que tuner, je teste 100+ maps générées pour fairness
5. En tant que tuner, j'ajuste les contraintes archétypes si nécessaire
6. En tant que tuner, je valide que l'IA ne triche pas et reste lisible

---

## Integration Points

### IP1: Arena Interface (après A1 + B1)

**Objectif:** Lane A peut consommer les arènes générées par Lane B.

**Contrat:**
```c
typedef struct Arena {
    int width;
    int height;
    Tile* tiles;
    SpawnPoint spawn_left;
    SpawnPoint spawn_right;
} Arena;

Arena* generate_arena(ArenaSeed seed, ArchetypeType type);
void free_arena(Arena* arena);
```

**Test de validation:**
- Lancer un duel sur une arène procédurale Small
- Vérifier que collision/gameplay fonctionnent
- Vérifier spawn symétriques et fairness

**Responsables:** Hicham + Walid en pair programming

---

### IP2: IA Interface (après A1 + B2)

**Objectif:** Lane A peut intégrer l'IA de Lane B comme adversaire.

**Contrat:**
```c
typedef enum {
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_JUMP,
    ACTION_ATTACK,
    ACTION_CHANGE_STANCE_UP,
    ACTION_CHANGE_STANCE_DOWN,
    ACTION_IDLE
} Action;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD,
    DIFFICULTY_VERY_HARD
} DifficultyLevel;

Action ai_get_action(GameState* state, DifficultyLevel diff);
```

**Test de validation:**
- Lancer un duel humain vs IA (difficulté Normal)
- Vérifier que l'IA est compétitive mais pas invincible
- Vérifier budget temps < 1 ms

**Responsables:** Hicham + Walid en pair programming

---

### IP3: MVP Complet (après A2/A3 + B3)

**Objectif:** Merge complet des 2 lanes, expérience end-to-end validée.

**Tests de validation:**
- QA complète : menu → match → victoire
- Test des 4 difficultés IA
- Test des 3 archétypes d'arènes + maps de base
- Validation 60 FPS stable
- Validation latency input < 30 ms
- Validation stabilité mémoire
- Build final < 50 Mo

**Responsables:** Hicham + Walid ensemble

---

## Sprint Planning Recommendation

### Sprint 0: Setup (1 semaine)
- Définir interfaces Arena et IA (contrats IP1/IP2)
- Setup repo Git avec branches `lane-a` et `lane-b`
- Créer stubs/mocks pour chaque lane
- Définir convention code et structure projet

### Sprint 1-3: Phase 1 Parallel (3 semaines)
- **Lane A (Hicham):** Epic A1 (Core Duel Engine)
- **Lane B (Walid):** Epic B1 (Arena) + Epic B2 (IA)
- **Fin Sprint 3:** IP1 + IP2 ensemble

### Sprint 4-5: Phase 2 Parallel (2 semaines)
- **Lane A (Hicham):** Epic A2 (Match Flow) + Epic A3 (Art/Audio)
- **Lane B (Walid):** Epic B3 (Balancing)
- **Fin Sprint 5:** IP3 ensemble

### Sprint 6: Merge & Release (1 semaine)
- Intégration finale
- QA end-to-end
- Fixes critiques
- Packaging & release MVP

**Total estimé:** 7 sprints (7 semaines si sprints 1 semaine, ou ~14 semaines si sprints 2 semaines)

---

## Vertical Slice Milestone

**Quand:** Fin Sprint 3 (après IP2)

**Contenu:**
- Duel 1v1 humain vs IA (difficulté Normal)
- Arène procédurale Small générée
- Placeholder visuals/audio acceptables
- Gameplay loop complet validé

**Validation:**
- Core loop fonctionne de bout en bout
- IA est compétitive et fun
- Génération procédurale stable et fair
- Interfaces Lane A/B validées
- Performance 60 FPS atteinte

**Décision:** Go/No-Go pour Phase 2 (polish + release MVP)
