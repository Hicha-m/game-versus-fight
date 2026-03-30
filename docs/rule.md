# Compte rendu détaillé de l’architecture du projet

## 1) Objectif général de l’architecture

L’objectif de cette architecture est de construire un projet **modulaire**, **maintenable**, et **académiquement propre**, en séparant clairement :

*   la **logique de jeu**
*   la **gestion des entrées**
*   la **boucle moteur**
*   le **rendu**
*   la **plateforme / SDL**
*   les **utilitaires génériques**

L’idée centrale est la suivante :

> **Le gameplay ne doit pas dépendre de SDL.**  
> SDL doit rester encapsulé dans les couches techniques du moteur.

Cela permet :

*   d’éviter les dépendances inutiles,
*   de garder des modules réutilisables,
*   d’avoir une architecture claire pour l’analyse du projet,
*   et de bien distinguer **ce qui est métier** et **ce qui est technique**.

***

# 2) Principe fondamental : séparation entre logique métier et technique

Le projet repose sur une règle très importante :

## 2.1. La logique métier

La logique métier correspond à :

*   l’état du jeu,
*   les règles du combat,
*   les déplacements,
*   les collisions métier,
*   la simulation,
*   l’IA éventuelle.

Cette logique doit être **indépendante** de SDL.

Elle ne doit pas connaître :

*   `SDL_Window`
*   `SDL_Renderer`
*   `SDL_Texture`
*   `SDL_Scancode`
*   les événements SDL
*   les appels SDL de rendu ou de clavier

***

## 2.2. La logique technique

La logique technique correspond à :

*   la création de la fenêtre,
*   l’initialisation SDL,
*   la lecture clavier réelle,
*   le rendu effectif,
*   la gestion du temps réel,
*   l’interface avec la plateforme.

Cette logique peut utiliser SDL, mais elle doit être confinée dans les modules prévus pour ça.

***

# 3) Organisation générale des modules

L’architecture est découpée en plusieurs couches.

## 3.1. `main`

Rôle :

*   point d’entrée du programme,
*   assemblage des grands modules,
*   lancement de la boucle principale.

`main` ne doit pas contenir de logique de gameplay détaillée.  
Il doit rester une couche d’orchestration.

***

## 3.2. `engine`

Rôle :

*   gérer la boucle principale,
*   coordonner les modules,
*   gérer le temps,
*   centraliser la couche plateforme,
*   faire l’interface entre le gameplay et SDL,
*   éventuellement transformer les entrées techniques en entrées métier.

`engine` est la couche qui pilote le programme.

***

## 3.3. `render`

Rôle :

*   dessiner à l’écran,
*   afficher l’état du jeu,
*   encapsuler le rendu SDL,
*   transformer les données du jeu en représentation visuelle.

`render` peut utiliser SDL, mais ne doit pas contenir la logique métier du jeu.

***

## 3.4. `input`

Rôle :

*   représenter l’état des commandes de manière abstraite,
*   fournir des structures d’input propres au projet,
*   rester découplé de SDL.

Le module `input` ne doit pas dépendre de SDL.  
Il manipule des notions comme :

*   `InputState`
*   `KeyCode` internes au projet
*   états de boutons/actions

***

## 3.5. `game`

Rôle :

*   contenir la logique de haut niveau du gameplay,
*   organiser les sous-systèmes gameplay,
*   représenter l’état global du jeu.

Le module `game` ne doit pas dépendre de SDL, ni de `engine`, ni de `render`.

***

## 3.6. `game/arena`

Rôle :

*   gérer l’arène,
*   dimensions, limites, zones, positions structurelles.

C’est un sous-module de gameplay pur.

***

## 3.7. `game/combat`

Rôle :

*   gérer les règles de combat,
*   attaques,
*   coups,
*   états d’armes,
*   interactions entre joueurs.

C’est un sous-module de gameplay pur.

***

## 3.8. `game/ai`

Rôle :

*   IA éventuelle,
*   prise de décision,
*   heuristiques,
*   minimax si tu l’intègres ici.

C’est aussi un sous-module gameplay pur.

***

## 3.9. `utils`

Rôle :

*   utilitaires génériques,
*   fonctions transversales,
*   math simples,
*   helpers sans dépendance métier ni SDL.

`utils` doit rester le plus neutre possible.

***

## 3.10. `core`

Rôle :

