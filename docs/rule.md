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

Limites structurelles : certains états d'action et booléens se recouvrent encore ;
le menu conserve une action globale et les événements de mort utilisent un seul
emplacement. Éviter de développer une logique multi-combattants sur ce contrat sans
le remplacer par une liste d'événements et une machine d'états explicite.
