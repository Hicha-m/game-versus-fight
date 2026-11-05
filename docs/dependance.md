# Dépendances et responsabilités

| Module | Dépendances |
| --- | --- |
| main | engine, game, render, core, utils |
| engine | SDL3, entrée abstraite, core |
| game | entrée abstraite, arena, combat, ai, menu, config |
| arena | core, utils |
| combat | arena, core, utils |
| ai | combat, arena, core |
| render | SDL3, SDL3_image optionnel, engine et état public game |
| utils / core | aucune dépendance de plateforme |

Les en-têtes d'entrée se trouvent actuellement dans `include/engine/input.h`, mais
leurs types sont indépendants de SDL. La compatibilité SDL_image et les textures
sont privées dans `src/render/`. Le script d'architecture surveille les inclusions
interdites ; les tests de simulation vérifient les règles de jeu.
