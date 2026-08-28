# Qt 6 i18n 翻译不生效问题

## 环境

- Qt 6.11.1 msvc2022_64
- CMake 3.30.5
- Windows 10/11

## 现象

1. `qsTrId("app.title")` 返回字面字符串 `"app.title"`，而不是翻译后的 `"FFmpeg 媒体工具箱"`
2. `QTranslator::load()` 返回 `true`，`qDebug` 输出 `Loaded translation from resource: ":/i18n/ffmedia_zh_CN.qm"`
3. `m_translator.translate("Main", "app.title")` 返回空字符串 `""`

## 翻译文件结构

### ts 文件 (`i18n/ffmedia_zh_CN.ts`)

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>Main</name>
    <message id="app.title">
        <source>app.title</source>
        <translation>FFmpeg 媒体工具箱</translation>
    </message>
    <message id="home.title">
        <source>home.title</source>
        <translation>FFmpeg 媒体工具箱</translation>
    </message>
    <!-- ... 更多翻译条目 ... -->
</context>
</TS>
```

### qm 文件

- `cmake-build-debug/ffmedia_zh_CN.qm` 存在，大小 2634 字节
- `cmake-build-debug/ffmedia_en.qm` 存在，大小 3080 字节

## CMake 配置

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml QuickControls2 LinguistTools)

set(TS_FILES
    i18n/ffmedia_en.ts
    i18n/ffmedia_zh_CN.ts
)

qt_add_translations(ffmedia-toolbox
    TS_FILES ${TS_FILES}
    SOURCES ${QML_SOURCES}
    LUPDATE_OPTIONS -no-obsolete
)
```

## main.cpp 加载逻辑

```cpp
LanguageManager languageManager;
engine.rootContext()->setContextProperty(QStringLiteral("languageManager"), &languageManager);
```

```cpp
// LanguageManager.cpp
void LanguageManager::loadTranslation(const QString &lang)
{
    QCoreApplication::removeTranslator(&m_translator);
    if (lang.isEmpty() || lang == "en") return;

    const QString qmPath = QStringLiteral(":/i18n/ffmedia_%1.qm").arg(lang);

    if (m_translator.load(qmPath)) {
        QCoreApplication::installTranslator(&m_translator);
        // 测试：返回空字符串 ""
        qDebug() << "Test:" << m_translator.translate("Main", "app.title");
    }
}
```

## QML 用法

```qml
// Main.qml
ApplicationWindow {
    title: qsTrId("app.title")  // 显示 "app.title"，而非 "FFmpeg 媒体工具箱"
}

// LanguageManager 通过 context property 暴露
ComboBox {
    currentIndex: languageManager.language.startsWith("zh") ? 1 : 0
    onActivated: (index) => {
        languageManager.language = index === 0 ? "en" : "zh_CN"
    }
}
```

## 已排除的可能

- `.qm` 文件存在且有内容（2634 字节）
- `QTranslator::load()` 返回 `true`
- 翻译条目在 `.ts` 文件中存在

## 怀疑的原因

1. `qsTrId` 的翻译查找机制与 `qsTr` 不同，可能需要特殊的 `.ts` 文件格式？
2. `qt_add_translations` 的 `SOURCES` 参数扫描 QML 时，是否正确识别了 `qsTrId` 调用？
3. `.qm` 文件中的翻译条目是否正确编译？（`translate("Main", "app.title")` 返回空）
4. 是否需要在 QML 中使用 `QT_TRID_NOOP` 宏预声明翻译 ID？
5. Qt 6.11 是否改变了 `qsTrId` 的行为？

## 相关文件

- `CMakeLists.txt`
- `src/main.cpp`
- `src/i18n/LanguageManager.h`
- `src/i18n/LanguageManager.cpp`
- `i18n/ffmedia_zh_CN.ts`
- `i18n/ffmedia_en.ts`
- `qml/Main.qml`
- `qml/pages/SettingsPage.qml`
