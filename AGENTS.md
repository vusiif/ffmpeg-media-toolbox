# Qt Widgets Project Rules

This project uses Qt 6.11 Widgets with CMake. The UI framework is
ElaWidgetTools (vendored at `ElaWidgetTools/`, MIT licensed, see
`ElaWidgetTools/LICENSE`).

## UI framework rules

1. Main window derives from `ElaWindow`. Pages are plain `QWidget` pages
   registered via `addPageNode()` / `addFooterNode()`.

2. Prefer Ela controls (`ElaText`, `ElaPushButton`, `ElaComboBox`,
   `ElaLineEdit`, `ElaSpinBox`, `ElaCheckBox`, `ElaProgressBar`) over stock
   widgets for visible UI chrome.

3. `ElaWidgetTools/` is vendored third-party code. Patches to it are allowed
   (that is the point of vendoring), but keep them minimal and mechanical
   (e.g. Qt 6.11 API compatibility). Never reformat unrelated code in it.

4. On startup: `eApp->init()` must be called before any Ela widget is created.

## Build-system rules

- `ElaWidgetTools` is added via `add_subdirectory(ElaWidgetTools)` and linked
  as `Ela::WidgetTools`.
- `Qt6::WidgetsPrivate` is required by ElaWidgetTools and must stay in
  `find_package`.
- The Ela shared library is copied next to the executable by a POST_BUILD
  command. Keep it when touching deploy logic.
- MSVC needs `/utf-8` (already set in CMakeLists) because sources contain
  non-ASCII text.

## Directory structure

```
src/         → C++ (backend + UI)
  app/       → MainWindow + pages
  ffmpeg/    → FFmpeg process/command layer
  media/     → media probing
  jobs/      → job model/queue
  i18n/      → LanguageManager
ElaWidgetTools/ → vendored UI framework (MIT)
i18n/       → Translation files (.ts)
Others/     → external UI references (not built)
```

## i18n

- Use `trId("key")` (from `src/i18n/TrId.h`) for all translatable strings.
  Do NOT use `qtTrId`/`qsTrId`: with this project's .ts/.qm setup they do
  not resolve. `trId()` looks up `QCoreApplication::translate("Main", id)`.
- Translation files: `i18n/ffmedia_en.ts`, `i18n/ffmedia_zh_CN.ts`.
  Convention: every `<message>` lives in context `Main` and its `<source>`
  equals its id. Keep this shape when editing .ts files by hand.
- `LanguageManager` installs the translator at construction (before any UI
  is built) and reloads on language change; `MainWindow::retranslateUi()`
  then refreshes all page texts.

## UI rewrite in progress

The project was migrated from QML to Widgets. Backend classes in
`src/ffmpeg`, `src/media`, `src/jobs` are shared with the old QML UI and
still carry `Q_PROPERTY`/`Q_INVOKABLE` declarations; these are harmless and
may be pruned lazily.
