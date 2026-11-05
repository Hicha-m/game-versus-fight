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
| Roulade | Bas + saut | Bas + saut |
| Ramasser une épée proche | Bas | Bas |
| Pause | Échap | Échap |

Sur clavier AZERTY, les codes physiques correspondent à Q/D et Z/S pour J1.
La configuration `game_config.json` est relative au répertoire d'exécution.
Les raccourcis sauvegardés sont chargés au démarrage. Le menu de raccourcis
reste une consultation ; modifier le fichier pour changer les affectations.

## Règles

Trois gardes : haute, médiane, basse. Une garde compatible face à une attaque
pare et désarme l'attaquant. Les coups létaux simultanés peuvent tuer les deux
combattants. À mains nues, une première frappe étourdit ; une autre frappe
pendant cet étourdissement achève l'adversaire. Les épées désarmées ou tombées
après un lancer peuvent être ramassées. Une roulade donne une brève esquive.

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

Quatre cibles : régressions de gameplay/configuration, génération, suite des
modules et règles d'architecture. Le test de génération historique est un
smoke test ; il ne prouve pas que chaque niveau est franchissable.

## Limites actuelles

Le jeu reste un prototype, pas une reproduction complète de Nidhogg. Les sprites
sont encore essentiellement statiques. Le système audio n'est pas implémenté ;
les valeurs de volume du menu sont seulement persistées. L'IA réactive est le
mode par défaut ; les variantes de recherche restent expérimentales et leur
simulation abstraite ne reproduit pas toute la physique. Les niveaux procéduraux
ne disposent pas encore d'une preuve de franchissabilité par simulation.

Les tests sans affichage ne remplacent pas une partie interactive à deux.
Voir [la validation des corrections](docs/validation.md) et
[les dépendances](docs/dependance.md).
