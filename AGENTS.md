# Qt / QML Project Rules

This project uses Qt 6.11 with CMake and modern QML modules.

## QML module rules

1. Use `qt_add_qml_module()` for all QML files.

2. The main QML module URI is: `ffmedia`

3. Load the application entry point only with:
   ```cpp
   engine.loadFromModule("ffmedia", "Main");
   ```

4. Never load QML using hard-coded resource URLs such as `qrc:/...`. Do not invent or guess Qt Resource System paths.

5. Do not add custom `RESOURCE_PREFIX` values unless explicitly requested.

6. Do not manually call `QQmlEngine::addImportPath()` unless there is a demonstrated requirement.

7. Every newly created `.qml`, `.js`, or `.mjs` file belonging to the application must also be registered in the appropriate `qt_add_qml_module(... QML_FILES ...)` declaration.

8. Whenever a QML file is added, removed, renamed, or moved, update `CMakeLists.txt` in the same change.

9. Before changing QML module configuration, inspect the existing:
   - `qt_add_qml_module()`
   - `URI`
   - `QML_FILES`
   - `qt_standard_project_setup()`
   - `engine.loadFromModule()`

   Do not infer these values from directory names.

10. Do not mix these approaches unless explicitly requested:
    - manual `.qrc` files
    - `qt_add_resources()`
    - `qt_add_qml_module()`

    Application QML should use `qt_add_qml_module()`.

## Entry point invariant

The following relationship must always remain valid:

```cmake
qt_add_qml_module(... URI ffmedia ...)
```

corresponds to:

```cpp
engine.loadFromModule("ffmedia", "Main");
```

Do not replace this with a filesystem path or `qrc:` URL.

## Build-system rule

Changes involving QML files must be treated as build-system changes. If a new QML file is created, ensure it is included by CMake before considering the task complete. After modifying `CMakeLists.txt`, assume CMake must be reconfigured before diagnosing the QML source itself.

## Error diagnosis

For errors such as:
- `No such file or directory`
- `Type ... unavailable`
- `module ... is not installed`
- `QQmlApplicationEngine failed to load component`

check the CMake QML module configuration and imports before changing QML application logic. Never "fix" these errors by guessing a different `qrc:/...` path.

## Directory structure

```
src/  → C++
qml/  → QML
i18n/ → Translation files (.ts)
```

## i18n

- Use `qsTrId("key")` for all translatable strings
- Translation files: `i18n/ffmedia_en.ts`, `i18n/ffmedia_zh_CN.ts`
- Language switching: `Qt.uiLanguage = "zh_CN"`
