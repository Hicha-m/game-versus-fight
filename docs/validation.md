# Validation des corrections de l'audit

Les corrections comprennent : gardes IA, parades orientées, morts simultanées,
recul conservé, attaque aérienne, roulade, récupération d'épées, exécution désarmée,
collisions séparées X/Y, transitions réservées aux vivants, conservation du coureur,
sortie finale, statistiques de nouvelle partie et configuration distincte des joueurs.
Les dépendances SDL publiques ont été déplacées vers le rendu privé.

Vérifications sur Linux :

- compilation du jeu et des tests ;
- quatre cibles CTest réussies ;
- mêmes cibles sous AddressSanitizer et UndefinedBehaviorSanitizer ;
- démarrage du jeu en mode dummy pendant cinq secondes, textures PNG chargées.

Commande sanitizers :

```sh
cmake -S . -B build-sanitize -DGAME_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitize -j 8
ASAN_OPTIONS=detect_leaks=0 SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software \
  ctest --test-dir build-sanitize --output-on-failure
```

La session d'exécution utilise ptrace : LeakSanitizer y échoue avant de produire
un résultat exploitable. Seule sa recherche de fuites a été désactivée. Aucune
conclusion de validation exhaustive des fuites n'est tirée de ce passage.

Les tests de régression couvrent la garde IA, l'attaque de dos, la double mort,
le recul, l'attaque aérienne, le mur latéral, désarmement/récupération, l'interdiction
de sortie d'un mort, la conservation de l'équipement, la remise à zéro des stats,
l'aller-retour des touches distinctes et le rejet d'un indice négatif.

À valider en fenêtre : équilibre des timings, lisibilité des armes, caméra, clavier
à deux joueurs, transitions et ressenti. Les animations complètes, l'audio, le menu
de remappage et une validation physique du procédural restent à réaliser. Ces limites
sont explicitement conservées dans le README ; les tests automatisés ne prouvent
pas une équivalence complète avec Nidhogg.
