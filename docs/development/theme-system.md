# Theme System

## Design tokens (`ThemeTokens`)

`Theme::tokens(ThemeType)` returns every colour and metric the UI uses:

| Group | Tokens |
| --- | --- |
| surfaces | `window`, `base`, `panelHeader`, `border`, `borderStrong` |
| text | `text`, `textMuted` |
| accent / state | `accent`, `accentHover`, `accentText`, `selection`, `success`, `warning`, `error`, `info` |
| tints (alpha) | `diffAdded`, `diffRemoved`, `searchMatch`, `sourceHighlight`, `currentLine` |
| syntax | `syntaxKeyword`, `Type`, `String`, `Number`, `Comment`, `Function`, `Preprocessor`, `Register`, `Label` |
| metrics | `spacing`, `radius`, `borderWidth`, `focusWidth` (≥ 2), `fontPointSize`, `monoFamily` |

`Theme::setCurrent(type)` switches the active tokens and bumps `Theme::generation()`.

## Stylesheets are generated

`Theme::getStylesheet(type)` = `baseStylesheet(tokens)` (widgets, inputs, buttons, item views, menus, scrollbars,
progress bars, tooltips) + `roleStylesheet(tokens)` (below). Nothing is hand-written per theme, so the contrast
the tests verify is exactly what is rendered.

## Roles

Widgets never set colours. They declare *what they are*:

```cpp
Theme::setRole(statusLabel, "error");       // sets property "role" and repolishes the widget
```

| Role | Applies to | Look |
| --- | --- | --- |
| `muted`, `ok`, `warning`, `error`, `title` | `QLabel` | text colour (warning/error also bold) |
| `noticeWarning`, `noticeError`, `noticeOk` | `QLabel` | tinted background banner |
| `toolbar`, `toolbarLabel` | `QWidget`, `QLabel` | panel header background with a bottom border; transparent label |
| `accent`, `success`, `danger`, `subtle` | `QPushButton` | filled accent / green / red / outlined |

`QPlainTextEdit[state="error"]` colours error text in editors.
Focus: `QPushButton/QComboBox/QLineEdit/QTableView/QTreeView:focus` get a `focusWidth` accent border.

## Syntax highlighters

Rules store a `SyntaxRole`, not a colour. On construction and on `applyTheme()` the colours are read from
`Theme::syntaxColor(role)`; a changed `Theme::generation()` triggers a recolour at the next highlight pass.
`MainWindow::onChangeTheme` calls `applyTheme()` on every highlighter, editor and pane.

## Adding or changing a theme

1. Add a value to `ThemeType` and `getAvailableThemes()`.
2. Add a `case` to `makeTokens()` with **all** tokens.
3. Run `test_theme`: it checks completeness and contrast and the full-window screenshot test checks polarity.
   If a colour fails, adjust the token — never add a widget-level override.

## Contrast requirements (enforced in `test_theme`)

| Pair | Minimum |
| --- | --- |
| `text` on `window` and on `base` | 4.5 : 1 |
| `textMuted` on `window` | 3 : 1 |
| `accentText` on `accent`; `accent` on `window` (focus ring) | 3 : 1 |
| `success`, `warning`, `error` on `window` | 3 : 1 |
| syntax colours on `window` (comments 2.5 : 1) | 3 : 1 |

Contrast uses the WCAG relative-luminance formula.
