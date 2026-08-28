# 语言切换问题描述

## 现象

在应用右上角切换语言（English/中文），UI 文本不刷新。

## 架构

- `TranslationManager` (C++) 管理翻译，从 JSON 文件加载
- `translations/en.json` 和 `translations/zh.json` 包含翻译键值对
- QML 通过 `i18n.tr(key, version)` 获取翻译文本
- `languageChanged` 信号在语言切换时发出
- `languageVersion` 属性在语言切换时递增

## 相关代码

### TranslationManager.h
```cpp
class TranslationManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY languageChanged)
    Q_PROPERTY(int languageVersion READ languageVersion NOTIFY languageChanged)
public:
    Q_INVOKABLE void setLanguage(const QString &lang);
    Q_INVOKABLE QString tr(const QString &key, int version = 0) const;
signals:
    void languageChanged();
};
```

### TranslationManager.cpp
```cpp
void TranslationManager::setLanguage(const QString &lang)
{
    m_currentLang = lang;
    m_translations = m_allTranslations.value(lang);
    m_languageVersion++;
    emit languageChanged();
}

QString TranslationManager::tr(const QString &key, int version) const
{
    Q_UNUSED(version)
    if (m_translations.contains(key)) {
        return m_translations.value(key);
    }
    return key;
}
```

### main.cpp
```cpp
TranslationManager i18n;
engine.rootContext()->setContextProperty(QStringLiteral("i18n"), &i18n);
```

### QML (Main.qml)
```qml
function t(key) {
    return i18n.tr(key, i18n.languageVersion)
}

Label {
    text: root.t("home.title")
}
```

### ComboBox (语言切换)
```qml
ComboBox {
    id: langCombo
    model: ["English", "中文"]
    currentIndex: i18n.currentLanguage === "zh" ? 1 : 0
    onActivated: (index) => {
        i18n.setLanguage(index === 0 ? "en" : "zh")
    }
}
```

## 问题分析

理论上 `i18n.languageVersion` 变化时，QML 应该重新绑定 `root.t()` 调用。但实际没有生效。

可能原因：
1. QML 对函数调用的绑定追踪不完整
2. `languageVersion` 的变化没有正确传播到 QML
3. 需要用其他方式强制刷新

## 已尝试的方案

1. ✅ 添加 `languageVersion` 属性并作为 `tr()` 参数
2. ✅ 在每个页面添加 `t()` 辅助函数
3. ❌ 仍然不刷新

## 环境

- Qt 6.11.1 msvc2022_64
- Windows
- CLion 2026.2.1
