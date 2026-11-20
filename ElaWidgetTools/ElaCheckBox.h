#ifndef ELAWORKSPACE_ELAWIDGETTOOLS_ELACHECKBOX_H_
#define ELAWORKSPACE_ELAWIDGETTOOLS_ELACHECKBOX_H_

#include <QCheckBox>

#include "ElaWidgetToolsExport.h"
#include "ElaPropertyMacro.h"
#include "ElaWidgetToolsDef.h"

class ELA_EXPORT ElaCheckBox : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY_CREATE(int, BorderRadius)
public:
    explicit ElaCheckBox(QWidget* parent = nullptr);
    explicit ElaCheckBox(const QString& text, QWidget* parent = nullptr);
    ~ElaCheckBox() override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ElaThemeType::ThemeMode _themeMode{ElaThemeType::Light};
    void _onThemeChanged(ElaThemeType::ThemeMode themeMode);
};

#endif // ELAWORKSPACE_ELAWIDGETTOOLS_ELACHECKBOX_H_
