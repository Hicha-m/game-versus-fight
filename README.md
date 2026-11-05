# Blade Rush — game-versus-fight

Duel local ou contre l'IA, inspiré de la progression de Nidhogg.
Gagner un échange donne le droit de progresser vers la sortie adverse.

## Compilation

Dépendances : compilateur C11, CMake **3.25+** (presets version 6), SDL3 3.2+,
et SDL3_image 3.2+ pour les PNG. Sans SDL3_image, seul le rendu BMP de secours
est disponible. Installer les bibliothèques avec leurs fichiers de développement
et renseigner `CMAKE_PREFIX_PATH` pour une installation locale.

```sh
cmake --preset dev
cmake --build --preset build -j 8
cmake --build --preset run
```

## Commandes par défaut

| Action | J1 | J2 |
| --- | --- | --- |
| Déplacement | A / D | Gauche / Droite |
| Garde haute / basse | W / S | Haut / Bas |
| Saut | Espace | Entrée |
| Estoc / frappe | J | Ctrl droit |
| Lancer | K | Maj droite |
| Roulade | Bas + direction | Bas + direction |
| Accroupissement / ramper | Maintenir bas | Maintenir bas |
| Balayage | Bas + attaque | Bas + attaque |
| Coup plongeant | Attaque en l’air | Attaque en l’air |
| Lancer contextuel | Haut + attaque | Haut + attaque |
| Saut mural | Saut contre un mur | Saut contre un mur |
| Ramasser une épée proche | Bas | Bas |
| Pause | Échap | Échap |

Sur clavier AZERTY, les codes physiques correspondent à Q/D et Z/S pour J1.
La configuration `game_config.json` est relative au répertoire d'exécution.
Dans Options → Keybinds : gauche/droite choisit le joueur, haut/bas l'action,
Sélection lance la capture de la prochaine touche, Échap annule. Une affectation
déjà utilisée par ce joueur est échangée avec l'ancienne. Échap et F11 sont réservés.
Les modifications sont sauvegardées et appliquées immédiatement. Options permet
aussi de basculer le plein écran ; F11 reste disponible. F3 affiche les corps de collision.

Musique et effets sont synthétisés localement. Les trois réglages de volume sont
appliqués au flux audio ; un ordinateur sans périphérique audio peut jouer en silence.
Au choix de la graine : chiffres pour éditer, Retour arrière pour effacer,
flèches pour tirer une nouvelle graine. La graine choisie est conservée au lancement
et affichée dans le HUD pour reproduire une partie.

## Règles

Trois gardes : haute, médiane, basse. Une garde compatible face à une attaque
bloque et repousse sans désarmer. Un changement de garde qui croise profondément
la lame adverse désarme. Les coups létaux simultanés peuvent tuer les deux
combattants. À mains nues, deux coups de poing rapprochés mettent au sol. Un balayage ou un
coup plongeant y suffit en un coup. Approcher, s’abaisser et attaquer permet ensuite
l’exécution. Les épées désarmées ou tombées
après un lancer peuvent être ramassées. La roulade abaisse le corps sans donner d’invincibilité : une garde basse reste létale.

La progression exige un combattant vivant ayant gagné l'avantage. Le coureur
conserve son équipement au passage de salle ; le défenseur réapparaît après
un délai. Il faut atteindre la sortie de la salle terminale pour gagner.
Une nouvelle partie remet les statistiques à zéro.

## Tests

```sh
cmake -S . -B build-tests -DGAME_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests -j 8
SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software ctest --test-dir build-tests --output-on-failure
```

Six cibles : comparaison Nidhogg, acceptation de l'audit, régressions de gameplay/configuration,
génération, modules et architecture. L'acceptation vérifie les menus, l'audio,
le rendu des animations et 132 couples graine/difficulté en traversée physique.
Les assertions d'acceptation restent actives en compilation Release.

## Installation Linux reproductible

```sh
bash scripts/bootstrap_linux.sh
cmake --preset dev -DCMAKE_PREFIX_PATH="$PWD/.deps/install"
```

Le script installe deux révisions SDL épinglées dans `.deps/`, sans droits
administrateur. Pour une fenêtre interactive, installer d'abord les headers X11
ou Wayland et audio de la distribution avant cette compilation SDL. Sans eux,
les tests dummy restent utilisables mais pas une fenêtre desktop.

## Validation et choix de conception

Les salles procédurales sont traversées par simulation dans les deux sens avant
la partie. Celles qui échouent reçoivent un remplacement déterministe vérifié.
L'IA réactive est le mode par défaut ; les variantes minimax et alpha-beta disponibles
par API simulent le combat réel sur un horizon borné, avec budget de calcul et délai
de décision. Elles ne prédisent pas une partie entière.

Les animations utilisent les sprites du dépôt. Ce projet reprend des mécaniques de
Nidhogg ; ses timings, règles d'arbitrage et présentation restent ceux de Blade Rush.
Voir [la clôture de l'audit](docs/validation.md) pour les preuves et les limites
matérielles des vérifications, et [les dépendances](docs/dependance.md).

## Comparaison documentée avec Nidhogg

Voir [la comparaison sourcée, les différences et les captures](docs/nidhogg-comparison.md).
Quatre thèmes jouables — Castle, Wilds, Mines, Clouds — sont sélectionnables avant
la partie. Les modes procédural et corridor restent disponibles. Les épées lancées
se déplacent horizontalement ; une séquence de course finale précède les statistiques.
