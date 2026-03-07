# Implementation - Portabilite des types C

## Objectif

Le projet vise une compilation portable (Linux, macOS, Windows). Pour y parvenir, les structures et les API publiques utilisent des types de taille explicite.

## Types entiers fixes

Nous utilisons les types de `<stdint.h>` :

- `int32_t`
- `uint16_t`
- `uint8_t`
- `uint32_t`
- `uint64_t`

Pourquoi :

- Le type `int` n'a pas une taille garantie par le standard C.
- La taille peut varier selon le compilateur, l'architecture et la plateforme.
- Les types fixes assurent un layout memoire stable pour les structures partagees entre modules.

## Booleens

Nous utilisons `bool` depuis `<stdbool.h>` pour les etats logiques (`true`/`false`).

Pourquoi :

- Le code est plus lisible qu'avec des `int` utilises comme booleens.
- Le comportement est explicite et uniforme sur toutes les plateformes supportees.

## Regle pratique

- API publique et structures de donnees: preferer les types explicites (`uint32_t`, `int32_t`, etc.).
- Eviter `int`/`long` pour les donnees dont la taille est importante pour la logique metier.