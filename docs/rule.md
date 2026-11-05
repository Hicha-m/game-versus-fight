# Architecture effective

`main` orchestre les entrées, la simulation à pas fixe et le rendu.
`engine` possède la fenêtre SDL, les événements et le temps.
`game` possède la partie, l'arène, le combat, les commandes et l'IA.
`render` observe cet état et traduit ses positions/actions en images.
`core` définit les types et constantes ; `utils` fournit les aides générales.

Le gameplay ne doit pas inclure SDL. Les en-têtes publics ne doivent pas exposer
SDL. Les détails SDL_image et les textures restent privés au rendu. Les codes
physiques sauvegardés dans la configuration sont des entiers validés et convertis
par le moteur. Les headers internes restent dans `src/` et dans leur module.

Vérification : `bash scripts/check_architecture.sh`. Cette vérification statique
complète les tests ; elle ne prouve pas à elle seule la qualité de l'architecture.

Le combat distingue une intention de garde explicite d'une commande sans changement.
Les transitions de salle conservent le coureur et programment le retour du défenseur.
Le retour au menu et le démarrage d'une nouvelle partie sont distincts du changement
de salle. Les règles jouables et commandes sont décrites dans le README.

Le combat produit une liste bornée d'événements. `game` en consomme chaque mort
et désarmement pour les statistiques et l'avantage ; les sons sont ensuite transmis
au moteur par des intentions sans dépendance SDL. Les morts doubles ne dépendent
pas de l'ordre des indices. La machine d'actions réconcilie les états prioritaires
à la fin du tick. Les drapeaux historiques restent présents dans l'API et sont maintenus
cohérents par cette transition centrale.

La génération structurelle appartient à arena. La certification de traversée utilise
le solveur de combat et est orchestrée par game : arena ne dépend donc pas de combat.
Le contrôle BFS historique ne sert pas de preuve physique ; les parties valident
les spawns et les trajectoires avant de commencer.
