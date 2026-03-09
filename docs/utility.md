# 🧱 Architecture du projet Blade Rush — Vue modulaire

## 📁 `src/ai/` — **Intelligence artificielle du bot**

Ce dossier gère toute la logique de décision du bot IA.

- `ai.c` → point d’entrée principal de l’IA
- `state.c/h` → représentation du game state pour l’IA
- `evaluator.c/h` → heuristique d’évaluation d’un état
- `minmax.c/h` → algorithme Min-Max / Alpha-Beta
- `cache.c/h` → cache des états déjà évalués
- `difficulty.c/h` → gestion des niveaux de difficulté

👉 Ce module est indépendant du moteur, et peut être testé en console.

---

## 📁 `src/arena/` — **Génération et gestion de la map**

Ce module gère la structure de l’arène et sa génération procédurale.

- `arena.c` → structure principale + accès aux tiles
- `generator.c/h` → génération procédurale
- `seed.c/h` → gestion du RNG
- `tiles.c/h` → types de cases (sol, plateforme, spawn…)
- `validator.c/h` → vérification de la validité de la map

👉 C’est le cœur spatial du jeu, utilisé par le combat et l’IA.

---

## 📁 `src/combat/` — **Système de duel**

Ce module gère les interactions physiques et les actions des joueurs.

- `character.c/h` → état du joueur (position, vie, etc.)
- `attacks.c/h` → logique des attaques
- `momentum.c/h` → inertie, frames de mouvement
- `physics.c/h` → collisions, gravité, déplacements
- `sword.c/h` → gestion de l’épée
- `combat.c` → orchestrateur du combat

👉 C’est le gameplay pur, celui que tu veux prioriser.

---

## 📁 `src/engine/` — **Moteur SDL3 et abstractions techniques**

Ce module encapsule SDL3 et les fonctions techniques.

- `audio.c/h` → effets sonores
- `input.c/h` → gestion des entrées clavier/manette
- `render.c/h` → affichage des sprites, HUD, etc.
- `timing.c/h` → gestion du delta time
- `debug.c/h` → overlays debug, logs visuels
- `engine.c` → initialisation globale

👉 Ce module est la glue technique entre SDL3 et ton jeu.

---

## 📁 `src/game/` — **Boucle de jeu et logique de haut niveau**

Ce module gère le cycle de vie du jeu, les menus, la campagne, etc.

- `game.c` → boucle principale, transitions entre phases
- `match.c/h` → gestion d’une partie (rounds, scores…)
- `menu.c/h` → navigation dans les menus
- `hud.c/h` → affichage des infos en jeu
- `config.c/h` → configuration du match (difficulté, map…)

👉 C’est le chef d’orchestre du jeu.

---

## 📁 `src/utils/` — **Fonctions utilitaires génériques**

- `logging.c/h` → logs texte
- `math.c/h` → vecteurs, calculs
- `memory.c/h` → allocation, vérifications
- `constants.h` → constantes globales
- `utils.c` → fonctions diverses

👉 Ce module est utilisé partout, mais ne dépend de rien.

---

## 📁 `include/` — **Headers publics**

Tous les headers accessibles globalement, organisés par module :

- `ai.h`, `arena.h`, `combat.h`, `engine.h`, `game.h`, `utils.h`, etc.
- `types.h` → contient tous les types partagés (`GameState`, `Vec2f`, `MatchPhase`, etc.)
- `ui.h` → encapsulation de SDL3 pour l’interface

---

## 📄 `main.c` — **Point d’entrée du programme**

- Initialise le moteur  
- Lance la boucle de jeu  
- Gère les erreurs fatales

---

# 🧠 Ce que cette architecture permet

- **Modularité forte** → chaque domaine est isolé
- **Testabilité** → tu peux tester l’IA ou le combat sans SDL
- **Lisibilité** → chaque fichier a une responsabilité claire
- **Scalabilité** → tu peux ajouter des modes, des assets, des effets sans casser le reste

---

# 🔥 Ce que tu peux faire maintenant

Vu que tu veux te concentrer sur le gameplay :

- Commence par `combat/` et `arena/`  
- Utilise `game.c` pour orchestrer les phases  
- Ignore `menu.c` et `multiplayer` pour l’instant  
- Utilise `ui_render_hud()` pour afficher un fond simple
