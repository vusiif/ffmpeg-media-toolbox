#include "ElaCheckBox.h"

#include "ElaApplication.h"
#include "ElaCheckBoxStyle.h"
#include "ElaTheme.h"

ElaCheckBox::ElaCheckBox(QWidget* parent)
    : QCheckBox(parent)
{
    _pBorderRadius = 3;
    setMouseTracking(true);
    setObjectName("ElaCheckBox");
    setStyle(new ElaCheckBoxStyle(style()));
    QFont font = this->font();
    font.setPixelSize(eApp->getFontPixelSize() + 2);
    setFont(font);

    _themeMode = eTheme->getThemeMode();
    _onThemeChanged(_themeMode);
    connect(eTheme, &ElaTheme::themeModeChanged, this, &ElaCheckBox::_onThemeChanged);
}

ElaCheckBox::ElaCheckBox(const QString& text, QWidget* parent)
    : ElaCheckBox(parent)
{
    setText(text);
}

ElaCheckBox::~ElaCheckBox()
{
    delete this->style();
}

void ElaCheckBox::_onThemeChanged(ElaThemeType::ThemeMode themeMode)
{
    _themeMode = themeMode;
    QPalette pal = palette();
    const QColor textColor = ElaThemeColor(_themeMode, BasicText);
    pal.setColor(QPalette::WindowText, textColor);
    pal.setColor(QPalette::Text, textColor);
    pal.setColor(QPalette::ButtonText, textColor);
    setPalette(pal);
    update();
}

void ElaCheckBox::paintEvent(QPaintEvent* event)
{
    if (palette().color(QPalette::WindowText) != ElaThemeColor(_themeMode, BasicText))
    {
        _onThemeChanged(_themeMode);
    }
    QCheckBox::paintEvent(event);
}
