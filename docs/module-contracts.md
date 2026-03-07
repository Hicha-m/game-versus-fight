# Contrats de modules - game-versus-fight

## Couches et dependances autorisees

- `types.h` est la base des types metier partages.
- `constants.h` contient les constantes globales de gameplay et de simulation.
- `utils.h` expose des utilitaires transverses (memoire, logs, maths).
- `arena.h`, `combat.h`, `ai.h`, `engine.h`, `ui.h`, `game.h` s'appuient sur `types.h`.
- `game.h` est la facade publique de haut niveau.

Contraintes de couplage :

- `ai` ne depend pas de `ui`.
- `ui` lit l'etat via `const GameState*` et ne pilote pas directement la simulation.
- `utils` reste independant du metier (pas de dependance vers arena/combat/ai/engine/game).

## Contrat d'erreurs

Les API metier publiques retournent `GameError` (hors helpers purs). Valeurs disponibles :

- `GAME_OK`
- `GAME_ERROR_INVALID_ARGUMENT`
- `GAME_ERROR_INVALID_STATE`
- `GAME_ERROR_OUT_OF_MEMORY`
- `GAME_ERROR_OUT_OF_BOUNDS`
- `GAME_ERROR_NOT_FOUND`
- `GAME_ERROR_UNSUPPORTED`
- `GAME_ERROR_TIMEOUT`
- `GAME_ERROR_IO`
- `GAME_ERROR_INTERNAL`

Regles :

- Retourner `GAME_OK` si l'operation est valide.
- Utiliser un code d'erreur specifique en cas d'echec.
- Ne jamais retourner une valeur non definie dans `GameError`.

## Contrat de memoire

- La map `Arena.tiles` est possedee par `Arena`.
- `arena_generate(...)` initialise ou remplace l'etat interne de `Arena`.
- `arena_destroy(...)` remet `Arena` dans un etat neutre et ne laisse pas de pointeur interne invalide.
- Un parametre `const T*` ne transfere jamais la propriete memoire.
- Les fonctions `*_init` et `*_create` initialisent un etat ; les fonctions `*_shutdown` et `*_destroy` liberent cet etat.

## Contrat de mutation

- `const GameState*` : lecture seule, aucune mutation.
- `GameState*` : mutation autorisee de l'etat de simulation.
- `FrameInput` est consomme sur la frame courante et n'est pas conserve par ownership.

## Contrat temporel et constantes

Le contrat moteur/combat/IA doit rester coherent avec les constantes de `include/constants.h` :

- `TARGET_FPS` (`60U`)
- `FIXED_TIMESTEP_MS` (`16U`)
- `AI_TIME_BUDGET_US` (`1000U`)
- `MAX_INPUT_BUFFER_FRAMES`
- `MAX_ROUND_TIME_SECONDS`

En particulier :

- La simulation est en pas fixe autour de `FIXED_TIMESTEP_MS`.
- Le choix d'action IA doit respecter le budget `AI_TIME_BUDGET_US`.
- La boucle globale vise la cadence `TARGET_FPS`.