*   types partagés très génériques,
*   constantes centrales,
*   primitives communes,
*   structures transversales non liées à SDL.

`core` est une base commune, mais elle doit rester minimale.

***

# 4) Règle principale sur les dépendances

Voici la règle fondamentale :

> **Les dépendances doivent aller du haut niveau technique vers le bas niveau métier uniquement par des interfaces propres.**  
> Le gameplay ne doit jamais remonter vers SDL.

Autrement dit :

*   `game` ne connaît pas SDL
*   `input` ne connaît pas SDL
*   `arena`, `combat`, `ai` ne connaissent pas SDL
*   SDL reste dans `engine` (interne) et `render`

***

# 5) Règles sur les headers publics et privés

C’est le cœur du problème que tu avais très bien détecté.

***

## 5.1. Header public : définition

Un **header public** est un fichier `.h` destiné à être inclus par d’autres modules.

Exemples :

*   `include/engine/engine.h`
*   `include/game/game.h`
*   `include/input/input.h`

Un header public définit l’API visible d’un module.

***

## 5.2. Règle absolue des headers publics

Un header public ne doit contenir que :

*   des types standards C,
*   des types du projet,
*   des `struct` publiques du projet,
*   des `enum` du projet,
*   des déclarations de fonctions publiques,
*   éventuellement des pointeurs opaques.

Un header public **ne doit pas** :

*   inclure SDL,
*   exposer des types SDL,
*   exposer des détails internes,
*   inclure des headers privés.

***

## 5.3. Pourquoi cette règle existe

Si un header public inclut SDL, alors tous les modules qui l’incluent héritent indirectement de SDL.

Exemple :

*   si `engine.h` inclut SDL,
*   alors `game.c` qui inclut `engine.h` voit SDL,
*   donc `game` dépend de SDL,
*   ce qui casse l’encapsulation.

C’est précisément ce qu’on veut éviter.

***

## 5.4. Header privé / interne : définition

Un header privé est un fichier réservé à l’implémentation interne du module.

Exemples :

*   `src/engine/engine_internal.h`
*   `src/render/render_internal.h`
*   `src/game/combat/combat_internal.h`

Ces headers peuvent contenir :

*   des détails d’implémentation,
*   des structures complètes privées,
*   des dépendances techniques,
*   SDL si nécessaire,
*   des helpers internes.

***

## 5.5. Règle des headers privés

Les headers privés :

*   ne doivent pas être placés dans `include/`
*   ne doivent pas être inclus par des modules externes
*   ne doivent être utilisés que par les `.c` du module propriétaire

En pratique :

*   `engine_internal.h` est réservé à `src/engine/*`
*   `combat_internal.h` est réservé à `src/game/combat/*`
*   etc.

***

# 6) Règles détaillées par module

***

## 6.1. `main`

### Rôle

`main.c` ne doit faire que :

*   initialiser les gros systèmes,
*   lancer la boucle,
*   fermer proprement le programme.

### `main` peut dépendre de :

*   `engine`
*   éventuellement `game`
*   éventuellement `render`
*   éventuellement `input`
*   `core`

### `main` ne doit pas dépendre directement de :

*   `arena`
*   `combat`
*   `ai`
*   headers privés

### Pourquoi

Parce que `main` ne doit pas connaître les sous-détails du gameplay.  
Il orchestre, il ne manipule pas les sous-systèmes internes.

***

## 6.2. `engine`

### Rôle

Le moteur pilote :

*   la boucle de jeu,
*   le temps,
*   l’interface avec SDL,
*   la collecte des entrées,
*   l’enchaînement update/render.

### `engine` public

Le header public d’`engine` doit exposer :

*   des types propres au projet,
*   des fonctions comme `engine_create`, `engine_run`, `engine_destroy`,
*   éventuellement des types opaques.

### `engine` public ne doit pas exposer :

*   `SDL_Window`
*   `SDL_Renderer`
*   `SDL_Event`
*   `SDL_Scancode`
*   aucun type SDL

### `engine` privé

Le code interne d’`engine` peut utiliser :

*   SDL
*   des headers internes
*   des structures privées du moteur

### Dépendances autorisées

L’implémentation d’`engine` peut dépendre de :

*   SDL
*   `input`
*   `game`
*   `render`
*   `core`
*   `utils`

### Dépendances interdites

`engine` ne doit pas dépendre directement des détails internes de :

