---
stepsCompleted: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]
inputDocuments: []
documentCounts:
  briefs: 0
  research: 0
  brainstorming: 0
  projectDocs: 0
workflowType: 'gdd'
lastStep: 14
project_name: 'jeux IA'
user_name: 'Hicham'
date: '2026-03-03'
game_type: 'fighting'
game_name: 'Blade Rush'
status: 'Complete'
---

# {{game_name}} - Game Design Document

**Author:** {{user_name}}
**Game Type:** {{game_type}}
**Target Platform(s):** {{platforms}}

---

## Executive Summary

### Game Name

**Blade Rush**

### Core Concept

**Blade Rush** est un jeu de combat d'escrime compétitif 1v1 en 2D inspiré de Nidhogg. Deux combattants s'affrontent dans des duels rapides et intenses où chaque seconde compte. Le joueur contrôle un escrimeur armé d'une épée, face à un adversaire (IA ou joueur humain), dans un objectif simple mais addictif : atteindre l'extrémité opposée du niveau.

Le gameplay se concentre sur des mécaniques d'escrime tactiques avec un système de hauteur d'épée (haute, moyenne, basse) permettant attaques, parades et désarmements. Chaque mort entraîne un respawn immédiat, créant un rythme frénétique de type "tug-of-war" territorial. Le joueur doit maîtriser le timing précis, les réflexes rapides et la lecture de l'adversaire pour progresser vers la victoire.

Développé en C avec SDL3 pour le rendu 2D dans le cadre d'un projet scolaire, **Blade Rush** vise à capturer l'essence minimaliste et compétitive de Nidhogg tout en intégrant une intelligence artificielle challengeante comme adversaire principal.

### Game Type

**Type:** Fighting
**Framework:** Ce GDD utilise le template Fighting avec des sections spécifiques pour : combat 1v1, mécaniques de combos, système défensif, et design compétitif.

### Target Audience

{{target_audience}}

### Unique Selling Points (USPs)

{{unique_selling_points}}

---

## Target Platform(s)

### Primary Platform

**PC (Windows/Linux)**

### Platform Considerations

- **Language & Framework:** Développé en C avec SDL3 pour le rendu 2D
- **Performance Target:** 60 FPS constant pour garantir la réactivité du combat
- **Resolution Support:** 720p minimum, 1080p recommandé, support multi-résolution
- **Distribution:** Exécutable standalone pour projet scolaire, potentiellement itch.io pour partage

### Control Scheme

**Primary Input: Keyboard**
- Touches directionnelles : Déplacement gauche/droite, saut
- Touches d'action : Attaque, changement hauteur d'épée, parade
- Support contrôleur (optionnel) : Possible extension future

---

## Target Audience

### Demographics

**Âge:** 13+ ans
**Type:** Étudiants et joueurs casual-core cherchant du fun rapide et compétitif

### Gaming Experience

**Core gamers** - Joueurs avec expérience moyenne des jeux vidéo, familiers avec les contrôles clavier de base et les mécaniques de combat simples.

### Genre Familiarity

Pas nécessairement experts en Fighting Games traditionnels. L'audience apprécie les jeux de combat accessibles (comme Nidhogg, Duck Game, Stick Fight) où le skill ceiling est élevé mais la courbe d'apprentissage est rapide.

### Session Length

**Sessions courtes à moyennes (5-30 minutes)** - Matches rapides permettant plusieurs rounds consécutifs ou des sessions "best of" entre amis.

### Player Motivations

- **Compétition skill-based:** Tester ses réflexes et sa lecture d'adversaire
- **Accessibilité immédiate:** "Easy to learn, hard to master"
- **Défi de l'IA:** Progresser contre une intelligence artificielle challengeante
- **Gameplay pur:** Focus sur les mécaniques sans distractions visuelles excessives
- **Satisfaction instantanée:** Parties rapides avec feedback immédiat

---

## Goals and Context

### Project Goals

1. **Performance & Optimisation Technique**
   - Mesurer et optimiser la complexité algorithmique du code
   - Monitorer l'utilisation mémoire en temps réel
   - Maintenir 60 FPS constant sur hardware moyen
   - Documenter les benchmarks et métriques de performance

2. **Implémentation IA Avancée**
   - Développer une IA adverse utilisant l'algorithme MinMax
   - Créer des niveaux de difficulté scalables
   - Analyser et optimiser les performances de l'IA

3. **Génération Procédurale de Maps**
   - Rechercher et implémenter des algorithmes de génération procédurale
   - Créer des arènes variées avec contraintes de gameplay équilibrées
   - Garantir la rejouabilité avec maps uniques

4. **Apprentissage Technique**
   - Maîtriser la programmation en C bas niveau
   - Explorer SDL3 pour le rendu 2D performant
   - Appliquer les concepts d'IA de jeu en pratique

### Background and Rationale

**Blade Rush** est développé dans le cadre d'un projet scolaire axé sur l'intelligence artificielle et la programmation de jeux. L'objectif est d'explorer les concepts fondamentaux de l'IA de jeu (algorithme MinMax) tout en maîtrisant les technologies bas niveau (C, SDL3).

Le choix de recréer un jeu style Nidhogg permet de se concentrer sur des mécaniques de combat pures et d'IA tactique sans la complexité de systèmes additionnels. L'ajout de la génération procédurale de maps et des systèmes de progression (momentum, épée évolutive) apporte une dimension de recherche et d'innovation au projet.

Ce projet vise à démontrer la capacité à construire un jeu complet de A à Z en C, avec une attention particulière aux performances, à l'architecture logicielle et à l'implémentation d'algorithmes d'IA classiques.

---

## Unique Selling Points (USPs)

### 1. Génération Procédurale de Maps

Contrairement à Nidhogg qui utilise des niveaux fixes, **Blade Rush** génère des arènes procéduralement. Chaque partie offre une configuration de niveau unique avec plateformes, obstacles et zones de spawn variées, garantissant une rejouabilité infinie.

### 2. Système de Momentum Dynamique

Le jeu récompense l'agressivité et la maîtrise avec un système de momentum :
- Plus le joueur attaque et avance sans mourir, plus sa **vitesse de déplacement** et sa **vitesse d'attaque** augmentent progressivement
- Crée un effet "snowball" risk/reward qui intensifie les duels
- Reset à la mort, maintenant l'équilibre compétitif

### 3. Épée Évolutive Contextuelle

L'arme évolue dynamiquement selon les performances :
- **3 parades réussies** → Portée de l'épée augmente de 10%
- **Mort** → Portée revient à la normale
- Encourage une approche tactique mixant offense et défense

### 4. IA MinMax Challengeante

