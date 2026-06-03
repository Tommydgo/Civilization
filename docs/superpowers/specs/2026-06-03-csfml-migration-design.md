# Migration ncurses → CSFML

**Date:** 2026-06-03  
**Branche:** `graphic`  
**Objectif:** Remplacer le rendu ncurses par une interface graphique CSFML avec HUD de jeu de stratégie.

---

## Contexte

L'interface actuelle (`src/ui/render.c`, 662 lignes) utilise ncurses pour un affichage terminal 3 panneaux. Elle expose 8 fonctions publiques via `render.h`. Le reste du code (`game.c`, `command.c`, tests) passe exclusivement par cette interface — la migration consiste à réécrire le module UI sans toucher à la logique de jeu.

---

## Architecture des fichiers

```
src/ui/
├── render.h        (inchangé — façade publique)
├── render.c        (façade publique + machine à états + event loop)
├── ui_draw.c       (toutes les fonctions de dessin CSFML : map, toolbar, panel, log)
└── ui_dialog.c     (popup modal, menu de démarrage, écran de fin)

assets/
└── font.ttf        (police monospace, ex: DejaVu Mono — incluse dans le repo)
```

`render.h` reste identique → `game.c`, `command.c`, `tests/render_stub.c` ne changent pas.

---

## Layout CSFML

Fenêtre 1280×720 (redimensionnable). 4 zones :

```
┌──────────────────────────────────────────────────────────────────┐
│ TOOLBAR  Tour 12 | Or 45 | Sci 20 | [Recherche] [Save] [Load] [▶ Tour suivant] │
├────────────────────────────────────┬─────────────────────────────┤
│                                    │  PANEL LATÉRAL              │
│   CARTE (tuiles sfRectangleShape)  │  - Stats unité/ville        │
│   Terrain coloré + sf::Text        │  - Boutons d'action         │
│   pour unités/villes               │  - Scores ennemis           │
│                                    │                             │
├────────────────────────────────────┴─────────────────────────────┤
│  LOG  Tour 11: Ville Rome fondée | ...                           │
└──────────────────────────────────────────────────────────────────┘
```

### Primitives CSFML

| Élément | Primitive |
|---------|-----------|
| Tuile terrain | `sfRectangleShape` coloré (vert / gris / bleu) |
| Unité/ville | `sfText` centré dans la tuile (`P`/`@` jaune, `E`/`#` rouge) |
| Sélection | Bordure `sfRectangleShape` dorée |
| Toolbar / panel / log | `sfRectangleShape` fond + `sfText` |
| Boutons | `sfRectangleShape` + `sfText`, hover via `sfMouse_getPosition` |
| Popup modal | `sfRectangleShape` centré semi-transparent + champ texte + curseur clignotant |
| Police | `assets/font.ttf` chargée au démarrage |

Pas de sprites externes — tout en formes géométriques + texte.

---

## Machine à états (dans render.c)

```c
typedef enum {
    UI_IDLE,           // rien de sélectionné
    UI_UNIT_SELECTED,  // unité cliquée → boutons dans le panneau
    UI_MOVE_MODE,      // attente clic destination
    UI_ATTACK_MODE,    // attente clic cible ennemie
    UI_DIALOG_INPUT,   // popup texte ouvert
    UI_COMMAND_READY,  // commande assemblée → render_read_input retourne
} UIMode;

static UIMode   s_mode        = UI_IDLE;
static int      s_selected_id = NO_ID;
static char     s_cmd_buf[128] = {0};
```

### Transitions

| Événement | État avant | Action | État après |
|-----------|-----------|--------|-----------|
| Clic unité joueur | IDLE | Affiche stats + boutons | UNIT_SELECTED |
| Bouton "Déplacer" | UNIT_SELECTED | Surligne tuiles valides | MOVE_MODE |
| Clic tuile | MOVE_MODE | Assemble `"move <id> <x> <y>"` | COMMAND_READY |
| Bouton "Attaquer" | UNIT_SELECTED | — | ATTACK_MODE |
| Clic ennemi | ATTACK_MODE | Assemble `"attack <id> <cible>"` | COMMAND_READY |
| Bouton "Fonder ville" | UNIT_SELECTED | Ouvre popup | DIALOG_INPUT |
| Entrée dans popup | DIALOG_INPUT | Assemble `"create_city <nom>"` | COMMAND_READY |
| Bouton "Recherche" | IDLE | Ouvre popup liste techs | DIALOG_INPUT |
| Entrée dans popup | DIALOG_INPUT | Assemble `"research <tech>"` | COMMAND_READY |
| Bouton "Tour suivant" | IDLE | Assemble `"next"` | COMMAND_READY |
| Bouton "Save/Load" | IDLE | Ouvre popup nom fichier | DIALOG_INPUT |
| Bouton "Quit" / fermeture fenêtre | tout | Assemble `"quit"` | COMMAND_READY |
| Échap / clic vide | tout | Réinitialise sélection | IDLE |

`render_read_input` tourne `sfWindow_pollEvent` en boucle jusqu'à `UI_COMMAND_READY`, copie `s_cmd_buf` dans le buffer de sortie, repasse en `UI_IDLE`.

---

## Interface publique (render.h — inchangée)

```c
void render_init(void);
void render_cleanup(void);
void render_full(GameState *gs);
void render_read_input(char *buf, int len);
void render_message(GameState *gs, const char *fmt, ...);
void render_info_clear(void);
void render_info_push(const char *fmt, ...);
int  render_start_menu(GameConfig *config, int *civ_id_out);
bool render_end_screen(GameState *gs);
```

`render_start_menu` : menu de difficulté + choix de civilisation dans des popups CSFML séquentiels (liste scrollable avec sélection clavier haut/bas + Entrée, identique à l'actuel `menu_select`).  
`render_end_screen` : écran de fin avec scores, boutons [R] Rejouer / [Q] Quitter.  
`render_info_push` / `render_info_clear` : le buffer info s'affiche dans la zone LOG à la place des événements normaux — même logique que ncurses.  
Popup texte (ville, tech, save/load) : saisie libre, Entrée pour confirmer, Échap pour annuler.

---

## Makefile

```makefile
# Remplace ncurses
LDFLAGS = -lcsfml-graphics -lcsfml-window -lcsfml-system

# Tests : exclure les 3 fichiers UI (render_stub.c les remplace)
TEST_GAME_SRC = $(filter-out src/main.c src/ui/render.c src/ui/ui_draw.c src/ui/ui_dialog.c, $(SRC))
```

CSFML doit être installé (`libcsfml-dev` ou via Nix). Détection automatique du path Nix à ajouter si nécessaire (même pattern que l'actuelle détection ncurses).

---

## Tests

`tests/render_stub.c` reste identique — stubs no-op pour les 8 fonctions publiques. Aucun test ne dépend de CSFML.

---

## Ce qui ne change pas

- `src/game.c` — boucle de jeu, dispatch table, `game_run`
- `src/command/` — parsing, `command_read`
- `include/structs.h`, `include/constants.h`
- Tous les autres sous-systèmes (`world/`, `entities/`, `empire/`, `tech/`, `ai/`, `victory/`, `saves/`)
- `tests/` dans son intégralité