*   `arena`
*   `combat`
*   `ai`
*   headers privés gameplay

Pourquoi ?  
Parce qu’`engine` doit piloter le système global, pas entrer dans les détails des sous-modules métier.

***

## 6.3. `render`

### Rôle

Convertir l’état du jeu en image.

### `render` peut dépendre de :

*   SDL
*   `game` (API publique seulement)
*   `core`
*   `utils`

### `render` ne doit pas dépendre de :

*   headers privés du gameplay
*   logique interne de combat/arena/ai
*   structures privées du moteur gameplay

### Pourquoi

Le rendu doit observer l’état du jeu, pas connaître les détails d’implémentation internes.

***

## 6.4. `input`

### Rôle

Définir une représentation abstraite de l’entrée.

Exemples :

*   `InputState`
*   `ActionState`
*   `KeyCode` internes au projet

### `input` peut dépendre de :

*   `core`
*   `utils`

### `input` ne doit pas dépendre de :

*   SDL
*   `engine`
*   `render`

### Pourquoi

Parce que `input` représente l’intention du joueur, pas le backend technique.

L’input doit dire :

*   “le joueur saute”
*   “le joueur va à gauche”
*   “attaque active”

et non :

*   “la touche SDL scancode X est enfoncée”

***

## 6.5. `game`

### Rôle

Représenter la logique globale du jeu.

### `game` peut dépendre de :

*   `input`
*   `arena`
*   `combat`
*   `ai`
*   `core`
*   `utils`

### `game` ne doit pas dépendre de :

*   `engine`
*   `render`
*   SDL

### Pourquoi

Le gameplay doit être indépendant de la plateforme et du rendu.

***

## 6.6. `arena`

### Rôle

Gestion de l’environnement de jeu.

### `arena` peut dépendre de :

*   `core`
*   `utils`

### `arena` ne doit pas dépendre de :

*   `engine`
*   `render`
*   SDL
*   `combat` (sauf si tu assumes un couplage explicite, mais idéalement non)
*   `ai`

### Pourquoi

L’arène doit rester un composant gameplay spécialisé et simple.

***

## 6.7. `combat`

### Rôle

Règles des interactions de combat.

### `combat` peut dépendre de :

*   `arena` si nécessaire pour contraintes spatiales
*   `input`
*   `core`
*   `utils`

### `combat` ne doit pas dépendre de :

*   `engine`
*   `render`
*   SDL

### Pourquoi

Le combat doit rester purement logique.

***

## 6.8. `ai`

### Rôle

Décider les actions automatiques.

### `ai` peut dépendre de :

*   `game` ou d’une API gameplay dédiée
*   `combat`
*   `arena`
*   `input`
*   `core`
*   `utils`

### `ai` ne doit pas dépendre de :

*   `engine`
*   `render`
*   SDL

### Pourquoi

L’IA raisonne sur un état de jeu abstrait, pas sur la plateforme.

***

## 6.9. `utils`

### Rôle

Fonctions utilitaires neutres.

### `utils` ne doit idéalement dépendre de rien d’applicatif.

### `utils` ne doit pas dépendre de :

*   SDL
*   `engine`
*   `game`
*   `render`
*   `input`

### Pourquoi

Parce que sinon le module utilitaire perd son statut générique.

***

## 6.10. `core`

### Rôle

Types de base partagés.

### `core` ne doit pas dépendre de SDL ni de modules applicatifs.

### Pourquoi

Parce que `core` est censé être la base la plus stable et la plus neutre.

***

# 7) Sens des dépendances autorisées

Voici le sens logique des dépendances.

## Dépendances autorisées (vision générale)

*   `main` → `engine`
*   `engine` → `game`
*   `engine` → `render`
*   `engine` → `input`
*   `game` → `arena`
*   `game` → `combat`
*   `game` → `ai`
*   `combat` → `input`
*   `ai` → `game` ou API gameplay dédiée
*   `tous` → `core`
*   `certains` → `utils`

***

## Dépendances interdites majeures

*   `game` → `engine`
*   `game` → `render`
*   `game` → SDL
*   `input` → SDL
*   `input` → `engine`
*   `arena` → SDL
*   `combat` → SDL
*   `ai` → SDL
*   `render` → headers privés gameplay
*   `main` → `arena/combat/ai`
*   public headers → SDL
*   public headers → headers privés

***

# 8) Règle spécifique sur SDL

