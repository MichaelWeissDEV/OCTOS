# Themes & Layout

## Themes

**Tools ▸ Theme** offers six themes: *Dark* (default), *Light*, *Solarized Dark*, *Solarized Light*, *Dracula*,
*Nord*. The choice is stored.

All colours come from one place (*design tokens*), so every widget — including editors, tables, tabs, status
badges, diff tints and syntax highlighting — follows the theme. There are no widgets that are only readable
in the dark theme. The test suite checks, for **every** theme:

* body text ≥ 4.5 : 1 contrast against the window and input backgrounds (WCAG AA);
* muted text, accent buttons, focus rings, success/warning/error colours ≥ 3 : 1;
* every syntax colour ≥ 3 : 1 (comments ≥ 2.5 : 1, they are meant to be quiet);
* the whole window rendered in each theme has no large areas of the wrong polarity (screenshot check).

Keyboard focus is always visible: focused buttons, combos, inputs, tables and trees get a 2 px accent border.

HiDPI: sizes are derived from the application font and Qt's scaling; no pixel sizes are hard-coded in the
widgets.

## Layout

* The source editor, the compiler panes and the docks are separated by **splitters**; drag them.
* The **Project** and **Assembly Analysis** docks can be docked left/right/bottom, floated, tabbed or closed.
  Toggle them in the **View** menu.
* On exit OCTOS stores the window geometry, the dock arrangement and the splitter positions. The next start
  restores them. A restored layout never hides the explorer of an open project.
* **View ▸ Reset Layout** restores the factory arrangement.

## Where it is stored

In OCTOS' settings store under `Layout/…` ([Settings](../reference/settings.md)). You can override the store
with the environment variable `OCTOS_SETTINGS_DIR`.
