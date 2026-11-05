# Suivi de l'audit — clôture des corrections

Les 15 constats ont une correction implémentée. « Vérifié » signifie que les
contrôles indiqués passent, pas que le jeu reproduit exactement le ressenti de
Nidhogg. La règle choisie pour deux coups létaux simultanés est la double mort.

| Audit | Correction | Vérification |
| --- | --- | --- |
| 1. Sortie d'un mort | Sortie réservée au porteur vivant de l'avantage | regression_tests |
| 2. Priorité J1 | Évaluation depuis un même instantané et fusion par combattant | regression_tests, audit_tests : double mort, statistiques des deux joueurs |
| 3. Parade de dos | Orientation et hauteur de lame vérifiées | regression_tests |
| 4. Garde IA ignorée | Commande explicite partagée avec le combat | regression_tests |
| 5. Collisions | Résolution X/Y par sous-pas, contacts sol/plafond, plateformes traversables, dangers | regression_tests, traversées simulées |
| 6. Configuration | Lecture JSON bornée par section/joueur, validation avant application, sauvegarde temporaire | audit_tests : format compact, touches distinctes, valeurs invalides |
| 7. Poursuite entre salles | Équipement/vitesse du coureur conservés, entrée sûre, délai du défenseur conservé, réapparition devant le coureur | regression_tests |
| 8. Cycle des armes | Désarmement et lancer déposent une arme ; bas pour récupérer ; frappe puis exécution désarmée | regression_tests, règles dans README |
| 9. Actions manquantes | Roulade, saut sur front d'appui, attaque aérienne | regression_tests, audit_tests |
| 10. Recul annulé | Contrôle bloqué pendant le stun, recul intégré avec friction | regression_tests |
| 11. Contact et dessin | Segment de lame commun, anticipation/contact/récupération, F3 pour les corps de collision | audit_tests : rendu des états et capture |
| 12. IA incohérente | IA réactive et recherches distinctes ; simulations avec combat_step réel ; budget de recherche borné | audit_tests : minimax/alpha-beta comparés à profondeur 2, métriques |
| 13. Victoire et stats | Sortie terminale nécessaire, liste d'événements, comptage des deux morts, remise à zéro | audit_tests |
| 14. Options inactives | Remappage J1/J2 avec capture/annulation, plein écran appliqué immédiatement, musique/effets synthétisés et volumes appliqués | audit_tests : remappage/sauvegarde, flux audio muet puis non nul |
| 15. Procédural | Contrôle de spawn complet et traversée réelle des deux bords dans les deux sens ; remplacement déterministe des salles rejetées | audit_tests : 132 couples graine/difficulté, génération répétée à l'identique |

## Architecture, rédaction et exploitation

- Headers SDL/image privés au rendu ; contrôle d'architecture réussi.
- État de fermeture, action de menu et générateur aléatoire portés par leurs contextes.
- Actions prioritaires réconciliées en fin de simulation : mort, stun, attaque,
  action temporisée, déplacement. La roulade et le lancer restent temporisés en l'air.
- Événements de combat bornés et multiples : morts, désarmements et attaques.
- Animations de course, saut, chute, roulade, attaque armée/désarmée, désarmement,
  mort, lancer et gardes chargées depuis les sprites existants. Temps de simulation
  pour les animations ; lissage caméra indépendant du nombre d'images.
- Spawns initiaux dans le champ de caméra, couleurs distinctes et HUD de progression/graine.
- VSync avec temporisation de secours ; audio à file bornée. Les métriques IA
  indiquent les nœuds visités et une estimation de pile, pas une mesure du tas.
- README et règles réécrits ; CMake minimum 3.25 cohérent avec les presets.
- Formatage C reproductible via `.clang-format`.
- Workflow GitHub Actions : dépendances SDL épinglées, compilation, sanitizers,
  tests et capture conservée comme artefact. Le résultat distant doit être consulté
  sur GitHub ; son ajout ne vaut pas affirmation d'une exécution distante réussie.
- La présentation PDF historique n'a pas été modifiée. Ce document et le README
  remplacent ses affirmations techniques devenues inexactes.

## Vérifications locales

Cinq cibles : `audit_tests`, `regression_tests`, `generation_tests`, `game_tests`,
`architecture_rules`. Elles passent en Debug, Release et sous ASan/UBSan sur Linux,
en mode dummy pour la vidéo et l'audio.
`audit_tests` ne saute pas silencieusement une absence de renderer/audio : elle échoue.
Une capture de scène est produite dans `build/audit-work/audit-frame.bmp` et a été
inspectée. Le menu de remappage et tous les états de combat passent par le renderer.

```sh
cmake --preset dev
cmake --build --preset build --parallel 4
ctest --preset test
```

Vérification mémoire et comportement indéfini :

```sh
cmake -S . -B build-sanitize -DGAME_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitize --parallel 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-sanitize --output-on-failure
```

La session locale utilise ptrace : LeakSanitizer y échoue avant de produire un
résultat exploitable. Sa recherche de fuites est donc désactivée localement ;
AddressSanitizer et UndefinedBehaviorSanitizer restent actifs. Le workflow distant
ne désactive pas LeakSanitizer.

## Portée de la validation

Le validateur de traversée est volontairement conservateur : son échec entraîne
un remplacement, même si un humain aurait trouvé un autre chemin. Il garantit un
parcours simulé avec les constantes et le solveur courants pour les salles acceptées,
pas toutes les trajectoires possibles ni toutes les futures modifications de physique.
La génération brute est disponible dans arena ; les parties passent par la validation
supérieure dans game avant de la proposer au joueur.

La vidéo et l'audio dummy permettent une validation automatique, pas une écoute sur
haut-parleurs ni une partie humaine sur un écran. Le ressenti, l'équilibrage et les
combinaisons clavier dépendantes du matériel restent à éprouver en jouant. Cela ne
constitue pas une fonctionnalité laissée non implémentée ni une équivalence certifiée
avec le jeu Nidhogg original.