Cette règle doit être formulée clairement :

## SDL est autorisé uniquement dans :

*   l’implémentation interne d’`engine`
*   le module `render`
*   éventuellement `main.c` si tu l’acceptes, mais idéalement très limité
*   d’éventuels fichiers plateforme dédiés

## SDL est interdit dans :

*   tous les headers publics
*   `game`
*   `arena`
*   `combat`
*   `ai`
*   `input`
*   `utils`
*   `core`

***

# 9) API publique vs API privée

***

## 9.1. API publique

L’API publique d’un module est l’ensemble de ce qu’il expose officiellement aux autres modules.

Elle doit être :

*   stable,
*   minimale,
*   claire,
*   indépendante des détails techniques internes.

Exemples :

*   `game_create`
*   `game_update`
*   `engine_run`
*   `render_draw`
*   `input_state_clear`

Une bonne API publique expose :

*   le **quoi**
*   pas le **comment**

***

## 9.2. API privée

L’API privée contient :

*   les détails internes,
*   les structures cachées,
*   les helpers de bas niveau,
*   les conversions techniques,
*   les fonctions intermédiaires.

Exemples :

*   conversion `KeyCode -> SDL_Scancode`
*   structures internes du moteur
*   cache des textures
*   helpers internes de collision
*   structures d’états temporaires

L’API privée ne doit jamais être considérée comme une interface stable pour les autres modules.

***

# 10) Étape par étape : déroulement architectural du programme

Voici maintenant le fonctionnement du projet **dans l’ordre**, du démarrage à une frame typique.

***

## Étape 1 : démarrage du programme

Le programme commence dans `main`.

`main` :

*   crée le moteur,
*   crée éventuellement l’état du jeu,
*   initialise les modules principaux via leur API publique,
*   lance l’exécution globale.

À ce stade :

*   `main` ne doit pas construire lui-même le combat,
*   ni gérer l’arène directement,
*   ni interroger l’IA directement.

Il délègue cela aux modules supérieurs.

***

## Étape 2 : initialisation du moteur

Le moteur initialise :

*   SDL,
*   la fenêtre,
*   le contexte de rendu,
*   les systèmes internes,
*   les horloges et états techniques.

Tout cela est interne au moteur.

Le reste du projet ne doit pas voir ces détails.

***

## Étape 3 : initialisation du jeu

Le module `game` initialise :

*   l’état global de la partie,
*   les joueurs,
*   l’arène,
*   les structures de combat,
*   éventuellement l’IA.

Cette phase est purement gameplay.

Aucune dépendance SDL ne doit apparaître ici.

***

## Étape 4 : lecture des entrées

Le moteur lit les entrées réelles via SDL.

Ensuite, il convertit les données techniques en structures métier :

*   état clavier SDL
*   vers `InputState`
*   éventuellement via des `KeyCode` internes

Cette conversion est la responsabilité du moteur ou d’une couche plateforme.

Le module `input` ne lit pas SDL lui-même.  
Il définit seulement **la forme abstraite des commandes**.

***

## Étape 5 : mise à jour du gameplay

Le moteur appelle ensuite le gameplay avec :

*   l’état du jeu,
*   les entrées abstraites,
*   le `dt` (delta time),
*   les éventuels états des deux joueurs.

Le gameplay applique :

*   déplacements,
*   attaques,
*   interactions,
*   collisions métier,
*   règles de victoire,
*   logique d’IA,
*   logique d’arène.

Cette étape ne doit jamais appeler SDL.

***

## Étape 6 : préparation des données à afficher

Après la mise à jour, le moteur ou le rendu récupère l’état gameplay :

*   positions,
*   animations logiques,
*   état des armes,
*   score,
*   état de l’arène.

Le rendu ne doit pas recalculer les règles métier.  
Il doit uniquement **lire** l’état.

***

## Étape 7 : rendu

Le module `render` convertit les données du jeu en affichage :

*   sprites,
*   rectangles,
*   couleurs,
*   caméra,
*   UI visuelle.

Cette couche peut utiliser SDL.

Le rendu ne doit pas modifier la logique du jeu.

***

## Étape 8 : fin de frame

Le moteur :

*   présente l’image,
*   met à jour le temps,
*   prépare la frame suivante,
*   vérifie si le programme doit continuer.

Puis il recommence.

***

# 11) Ce que signifie “encapsulation correcte” dans ce projet