Adversaire IA utilisant l'algorithme MinMax pour prendre des décisions tactiques :
- Évalue les positions, attaques et mouvements optimaux
- Niveaux de difficulté ajustables (profondeur de l'arbre de recherche)
- Offre un défi constant pour le joueur solo

### Competitive Positioning

**Blade Rush** se positionne comme une évolution technique et mécanique de Nidhogg, conçue pour un public intéressé par :
- Le combat skill-based avec couches de profondeur supplémentaires
- La rejouabilité via génération procédurale
- Un défi solo contre IA (vs focus multijoueur de Nidhogg)
- Une approche technique/académique du game design

Le projet démontre qu'un jeu peut être à la fois un exercice technique rigoureux et une expérience de jeu fun et innovante

---

## Core Gameplay

### Game Pillars

1. **Mastery & Skill**
   - Le jeu récompense la pratique, l'amélioration des réflexes et la compréhension profonde des mécaniques
   - Skill ceiling élevé permettant une progression continue
   - Les victoires sont le résultat de la maîtrise, pas de la chance

2. **Tactical Combat**
   - Chaque échange requiert lecture de l'adversaire, timing précis et positionnement stratégique
   - Système de hauteur d'épée (haute/moyenne/basse) créant un rock-paper-scissors tactique
   - Les parades et contre-attaques sont aussi importantes que l'offense

3. **Risk/Reward**
   - Le système de momentum encourage l'agressivité mais avec conséquences en cas d'échec
   - L'épée évolutive récompense le jeu défensif (parades) tout en créant des avantages temporaires
   - Chaque décision comporte un risque calculé

4. **Infinite Replayability**
   - Maps générées procéduralement garantissent que chaque partie est unique
   - Système de rounds paramétrable offrant des sessions courtes ou longues
   - L'IA MinMax s'adapte et crée des défis constants

**Pillar Prioritization:** En cas de conflit entre piliers, prioriser dans cet ordre :
1. Tactical Combat (le cœur du jeu)
2. Mastery & Skill (la progression du joueur)
3. Risk/Reward (la profondeur stratégique)
4. Infinite Replayability (la longévité)

### Core Gameplay Loop

**Micro-Loop (Engagement individuel - 5-15 secondes) :**
```
Approche adversaire → Lecture position/hauteur épée → Attaque/Parade/Feinte → 
Gagne/perd l'échange → Ajuste stratégie → Réengage
```

**Macro-Loop (Round complet - 1-5 minutes) :**
```
Spawn sur map procédurale → Engage adversaire → Échange tactical → 
Mort/Kill (reset momentum/épée) → Respawn → Progression territoriale → 
Atteindre goal adverse (victoire round) OU adversaire atteint ton goal (défaite round)
```

**Meta-Loop (Match complet - 5-30 minutes) :**
```
Configure rounds (1 à ∞) → Génère nouvelle map → Joue rounds → 
Gagne/perd match → Rematch sur nouvelle map → Répète
```

**Loop Timing:** 
- Micro-loop : 5-15 secondes par engagement
- Macro-loop : 1-5 minutes par round
- Meta-loop : 5-30 minutes par match (selon configuration rounds)

**Loop Variation:** Chaque itération est différente grâce à :
- Maps générées procéduralement (layout, plateformes, obstacles uniques)
- État de momentum dynamique (vitesse/attaque variables)
- État épée évolutive (portée variable selon parades)
- Décisions tactiques de l'IA MinMax (imprévisibilité stratégique)
- Progression territoriale (spawn positions changent selon contrôle)

### Win/Loss Conditions

#### Victory Conditions

**Victoire de Round :**
- Atteindre la **zone de goal adverse** (extrémité opposée du niveau)
- Système "tug-of-war" territorial : progression jusqu'au bout

**Victoire de Match :**
- Gagner le **nombre de rounds configuré** dans le menu
- Options : 1 round, Best of 3, Best of 5, Best of 7, 10, 20, ou mode Infini
- Mode Infini : jouer jusqu'à abandon volontaire du joueur

#### Failure Conditions

**Défaite de Round :**
- L'adversaire atteint **ta zone de goal** (ton extrémité)
- Perd le contrôle territorial complet

**Défaite de Match :**
- L'adversaire gagne le nombre de rounds requis avant toi

#### Failure Recovery

**Sur Mort Individuelle (pendant un round) :**
- **Respawn immédiat** sur la dernière zone contrôlée ou point de spawn
- **Reset Momentum** : vitesse déplacement et attaque reviennent à normale
- **Reset Épée** : portée revient à la normale
- **Perte territoriale** : l'adversaire gagne du terrain (spawn point se déplace)
- **Apprentissage** : Observer pattern de l'IA, ajuster hauteur épée et timing

**Sur Défaite de Round :**
- Nouvelle map générée procéduralement
- Tous les stats reset (momentum, épée)
- Score du match mis à jour
- Continue jusqu'à victoire/défaite du match

**Philosophie de l'Échec :**
La mort est fréquente mais **jamais punitive** au point de décourager. Elle enseigne :
- Quel pattern d'attaque l'IA utilise
- Quelles hauteurs d'épée sont efficaces
- Timing optimal pour parades vs attaques
- Gestion du momentum (quand être agressif vs prudent)

---

## Game Mechanics

### Primary Mechanics

#### 1. Mouvement Horizontal
- **Avancer/Reculer** : Déplacement gauche/droite sur la map
- **Quand utilisé** : Constamment, pour positionner et progresser/reculer
- **Compétences testées** : Positionnement, lecture d'espace, gestion distance
- **Progression** : Influencé par le système Momentum (plus rapide si momentum actif)
- **Interactions** : Combiné avec Changement Hauteur Épée pour attacks positionnelles

#### 2. Attaque à l'Épée
- **Action** : Frapper l'adversaire avec l'épée
- **Quand utilisé** : Situationnellement, quand à distance d'attaque
- **Compétences testées** : Timing, lecture adversaire, distance management
- **Progression** : Vitesse augmente avec Momentum ; portée augmente avec Épée Évolutive
- **Interactions** : Peut être bloqué par Parer ; hauteur épée compte pour efficacité

#### 3. Parer (Défense Active)
- **Action** : Bloquer l'attaque adversaire avec l'épée
- **Quand utilisé** : En réaction aux attaques
- **Compétences testées** : Réflexes, timing de parade, anticipation
- **Progression** : Chaque 3 parades réussies → Épée Évolutive (+10% portée)
- **Interactions** : Réinitialise Momentum si enchâîné trop longtemps ; récompense défense tactique

#### 4. Changer Hauteur d'Épée (Haute/Moyenne/Basse)
- **Action** : Ajuster la position verticale de l'épée (3 niveaux)
- **Quand utilisé** : Avant chaque engagement, pour adapter à hauteur attaque adversaire
- **Compétences testées** : Lecture adversaire, anticipation, rock-paper-scissors tactique
- **Système** : 
  - Haut bloque attaques hautes, faible contre basses
  - Moyen bloque moyen, équilibré
  - Bas bloque attaques basses, faible contre hautes
- **Interactions** : Critique pour la defensive strategy; combiné avec Attaque

#### 5. Sauter (Manuel pour Joueur, Réactif pour IA)
- **Action** : Saut vertical manuel pour joueur
- **Quand utilisé** : Pour franchir gaps/trous ; passer au-dessus adversaire
- **Compétences testées** : Timing, positionnement aérien
- **Mécanique IA** : Saut automatique déclenché par :
  - Avance + trou devant → Saute automatiquement
  - Avance + joueur juste devant → Saute automatiquement
  - Recule + trou derrière → Bloqué, ne recule pas
- **Important** : N'est PAS une action dans l'arbre MinMax de l'IA

#### 6. Momentum (Système Passif)
- **Description** : Plus le joueur attaque ou avance sans mourir, plus il devient rapide
- **Effets** :
  - +Vitesse de déplacement progressive
  - +Vitesse d'attaque progressive
  - Crée un "snowball effect" qui récompense agressivité
- **Reset** : À la mort, momentum revient à 0
- **Pilier Servi** : Risk/Reward (agressivité récompensée mais risquée)

#### 7. Épée Évolutive (Système Passif)
- **Description** : L'arme évolue selon les performances en défense
- **Mécaniques** :
  - **3 parades réussies** → Portée épée +10%
  - **Mort** → Portée revient à normale
- **Pilier Servi** : Risk/Reward (récompense jeu défensif tactique)

### Mechanic Interactions

**Défense Tactique** : Hauteur épée + Parer crée un système rock-paper-scissors. L'adversaire doit anticiper ta hauteur pour attaquer efficacement.

**Positionnement + Momentum** : Avancer avec Momentum te rend plus rapide ET plus fort, créant une zone de danger progressive.

**Distance + Attaque** : Attaque n'est efficace que si dans range. Momentum rend les joueurs plus motivés à engager, ce qui donne du depth tactique.

**Défense Récompensée** : Parer beaucoup → Épée évolue → Plus grande portée → Peut engager de plus loin, récompensant les joueurs défensifs.

---

## Controls and Input

### Control Scheme (PC - Clavier)

| Action | Touche Primaire | Description |
|--------|-----------------|-------------|
| **Avancer (Gauche)** | `A` ou `←` | Déplace le joueur vers la gauche |
| **Reculer (Droite)** | `D` ou `→` | Déplace le joueur vers la droite |
| **Attaquer** | `J` | Frappe avec l'épée |
| **Parer** | `SHIFT` | Bloque l'attaque adversaire |
| **Hauteur Haut** | `W` ou `↑` | Élève l'épée en position haute |
| **Hauteur Bas** | `S` ou `↓` | Abaisse l'épée en position basse |
| **Hauteur Milieu** | Relâcher `W`/`S` | Épée en position médiane |
| **Sauter** | `SPACE` | Saut vertical (joueur manuel uniquement) |

### Input Feel

- **Responsivity** : Tous les inputs doivent avoir feedback immédiat (FPS constant 60 pour réactivité)
- **Timing Precision** : Parade et Attaque nécessitent timing exact (fenêtres frame-perfect)
- **Queuing** : Inputs peuvent être buffered pour permettre input spacing naturel
- **Air Control** : Joueur peut contrôler direction saut en l'air

### Accessibility Controls

- **Rebindable Keys** : TOUS les contrôles peuvent être remappés dans le menu Paramètres
- **Preset Profiles** : Options de layouts prédéfinis (QWERTY, AZERTY, Contrôleur alternatif, etc.)
- **Timing Windows** : Options pour augmenter/diminuer la fenêtre timing des parades (facile → difficile)
- **Visual Feedback** : Feedback visuel et sonore sur chaque action (hit, block, momentum gain)

### Planned Extensions (Future)

- **Support Manette** : À implémenter dans une version ultérieure
- **Mode Joueur vs Joueur (Local)** : À implémenter après version solo stable

---

## Fighting Game Specific Design

### Character Roster

**Play Style Complexity:**

- **Single Player Character**
  - Simplifié pour l'apprentissage
  - Focus sur mécaniques d'escrime pures
  - Pas de variation archetype (rushdown, zoner, etc.)

- **IA Opponent**
  - Contrôlée par algorithme MinMax
  - Adaptative à 4 niveaux de difficulté
  - Comportement prévisible mais challenging

**Balance Philosophy:** 
Pas de tier system traditionnel. L'équilibre vient des **niveaux de difficulté IA** qui modulent agressivité, profondeur de prévision et errance aléatoire.

---

### Move Lists and Frame Data

**Simplified Combat System:**

Blade Rush utilise un système d'escrime tactique plutôt que des mouvements spécialisés complexes.

#### Normal Attacks

- **Attack** (Épée frontale)
  - Startup : ~6 frames
  - Active : 4 frames
  - Recovery : 10 frames
  - Range : Affecté par Épée Évolutive (+10% max)
  - Damage : 1 hit = 1 mort (binaire)
  - Speed : Affectée par Momentum (jusqu'à 20% plus rapide)

#### Defensive Moves

- **Parry** (Parade active)
  - Startup : 2 frames (réactif)
  - Active : Until release-input
  - Recovery : 4 frames
  - Coverage : Dépend de hauteur épée (Haut/Moyen/Bas)
  - Reward : Counter-hit possible ; +1 parade counter

#### Height Adjustment

- **Change Épée Height** (Haut/Moyen/Bas)
  - Startup : 1 frame (instantané)
  - No recovery penalty
  - Rock-Paper-Scissors efficacité :
    - Haut bat Bas
    - Bas bat Moyen
    - Moyen bat Haut
    - Quand hauteur match : parade efficace

#### No Special Moves

Blade Rush évite :
- Quarter-circle inputs (complexe pour escrime)
- Super/ultimate moves (clash avec simplicité Nidhogg)
- Charge attacks (trop lent pour tempo rapide)

---

### Combo System

**Escrime Tactique vs Traditional Combos**

Blade Rush n'a **pas de combo system traditionnel** (cancels, links, chains). À la place :

#### Engagement Chains

**Échanges successifs** plutôt que combos:

1. **Initiation** : Approche adversaire à range d'attaque
2. **Engagement** : Attaque ou finte hauteur épée
3. **Response** : Adversaire pare/esquive ou se fait toucher
4. **Counter-Attack** : Si parade réussie, opportunité de countre-attaque
5. **Reset** : Repositionnement, recommence

#### Damage Model

- **Single Hit KO** : Touche = mort immédiate
- **No Health Bar** : Combat binaire (vivant/mort)
- **No Damage Scaling** : Pas de réduction dégâts prolongés
- **Simplicity** : Garde focus sur tactique, pas sur optimal combos

#### Momentum Chains

L'aspect "combo-like" vient du **Momentum** :
- Chaque hit sans réciproquité renforce le joueur
- Vitesse +X% accumule sans cap linéaire
- Create psychological "snowball" effect

---

### Defensive Mechanics

#### Parry System (Cœur de la Défense)

**Height-Based Matching:**

| Attaque Height | Parry Height | Résultat |
|---------------|--------------|---------| 
| Haut | Haut | **Effective** ✓ (counter possible) |
| Haut | Moyen | Partielle (gap en haut) |
| Haut | Bas | **Échoue** ✗ (hit clean) |
| Moyen | Moyen | **Effective** ✓ |
| Moyen | Haut/Bas | Partielle |
| Bas | Bas | **Effective** ✓ |
| Bas | Moyen | Partielle |
| Bas | Haut | **Échoue** ✗ |

#### Defensive Progression

- **Parry Counter** : Après 3 parades réussies → Épée Évolutive (+10% portée)
- **No Guard Break** : Blocage infini (pas de chip damage)
- **No Invincibility Frames** : Pendant parade, joueur vulnérable si bad spacing

#### Avoidance

- **Mouvement** : Se déplacer crée gap temporel
- **Saut** : Esquive verticale, passe adversaires
- **No Backdash I-frames** : Reculer = simplement s'éloigner

---

### Stage Design

#### Procedural Map Generation

**Blade Rush génère dynamiquement chaque map avec :**

- **Terrain Variations** :
  - Platforms de différentes hauteurs
  - Gaps/trous (sans endroits inaccessibles)
  - Obstacles visuels (pas hit-box blocking)
  - Walls (boundaries)

- **Spawn Points** :
  - Joueur spawn gauche, IA spawn droite
  - Distance initiale : paramétrable
  - Safe zones (pas hit-box initialement)

- **Walking Distance** :
  - Maps de taille variable (50-200 units proposé)
  - Plus court = matches rapides (5-30s)
  - Plus long = matches tactiques (1-5 min)

#### No Complex Stage Mechanics

- No "stage hazards" (initial MVP)
- No "environmental kills"
- No "interactive elements"
- Focus sur purity of combat

---

### Single Player Modes

#### Arcade Mode (Planned Future)

- Serie de 5 matches vs IA
- Niveaux progressifs (Easy → Très Difficile)
- Endings personnalisés par victoire

#### Training Mode

- **Free Practice** :
  - Joueur vs IA statique (pause possible)
  - IA ne bouge pas/attaque pas
  - Lab des matchups

- **Recorded Playback** :
  - Post-match replay (optionnel)
  - Analyze IA patterns

#### Main Mode for MVP

**Endless Matches contre IA Configurable:**

- Sélectionner difficulté IA (4 niveaux)
- Sélectionner nombre de rounds (1 à ∞)
- Générer map procédurale
- Play match
- Afficher stats (kills, rounds won, items sur momentum max)

---

### Competitive Features - IA MinMax Architecture

**Blade Rush utilise un système IA stratégique basé sur MinMax avec adaptation de difficultés.**

#### Core State Representation (Discrétisé)

L'IA décide ALL 100ms basé sur cet état discrétisé :

```
{
  pos_bot, pos_player,          // Positions absolues
  distance,                      // Discrétisé: PROCHE/MOYEN/LOIN
  dir_bot, dir_player,          // Direction facing
  posture_bot, posture_player,  // Hauteur épée: HAUT/MOYEN/BAS
  advantage,                     // Qui a hit last: OUI/NON
  has_sword_bot, has_sword_player, // Possède épée: OUI/NON
  sword_distance,               // Range: NORMAL/+10%
  danger_bot, danger_player,    // Near hole: PROCHE/SAFE
  height_diff,                  // Difference terrain
  timer                         // Round timer
}
```

#### Available MinMax Actions

**L'IA choisit UNE action chaque décision tick :**

1. **Avancer** (towards player)
2. **Reculer** (away from player)
3. **Attaquer** (if in range)
4. **Parer + Hauteur** (Parer + set hauteur: HAUT/MOYEN/BAS)
5. **Changer Hauteur Seulement** (no parry, juste positional setup)

**Saut** = PAS une action MinMax (automatique reactif)

#### Heuristic Scoring

**L'IA évalue chaque action avec scores cohérents :**

```
Score = Base Heuristic + Tactical Bonus ± Random Noise

Base Heuristic :
  +100  si touche le joueur
  -100  si se fait toucher
  +20   si gagne distance quand doit avancer
  -20   si se met en danger (trou proche)
  -200  si tombe dans un trou
  +15   si engage à range optimal
  -30   si reculer expose le dos au danger

Tactical Bonus (basé sur gamestate) :
  +25   si joueur à portée attaque optimale
  -40   si joueur vient de faire hit (momentum malus)
  +30   si IA a avantage (height match)
  -50   si joueur a momentum fort
```

#### Action Filtering (Reduce Branching)

**Avant MinMax, filtrer les actions absurdes :**

- **Pas parer si distance LOIN** (inutile)
- **Pas attaquer si distance > attack range**
- **Pas reculer si trou derrière** (collision)
- **Pas attaquer si pas d'épée** (sanity check)

Après filtrage : **3-5 actions réelles** évaluées par MinMax.

#### MinMax avec Alpha-Beta Pruning

**Algorithm :**

```
function minimax(state, depth, alpha, beta, isMaximizing):
    if depth == 0 or isTerminal(state):
        return evaluate(state)
    
    if isMaximizing:  // IA maximizes son score
        for each action in filtered_actions:
            child_state = apply(action, state)
            value = minimax(child_state, depth-1, alpha, beta, false)
            alpha = max(alpha, value)
            if beta <= alpha: break  // Prune
        return alpha
    else:  // Joueur minimizes IA score
        for each action in player_actions:
            child_state = apply(action, state)
            value = minimax(child_state, depth-1, alpha, beta, true)
            beta = min(beta, value)
            if beta <= alpha: break  // Prune
        return beta
```

**Effect** : Explore 10× moins d'états → Décision rapide (<16ms pour 60fps)

#### Randomness in AI Decision

**5-10% aléatoire pour naturalité :**

```
if random() < randomness_chance:
    chosen_action = random_action_from_filtered
else:
    chosen_action = minimax_best_action
```

Casse les patterns trop robot, rend IA "humaine".

---

#### Four Difficulty Levels

L'IA a 4 niveaux ajustables via **3 paramètres** :

| Niveau | Profondeur | Randomness | Filtrage | Heuristique |
|--------|------------|------------|----------|-------------|
| **FACILE** | 1 | ±20% | Loose (ignore parfois better action) | Standard |
| **NORMAL** | 2 | ±10% | Standard | Standard |
| **DIFFICILE** | 3 | ±5% | Strict (retire inutiles) | Slight aggro |
| **TRÈS DIFFICILE** | 3 | 0% | Aucune (toutes actions) | Aggressive |

**Parameter Definitions :**

1. **Profondeur MinMax** (turns ahead predicted)
   - Facile: See 1 turn ahead (immediate threats)
   - Normal: See 2 turns ahead (react + anticipate)
   - Difficile: See 3 turns ahead (plan chains)
   - Très Difficile: 3 turns + pattern learning

2. **Randomness** (aléatoire dans heuristique)
   - Facile: ±20% variation → Débutant imprévisible
   - Normal: ±10% variation → Compétent
   - Difficile: ±5% variation → Expert cohérent
   - Très Difficile: 0% → Parfait minmax

3. **Action Filtering** (strictness)
   - Facile: Peut faire actions under-optimal parfois
   - Normal: Filtre standard (pas absurde)
   - Difficile: Filtre strict (actions efficientes uniquement)
   - Très Difficile: Aucun filtre (explore tout MinMax)

#### Behavioral Outcomes

**Chaque niveau crée une « personnalité » différente :**

- **Facile** : IA fait erreurs, joueur gagne facilement, bon pour apprendre patterns
- **Normal** : IA compétente mais pas parfaite, equivalent joueur intermédiaire
- **Difficile** : IA anticipe bien, require joueur master des mechanics
- **Très Difficile** : IA joue parfait, lose é impossible (skill test pur)

#### Balancing the IA

**Si IA trop strong même à Facile :** Augmente `randomness` ou réduit `depth`
**Si IA trop faible même à Très Difficile :** Réduit `randomness` ou augmente `depth`

Le système est **flexible et testable**.

---

### Summary: No Traditional Competitive Mode (MVP)

- **Pas de matchmaking ranked** (solo seulement)
- **Pas de replay system** (potential future)
- **Pas de netcode** (local only)
- **Single Player vs 4-Level IA** = Full competitive content pour MVP

---

## Progression and Balance

### Player Progression

**Skill-Based Learning:**

Blade Rush n'a **aucun système de progression persistante** (pas d'XP, d'unlocks, de stats). La progression est **purement skill-based** :

1. **Understand Movement & Distance** : Apprendre les ranges, timing, positionnement
2. **Master Height System** : Comprendre rock-paper-scissors (Haut/Moyen/Bas)
3. **Attack Timing** : Quand attaquer, anticipation patterns IA
4. **Parry Mastery** : Hauteur matching, réflexes, contre-attaques
5. **Momentum Management** : Quand être agressif vs cautious

**Pacing:** 
- Chaque match enseigne les patterns IA
- Skill décision vient après 10-20 matches
- Master play après 50+ matches (dépend joueur)

**No Meta Progression:**
- Pas de sauvegarde stats (stats affichées post-match seulement)
- Pas d'unlockables
- Chaque session est fraîche

### Difficulty Curve

**Four Selectable Difficulty Levels:**

Joueur choisit directement le niveau désiré. Pas de progression forcée.

| Niveau | Profondeur | Randomness | Joueur Idéal |
|--------|------------|------------|-------------|
| **FACILE** | 1 turn | ±20% | Apprendre mechanics |
| **NORMAL** | 2 turns | ±10% | Joueur compétent |
| **DIFFICILE** | 3 turns | ±5% | Joueur expert |
| **TRÈS DIFFICILE** | 3 turns | 0% | Skill test pur |

**Difficulty Progression:**
- **Pas de difficulty spike** : Chaque niveau est sélectionnable indépendamment
- **Smooth skill curve** : Chaque niveau 20% plus challengeant que précédent
- **Map variation** : Seule variante via génération procédurale de maps

### Economy and Resources

_This game does not feature an in-game economy or resource system._

### Performance Metrics

#### Time Tracking

Blade Rush mesure le **temps par round** comme métrique de skill:

- **Round Time** : Durée du round en secondes
- **Best Time** : Meilleur temps jamais réalisé (par level)
- **Average Time** : Moyenne des 10 derniers rounds

**Display:**
- Affichée dans stats post-match
- Permet auto-assessment de progression

#### Stat Tracking (Display Only)

Stats affichées post-match, NOT sauvegardées:
- Rounds won/lost
- Kills count
- Max momentum reached
- Round time
- IA difficulty chosen

**No Persistent Leaderboards (MVP)** - Peut être future feature

---

## Level Design Framework

### Structure Type

Blade Rush utilise une structure **Arena/Match procédurale** orientée duel 1v1. Il n’y a pas de campagne de niveaux fixes : chaque match se déroule dans une arène générée avant le début de la partie.

Le système retenu est **Pre-match generation** :
- L’arène complète est générée en une fois avant le combat
- Aucun streaming en cours de match
- Terrain stable pour l’IA Min-Max et pour la lisibilité joueur
- Validation d’équité possible avant lancement

### Level Types

Le jeu utilise des **variations contrôlées** via des archétypes d’arènes.

Archétypes MVP :
- **Small arena** : courte, rythme agressif, peu de trous
- **Medium arena** : format standard équilibré
- **Large arena** : plus longue, jeu de spacing et de tempo
- **Obstacle arena** : densité d’obstacles plus élevée

Les joueurs peuvent sélectionner des **maps de base** dans les paramètres (utile pour tests gameplay et QA).

#### Tutorial Integration

Pas de niveau tutoriel dédié dans le MVP. L’apprentissage se fait par onboarding minimal, premières arènes simples (Small/Medium), et répétition rapide des duels.

#### Special Levels

Pas de boss level ni de secret level dans le MVP. Les distinctions viennent des archétypes et de leurs paramètres.

Éléments cosmétiques d’arènes (thèmes/biomes visuels) : **post-MVP**.

### Level Progression

Modèle de progression: **Open Selection + Endless Matches**.
- Tous les matchs sont accessibles immédiatement
- Pas de déblocage d’étapes
- Variété assurée par génération procédurale contrainte

#### Unlock System

MVP: **aucun unlock de level**. Option de sélection d’archétype/map de base via paramètres disponible pour test et préférence joueur.

#### Replayability

La rejouabilité repose sur la génération procédurale pré-match, l’alternance des archétypes, et les variations internes sous contraintes de fairness.

### Level Design Principles

- Générer l’arène en pré-match pour éviter tout lag en duel
- Favoriser lisibilité et équité plutôt que complexité géométrique
- Garantir un terrain exploitable par l’IA locale
- Régénérer immédiatement toute map invalidée

Règles de fairness (MVP) :
1. **Spawn symétriques**
  - Joueur A à gauche, Joueur B à droite
  - Distance équivalente au premier obstacle
  - Aucun trou adjacent au spawn

2. **Segments non injustes**
  - Aucun trou impossible à franchir
  - Aucun obstacle collé à un trou
  - Aucune zone boost/slow sous un spawn

3. **Distribution équilibrée**
  - Nombre de trous plafonné (ex: max 3)
  - Obstacles espacés
  - Zones spéciales réparties de manière régulière

4. **Validation automatique**
  - Traversée complète possible sans mort forcée
  - Symétrie stricte ou équivalence fonctionnelle des côtés
  - Contrôle des longueurs de segments
  - Contrôle de largeur maximale des trous
  - Si échec: régénération

5. **IA-friendly map readability**
  La map doit exposer clairement les informations locales nécessaires:
  - `danger_bot` (safe / near_hole / on_edge)
  - `obstacle_between` (true/false)
  - `zone_type` (normal / boost / slow)

---

## Art and Audio Direction

### Art Style

Blade Rush adopte un **Pixel Art moderne**, lisible et contrasté, optimisé pour un gameplay 1v1 rapide et précis.
Ce style est retenu pour sa production efficace en solo, sa clarté en mouvement, et sa compatibilité performance avec SDL3.

#### Visual References

- **Nidhogg 1** : lisibilité des silhouettes et clarté du duel
- **Samurai Gunn** : impact visuel et minimalisme efficace
- **TowerFall** : lisibilité des personnages et code couleur propre

#### Color Palette

Direction colorimétrique orientée lisibilité compétitive :
- **Arrière-plans désaturés** : gris, bleu foncé, brun
- **Combattants à fort contraste** : rouge vif vs bleu vif
- **Effets d’épée** : blanc/jaune pour ressortir instantanément
- **Zones spéciales** : accents néon légers (cyan/magenta) pour boost/slow

#### Camera and Perspective

- Vue **2D latérale** centrée duel
- Cadrage stable, sans effets visuels intrusifs
- Priorité à la lecture des postures d’épée (haut/mid/bas)

### Audio and Music

Blade Rush suit une direction sonore **électro/synthwave légère**, rythmée mais non envahissante, au service du tempo des matchs.

#### Music Style

- Base MVP : **électro minimaliste** (boucles courtes)
- Variation possible : **synthwave sombre** pour certaines arènes
- Option stylistique future : **chiptune moderne** (hors MVP)

#### Sound Design

SFX style **arcade métallique**, courts et percutants :
- Impact d’épée : métallique sec
- Parade : cling aigu
- Dash/déplacement : woosh léger
- Mort : impact sourd
- Respawn : pop clair

Directives globales :
- sons courts
- attaques lisibles à l’oreille
- feedback immédiat et compétitif

#### Voice/Dialogue

- Pas de dialogue narratif
- **Grunts légers** (attaque, mort)
- **Annonces UI courtes** : “Fight!”, “Point!”, “Victory!” (voix synthétiques/pitchées)

### Aesthetic Goals

- Renforcer la sensation de duel nerveux, propre et compétitif
- Maximiser la lisibilité visuelle et sonore à haute vitesse
- Garder une signature simple mais marquante (silhouettes fines, FX nets, UI sobre)
- Respecter le scope MVP avec une direction forte sans surcharge de production

---

## Technical Specifications

### Performance Requirements

Blade Rush cible une expérience de duel 1v1 ultra-réactive avec priorité absolue à la fluidité et à la lisibilité.

#### Frame Rate Target

- **60 FPS stables obligatoires** (objectif principal MVP)
- **120 FPS optionnel** si le matériel le permet
- **Input latency < 30 ms** (priorité critique)

#### Resolution Support

- **Résolution native : 1280x720 (720p)**
- **Upscale : 1920x1080 (1080p)** via integer scaling
- Modes d’affichage : **fenêtré** + **plein écran borderless**

#### Load Times

Temps de chargement cible au lancement d’un match : **< 2 secondes**, incluant :
- chargement sprites
- chargement SFX
- génération map procédurale
- initialisation IA

### Platform-Specific Details

MVP orienté **PC offline**, sans dépendance réseau.

#### PC Requirements

**Minimum Specs**
- CPU : Dual-core 2.0 GHz
- RAM : 2 Go
- GPU : Intel HD 4000 ou équivalent
- OS : Windows / Linux
- Stockage : < 200 Mo

**Recommended Specs**
- CPU : Quad-core 3.0 GHz
- RAM : 4 Go
- GPU : GTX 750 / RX 460 ou équivalent
- OS : Windows 10/11
- Stockage : < 500 Mo

**Input**
- Clavier obligatoire (ZQSD + flèches)
- Manette optionnelle (XInput)
- Rebind complet : **post-MVP**

#### Build & Distribution

- Binaire local (projet scolaire)
- Package portable : un dossier avec exécutable + assets
- Aucune installation requise
- Distribution optionnelle : itch.io (zip)
- Mode offline only (pas de netcode en MVP)

#### Debug Mode

- Hitboxes visibles
- État IA affiché
- Seed de génération procédurale affichée

### Asset Requirements

Le scope asset est calibré pour un MVP solo performant.

#### Art Assets

**Personnages**
- 2 personnages (base sprite partagée, variantes couleur)
- Animations par posture :
  - Idle (3 frames)
  - Marche (4 frames)
  - Attaque (4 frames)
  - Parade (2 frames)
  - Changement posture (1 frame)
  - Mort (3 frames)
  - Saut (2 frames)
- Total estimé : **~20–25 frames par personnage**

**Arènes procédurales**
- 3 variantes : Small / Medium / Large
- Tiles principales : Sol, Trou, Obstacle, Zone boost, Zone slow
- Total environnement : **10–12 sprites**

**Effets visuels**
- Slash (2 frames)
- Étincelles (2 frames)
- Poussière (2 frames)

**UI**
- Barres simples
- Icônes de posture
- Icône d’avantage

#### Audio Assets

- **SFX : 10–15** (attaque, parade, impact, mort, respawn, pas, saut, zones, UI)
- **Musiques : 2–3** (menu, combat, victoire)

#### External Assets

- Priorité aux assets originaux / libres adaptés au style MVP

### Technical Constraints

1. **Input Latency**
- Lecture input avant toute logique de frame
- Pas de buffering inutile
- VSync non forcé si impact négatif sur la latence

2. **Duel Determinism**
- Physique simple et stable
- États IA discrets
- Pas de RNG dans collisions/combat
- RNG réservé à la génération procédurale via seed fixe

3. **IA Min-Max Budget**
- Temps de calcul cible : **< 1 ms**
- Branching factor réel : **≤ 5**
- Profondeur : **2–3** selon difficulté
- Alpha-beta pruning : **obligatoire**

4. **Runtime Stability**
- Aucune fuite mémoire (SDL + malloc)
- Contrôle de la fragmentation
- Boucle de jeu fixe avec delta time clampé

5. **Build Size**
- Cible MVP : **< 50 Mo**
- Pixel art + OGG + C pour footprint minimal

---

## Development Epics

### Epic Overview

Blade Rush utilise une structure **Parallel Lanes** optimisée pour 2 développeurs (Hicham + Walid) travaillant en parallèle avec des points d'intégration définis.

| # | Epic Name | Lane | Scope | Dependencies | Est. Stories |
|---|-----------|------|-------|--------------|--------------|
| A1 | Core Duel Engine | Lane A (Gameplay) | Mouvement, postures, combat, collisions | Aucune | 8-10 |
| A2 | Match Flow & UX/UI | Lane A (Gameplay) | Menu, HUD, paramètres, scoring | A1 | 6-8 |
| A3 | Art/Audio Integration | Lane A (Gameplay) | Sprites, SFX, musiques, polish | A1, A2 | 5-7 |
| B1 | Arena Procedural Generation | Lane B (Systems) | Générateur, tiles, validation fairness | Aucune | 6-8 |
| B2 | IA Min-Max & Difficulty | Lane B (Systems) | Min-Max, alpha-beta, 4 difficultés | Aucune | 8-10 |
| B3 | Balancing & AI Tuning | Lane B (Systems) | Tuning IA, tests fairness | B1, B2 | 4-6 |

### Recommended Sequence

**Phase 1 — Parallel Development (Sprints 1-3):**
- **Lane A:** Epic A1 (Core Duel Engine)
- **Lane B:** Epic B1 (Arena Generation) + Epic B2 (IA Min-Max)
- **Integration Point 1 (IP1):** Interface Arena contractée
- **Integration Point 2 (IP2):** Interface IA contractée

**Phase 2 — Parallel Polish (Sprints 4-5):**
- **Lane A:** Epic A2 (Match Flow) + Epic A3 (Art/Audio)
- **Lane B:** Epic B3 (Balancing & Tuning)
- **Integration Point 3 (IP3):** MVP complet

**Phase 3 — Merge & Release (Sprint 6):**
- Intégration finale
- QA end-to-end
- Packaging & release MVP

### Vertical Slice

**Premier milestone jouable (après IP2):**
Duel 1v1 humain vs IA (difficulté Normal) sur arène procédurale Small, avec placeholder visuals/audio. Ce vertical slice valide :
- Core loop complet
- IA compétitive fonctionnelle
- Génération procédurale stable
- Interfaces lanes A/B validées

### Integration Points

**IP1 — Arena Interface (après A1 + B1):**
- Contrat: `Arena* generate_arena(ArenaSeed seed, ArchetypeType type)`
- Lane A consomme arènes générées par Lane B
- Test: duel sur arène procédurale

**IP2 — IA Interface (après A1 + B2):**
- Contrat: `Action ai_get_action(GameState* state, DifficultyLevel diff)`
- Lane A intègre IA de Lane B comme adversaire
- Test: duel humain vs IA sur map simple

**IP3 — MVP Complet (après A2/A3 + B3):**
- Merge complet des 2 lanes
- Tests end-to-end
- Build final

Détails complets : voir [epics.md](epics.md)

---

## Success Metrics

### Technical Metrics

Ces métriques garantissent que Blade Rush tourne de manière fluide, stable et propre sur n'importe quel PC étudiant.

#### Performance

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Frame rate | ≥ 60 FPS constant | Monitoring FPS en jeu, aucune chute sous 55 |
| Input latency | < 30 ms | Mesure lecture input → action affichée |
| Temps chargement match | < 2 secondes | Génération arène + chargement assets |
| Temps IA Min-Max | < 1 ms par frame | Profiling toutes difficultés |

#### Stability & Memory

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Memory leaks | 0 fuites | Test 100 rounds consécutifs |
| Crash rate | 0% | Tests QA internes |
| CPU usage | < 40% | Machine minimale (Dual-core 2.0 GHz) |
| RAM usage | < 300 Mo | Monitoring pendant gameplay |

#### Build & Packaging

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Build size | < 50 Mo | Taille finale du package |
| Portability | Exécutable standalone | Test sans installation |
| Compatibility | Windows + Linux | Validation sur les 2 OS |

### Gameplay Metrics

Ces métriques valident que le duel est fun, équilibré, lisible et que l'IA se comporte comme prévu.

#### Match Flow

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Durée moyenne round | 2-3 minutes | Analytics gameplay moyen |
| Temps entre 2 morts | 10-20 secondes | Playtests chronométrés |
| Taux erreurs IA | < 1% | Actions absurdes détectées |

#### AI Difficulty Balance (Winrate cible joueur)

| Difficulty | Target Winrate | Measurement Method |
|------------|----------------|-------------------|
| Facile | ~80% | Test 20 matchs par difficulté |
| Normal | ~50% | Test 20 matchs par difficulté |
| Difficile | ~30% | Test 20 matchs par difficulté |
| Très Difficile | ~10% | Test 20 matchs par difficulté |

#### Combat Balance

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Usage postures équilibré | Aucune > 50% | Stats distribution postures |
| Taux parade réussi IA | ~40% | IA vs joueur, éviter "robotique" |

#### Procedural Arenas

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Taux rejet fairness | < 5% | Maps invalides régénérées |
| Temps génération | < 50 ms | Profiling pré-match |

**Note MVP:** Le système permet de sauvegarder les maps générées procéduralement pour y rejouer (utile pour tests et préférences joueur).

### Qualitative Success Criteria

Ces critères déterminent si le jeu "fonctionne" en tant qu'expérience, même s'ils ne sont pas directement chiffrés.

**Ressenti du duel:**
- "Le duel est nerveux, lisible, impactant"
- "Les postures sont claires et compréhensibles en un coup d'œil"
- "Les impacts d'épée sont satisfaisants (SFX + flash)"

**Intelligence Artificielle:**
- "L'IA Normal est challenging mais pas frustrante"
- "L'IA Difficile semble réfléchir, pas tricher"
- "L'IA ne spamme pas, ne bug pas, ne fait pas d'actions absurdes"

**Arènes procédurales:**
- "Les arènes semblent justes, variées, cohérentes"
- "Aucune arène ne donne un avantage structurel à un joueur"

**Direction artistique & audio:**
- "Le pixel art est clair, contrasté, lisible"
- "Les SFX métalliques donnent une signature sonore"
- "La musique électro/synthwave soutient le rythme sans gêner"

### Metric Review Cadence

- **Pendant développement:** Tests automatisés continus (performance, stabilité)
- **Fin de chaque epic:** Playtests internes pour gameplay metrics
- **Pre-release:** QA complète avec validation de tous les KPIs
- **Post-MVP (optionnel):** Analytics si distribution itch.io

**Synthèse compacte:** Blade Rush est un succès MVP si le jeu tourne à 60 FPS (latency < 30 ms, IA < 1 ms), les rounds durent 2-3 min avec IA Normal ≈ 50% winrate, les arènes sont fair à 95%+, et l'expérience duel est lisible/nerveuse/satisfaisante avec une identité audiovisuelle forte.

---

## Out of Scope

Cette section verrouille ce qui **ne sera pas développé** dans Blade Rush v1.0 MVP. Elle protège le scope, évite la dérive, et clarifie ce qui est réservé au post-MVP.

### Features exclues

- Multijoueur local (2 joueurs sur le même PC)
- Multijoueur online (rollback, matchmaking, netcode)
- Mode spectateur / replay
- Classements en ligne (leaderboards)
- Éditeur d'arènes ou de niveaux
- Système de progression, XP, talents, perks
- Mode histoire / campagne
- IA avancée (apprentissage, comportement adaptatif)

### Modes de jeu exclus

- Mode Arcade
- Mode Survie
- Mode Time Attack
- Mode Tournoi
- Mode "Chaos" avec power-ups

### Contenu artistique exclu

- Skins de personnages
- Variantes visuelles d'arènes (biomes : neige, désert, cyberpunk…)
- Animations complexes (>5 frames par action)
- Cutscenes, intros animées, cinématiques
- Effets de particules avancés (sparks dynamiques, shaders)

### Audio exclu

- Voix narratives longues
- Doublage complet
- Soundtrack dynamique (adaptive music)

### Plateformes exclues

- Console (Switch, PlayStation, Xbox)
- Mobile (Android, iOS)
- Web (WebAssembly)
- Steam Workshop / modding

### Systèmes techniques exclus

- Sauvegarde/chargement de profils
- Cloud save
- Backend serveur
- Analytics en ligne
- Support manette avancé (vibrations, remapping complet)

### Polish exclu

- UI animée complexe
- Transitions cinématiques
- Effets de caméra avancés (shake dynamique, zoom)
- Optimisation extrême (profiling profond)

### Deferred to Post-Launch

Les éléments suivants peuvent être considérés pour des versions futures basées sur le feedback utilisateur et la viabilité du produit :
- Multijoueur local (priorité si succès MVP)
- Éditeur d'arènes communautaire
- Modes de jeu alternatifs (Arcade, Survie)
- Cosmétiques et biomes visuels
- Support console/mobile si demande forte

---

## Assumptions and Dependencies

Cette section clarifie **sur quoi repose le projet**. Si une hypothèse tombe, le planning ou le scope doit être ajusté.

### Key Assumptions

**Hypothèses techniques:**
- SDL3 est stable et suffisante pour le rendu 2D + input + audio
- Le langage C est suffisant pour le gameplay, l'IA et la génération procédurale
- Le jeu tourne sur PC uniquement (Windows/Linux)
- Pas de moteur externe (Unity/Unreal) → pipeline 100% custom
- Pas de physique complexe → collisions simples, hitboxes rectangulaires
- L'IA Min-Max reste sous 1 ms grâce au modèle discret
- La génération procédurale reste simple (segments, règles de fairness)

**Hypothèses équipe:**
- Équipe de 2 personnes (Hicham + Walid) disponible 1 mois
- Répartition claire : Lane A (Hicham) = Gameplay/UI/Intégration, Lane B (Walid) = IA/Génération
- Capacité à travailler en parallèle grâce aux interfaces définies
- Communication quotidienne + intégration Git tous les 2-3 jours

**Hypothèses contenu:**
- Pixel art produit en interne (sprites simples, 3-5 frames)
- SFX métalliques trouvés ou créés (libres de droits)
- Musique électro/synthwave libre ou produite en interne
- Pas de besoin de VO ou d'assets complexes

**Hypothèses gameplay:**
- Le duel reste lisible grâce au pixel art contrasté
- Les 3 postures sont compréhensibles visuellement
- Les arènes procédurales restent fair dans 95% des cas
- L'IA Normal doit être battable par un joueur moyen

**Hypothèses planning:**
- 4 semaines = 7 sprints courts (Scrum Light)
- Vertical Slice jouable fin semaine 2
- Polish + debug semaine 4
- Build final stable avant la soutenance

### External Dependencies

**Outils & librairies:**
- SDL3 + SDL_image + SDL_mixer (rendu, audio)
- GitHub pour versioning + Kanban
- Outils graphiques (Aseprite, Photoshop, Krita)
- Outils audio (Audacity, Bfxr)

**Contraintes:**
- Aucun backend, aucune API externe
- Aucun service cloud
- Pas de dépendance à des assets premium/payants (priorité assets libres)

### Risk Factors

**Risques techniques:**
- SDL3 instable ou bugs critiques → Mitigation: fallback SDL2
- Performance IA > 1 ms → Mitigation: réduire profondeur/branching
- Génération procédurale trop lente → Mitigation: simplifier contraintes

**Risques équipe:**
- Disponibilité réduite d'un membre → Mitigation: sprints flexibles, priorisation core features
- Conflit d'intégration Git → Mitigation: branches claires, sync fréquent

**Risques contenu:**
- Assets pixel art prennent trop de temps → Mitigation: placeholders acceptables MVP
- SFX manquants → Mitigation: librairie freesound, génération procédurale audio

**Risques planning:**
- Dérive de scope → Mitigation: section Out of Scope stricte, revues sprint
- Vertical slice non atteint semaine 2 → Mitigation: couper features non-core

---

## Document Information

**Document:** Blade Rush - Game Design Document  
**Version:** 1.0  
**Created:** 2026-03-03  
**Author:** Hicham  
**Status:** Complete  

### Change Log

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-03-03 | Initial GDD complete |
