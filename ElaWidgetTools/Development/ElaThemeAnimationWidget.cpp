#include "ElaThemeAnimationWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
ElaThemeAnimationWidget::ElaThemeAnimationWidget(QWidget* parent)
    : QWidget{parent}
{
    _pEndRadius = 0.01;
}

ElaThemeAnimationWidget::~ElaThemeAnimationWidget()
{
}

void ElaThemeAnimationWidget::startAnimation(int msec)
{
    QPropertyAnimation* themeChangeAnimation = new QPropertyAnimation(this, "pRadius");
    themeChangeAnimation->setDuration(msec);
    themeChangeAnimation->setEasingCurve(QEasingCurve::InOutSine);
    connect(themeChangeAnimation, &QPropertyAnimation::finished, this, [=]() {
        Q_EMIT animationFinished();
        this->deleteLater();
    });
    connect(themeChangeAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
        update();
    });
    themeChangeAnimation->setStartValue(0);
    themeChangeAnimation->setEndValue(_pEndRadius);
    themeChangeAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    show();
}

void ElaThemeAnimationWidget::paintEvent(QPaintEvent* event)
{
    if (_pOldWindowBackground.isNull())
    {
        return;
    }

    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setPen(Qt::NoPen);

    // Dynamic radial transition revealing the newly painted window underneath
    // by clipping out the expanding circle from the old window snapshot
    qreal devicePixelRatioF = _pOldWindowBackground.devicePixelRatioF();
    QPainterPath fullPath;
    fullPath.addRect(rect());

    QPainterPath circlePath;
    circlePath.addEllipse(QPointF(_pCenter.x(), _pCenter.y()), _pRadius, _pRadius);

    QPainterPath clipPath = fullPath.subtracted(circlePath);
    painter.setClipPath(clipPath);
    painter.drawImage(rect(), _pOldWindowBackground);
    painter.restore();
}