Une architecture est correctement encapsulée si :

1.  le gameplay peut être compris sans SDL,
2.  `input` est exprimé en termes métier,
3.  `engine.h` n’expose aucun type SDL,
4.  les headers publics n’exposent pas les détails internes,
5.  les sous-modules gameplay ne sont pas appelés directement par `main`,
6.  le rendu ne dépend pas des headers privés gameplay,
7.  les utilitaires restent génériques,
8.  les dépendances suivent un sens lisible et justifié.

***

# 12) Erreurs architecturales typiques à éviter

Voici les fautes classiques qu’il faut éviter :

***

## 12.1. Mettre SDL dans un header public

Exemple de faute :

*   inclure SDL dans `engine.h`
*   mettre `SDL_Scancode` dans une structure publique

Conséquence :

*   SDL fuit dans tout le projet

***

## 12.2. Faire dépendre `game` de `engine`

C’est une inversion de dépendance.

Le jeu ne doit pas connaître le moteur.

***

## 12.3. Faire dépendre `input` de SDL

Dans ce cas, le module `input` n’est plus une abstraction métier.

***

## 12.4. Faire appeler `combat_*` directement depuis `main`

Cela casse la hiérarchie du projet.  
`main` ne doit pas manipuler les sous-détails internes du gameplay.

***

## 12.5. Inclure `*_internal.h` depuis un autre module

Cela contourne l’API publique et casse l’encapsulation.

***

## 12.6. Mettre des headers privés dans `include/`

Si un header interne est placé dans l’espace public, l’architecture devient ambiguë.

***

# 13) Version finale des règles du projet

Je te propose maintenant une formulation nette, exploitable telle quelle.

***

## Règle 1 — séparation des responsabilités

Chaque module doit avoir une responsabilité claire :

*   `main` orchestre,
*   `engine` pilote,
*   `render` affiche,
*   `input` abstrait les commandes,
*   `game` et ses sous-modules appliquent les règles du jeu,
*   `utils/core` fournissent des briques génériques.

***

## Règle 2 — gameplay sans SDL

Aucun module de gameplay ne doit dépendre de SDL :

*   `game`
*   `arena`
*   `combat`
*   `ai`

***

## Règle 3 — input sans SDL

Le module `input` doit rester indépendant de SDL et ne manipuler que des abstractions métier.

***

## Règle 4 — SDL confiné

SDL est autorisé uniquement dans les couches techniques :

*   `engine` interne,
*   `render`,
*   éventuellement une couche plateforme dédiée.

***

## Règle 5 — headers publics propres

Les headers publics :

*   n’incluent pas SDL,
*   n’exposent pas de types SDL,
*   n’incluent pas de headers privés,
*   ne contiennent que l’API officielle du module.

***

## Règle 6 — headers privés confinés

Les headers privés :

*   restent dans `src/`,
*   ne sont pas installés dans `include/`,
*   ne sont inclus que par leur module propriétaire.

***

## Règle 7 — `main` reste haut niveau

`main` ne doit pas inclure ni appeler directement :

*   `arena`
*   `combat`
*   `ai`
*   headers privés

***

## Règle 8 — `engine` pilote sans casser l’encapsulation

`engine` peut coordonner `game`, `render`, `input`, SDL,  
mais ne doit pas inclure les détails internes privés du gameplay.

***

## Règle 9 — `render` observe, ne gouverne pas

`render` lit l’état du jeu pour l’afficher,  
mais ne doit pas contenir la logique métier ni dépendre des headers privés gameplay.

***

## Règle 10 — `utils` et `core` restent neutres

Ces modules ne doivent dépendre ni de SDL, ni des modules applicatifs.

***

# 14) Conclusion

L’architecture correcte de ton projet repose sur une idée simple mais très forte :

> **Le gameplay est pur, le moteur est technique, et SDL doit rester caché.**

Concrètement :

*   les modules métier (`game`, `arena`, `combat`, `ai`, `input`) restent indépendants,
*   les modules techniques (`engine`, `render`) encapsulent la plateforme,
*   les headers publics restent propres,
*   les headers privés restent confinés,
*   les dépendances suivent un sens maîtrisé.

C’est cette discipline qui permet d’avoir :

*   un projet lisible,
*   un découpage propre,
*   une bonne justification en soutenance,
*   et un script d’architecture cohérent.
