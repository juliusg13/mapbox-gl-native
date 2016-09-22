#include "qquickmapboxglrenderer.hpp"

#include <mbgl/util/constants.hpp>

#include <QMapbox>
#include <QQuickMapboxGL>
#include <QQuickMapboxGLStyleProperty>
#include <QQuickMapboxGLMapParameter>

#include <QDebug>
#include <QQuickItem>
#include <QRegularExpression>
#include <QString>
#include <QtGlobal>
#include <QQmlListProperty>

#include <cmath>

QQuickMapboxGL::QQuickMapboxGL(QQuickItem *parent_)
    : QQuickFramebufferObject(parent_)
{
}

QQuickMapboxGL::~QQuickMapboxGL()
{
}

QQuickFramebufferObject::Renderer *QQuickMapboxGL::createRenderer() const
{
    return new QQuickMapboxGLRenderer;
}

void QQuickMapboxGL::setMinimumZoomLevel(qreal zoom)
{
    zoom = qMax(mbgl::util::MIN_ZOOM, zoom);
    zoom = qMin(m_maximumZoomLevel, zoom);

    if (m_minimumZoomLevel == zoom) {
        return;
    }

    m_minimumZoomLevel = zoom;
    setZoomLevel(m_zoomLevel); // Constrain.

    emit minimumZoomLevelChanged();
}

qreal QQuickMapboxGL::minimumZoomLevel() const
{
    return m_minimumZoomLevel;
}

void QQuickMapboxGL::setMaximumZoomLevel(qreal zoom)
{
    zoom = qMin(mbgl::util::MAX_ZOOM, zoom);
    zoom = qMax(m_minimumZoomLevel, zoom);

    if (m_maximumZoomLevel == zoom) {
        return;
    }

    m_maximumZoomLevel = zoom;
    setZoomLevel(m_zoomLevel); // Constrain.

    emit maximumZoomLevelChanged();
}

qreal QQuickMapboxGL::maximumZoomLevel() const
{
    return m_maximumZoomLevel;
}

void QQuickMapboxGL::setZoomLevel(qreal zoom)
{
    zoom = qMin(m_maximumZoomLevel, zoom);
    zoom = qMax(m_minimumZoomLevel, zoom);

    if (m_zoomLevel == zoom) {
        return;
    }

    m_zoomLevel = zoom;

    m_syncState |= ZoomNeedsSync;
    update();

    emit zoomLevelChanged(m_zoomLevel);
}

qreal QQuickMapboxGL::zoomLevel() const
{
    return m_zoomLevel;
}

void QQuickMapboxGL::setCenter(const QGeoCoordinate &coordinate)
{
    if (m_center == coordinate) {
        return;
    }

    m_center = coordinate;

    m_syncState |= CenterNeedsSync;
    update();

    emit centerChanged(m_center);
}

QGeoCoordinate QQuickMapboxGL::center() const
{
    return m_center;
}

void QQuickMapboxGL::setVisibleRegion(const QGeoShape &shape)
{
    m_visibleRegion = shape;
}

QGeoShape QQuickMapboxGL::visibleRegion() const
{
    return m_visibleRegion;
}

void QQuickMapboxGL::setColor(const QColor &color)
{
    if (m_color == color) {
        return;
    }

    m_color = color;

    QVariantMap paintProperty;
    paintProperty["type"] = QQuickMapboxGLLayoutStyleProperty::PaintType;
    paintProperty["layer"] = "background";
    paintProperty["property"] = "background-color";
    paintProperty["value"] = color;
    onStylePropertyUpdated(paintProperty);

    emit colorChanged(m_color);
}

QColor QQuickMapboxGL::color() const
{
    return m_color;
}

void QQuickMapboxGL::pan(int dx, int dy)
{
    m_pan += QPointF(dx, -dy);

    m_syncState |= PanNeedsSync;
    update();
}

void QQuickMapboxGL::setStyle(QQuickMapboxGLStyle *style)
{
    if (style == m_style) {
        return;
    }

    disconnect(style, SIGNAL(urlChanged(QString)), this, SLOT(onStyleChanged()));
    disconnect(style, SIGNAL(propertyUpdated(QVariantMap)), this, SLOT(onStylePropertyUpdated(QVariantMap)));
    delete m_style;
    m_style = style;
    if (style) {
        style->setParentItem(this);
        connect(style, SIGNAL(urlChanged(QString)), this, SLOT(onStyleChanged()));
        connect(style, SIGNAL(propertyUpdated(QVariantMap)), this, SLOT(onStylePropertyUpdated(QVariantMap)));
    }

    m_syncState |= StyleNeedsSync;
    m_styleLoaded = false;
    update();

    emit styleChanged();
}

QQuickMapboxGLStyle *QQuickMapboxGL::style() const
{
    return m_style;
}

void QQuickMapboxGL::setBearing(qreal angle)
{
    angle = std::fmod(angle, 360.);

    if (m_bearing == angle) {
        return;
    }

    m_bearing = angle;

    m_syncState |= BearingNeedsSync;
    update();

    emit bearingChanged(m_bearing);
}

qreal QQuickMapboxGL::bearing() const
{
    return m_bearing;
}

void QQuickMapboxGL::setPitch(qreal angle)
{
    angle = qMin(qMax(0., angle), mbgl::util::PITCH_MAX * mbgl::util::RAD2DEG);

    if (m_pitch == angle) {
        return;
    }

    m_pitch = angle;

    m_syncState |= PitchNeedsSync;
    update();

    emit pitchChanged(m_pitch);
}

qreal QQuickMapboxGL::pitch() const
{
    return m_pitch;
}

void QQuickMapboxGL::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    QQuickFramebufferObject::itemChange(change, value);

    switch (change) {
    case QQuickItem::ItemChildAddedChange:
        if (QQuickMapboxGLStyleProperty *property = qobject_cast<QQuickMapboxGLStyleProperty *>(value.item)) {
            connect(property, SIGNAL(updated(QVariantMap)), this, SLOT(onStylePropertyUpdated(QVariantMap)));
            connect(this, SIGNAL(styleChanged()), property, SLOT(checkUpdated()));
        }
        break;
    case QQuickItem::ItemChildRemovedChange:
        if (QQuickMapboxGLStyleProperty *property = qobject_cast<QQuickMapboxGLStyleProperty *>(value.item)) {
            disconnect(property, SIGNAL(updated(QVariantMap)), this, SLOT(onStylePropertyUpdated(QVariantMap)));
            disconnect(this, SIGNAL(styleChanged()), property, SLOT(checkUpdated()));
        }
    default:
        break;
    }
}

void QQuickMapboxGL::onMapChanged(QMapbox::MapChange change)
{
    if (change == QMapbox::MapChangeDidFinishLoadingStyle) {
        m_styleLoaded = true;
        update();
    }
}

void QQuickMapboxGL::onStylePropertyUpdated(const QVariantMap &params)
{
    switch (params.value("type").toInt()) {
    case QQuickMapboxGLStyleProperty::LayoutType:
        m_layoutChanges << params;
        break;
    case QQuickMapboxGLStyleProperty::PaintType:
        m_paintChanges << params;
        break;
    }

    update();
}

void QQuickMapboxGL::onStyleChanged()
{
    m_syncState |= StyleNeedsSync;
    update();

    emit styleChanged();
}

void QQuickMapboxGL::appendParameter(QQmlListProperty<QQuickMapboxGLMapParameter> *prop, QQuickMapboxGLMapParameter *param)
{
    QQuickMapboxGL *map = static_cast<QQuickMapboxGL *>(prop->object);
    map->m_parameters.append(param);
}

int QQuickMapboxGL::countParameters(QQmlListProperty<QQuickMapboxGLMapParameter> *prop)
{
    QQuickMapboxGL *map = static_cast<QQuickMapboxGL *>(prop->object);
    return map->m_parameters.count();
}

QQuickMapboxGLMapParameter *QQuickMapboxGL::parameterAt(QQmlListProperty<QQuickMapboxGLMapParameter> *prop, int index)
{
    QQuickMapboxGL *map = static_cast<QQuickMapboxGL *>(prop->object);
    return map->m_parameters[index];
}

void QQuickMapboxGL::clearParameter(QQmlListProperty<QQuickMapboxGLMapParameter> *prop)
{
    QQuickMapboxGL *map = static_cast<QQuickMapboxGL *>(prop->object);
    for (auto param : map->m_parameters) {
    }
    map->m_parameters.clear();
}

QQmlListProperty<QQuickMapboxGLMapParameter> QQuickMapboxGL::parameters()
{
    return QQmlListProperty<QQuickMapboxGLMapParameter>(this,
            nullptr,
            appendParameter,
            countParameters,
            parameterAt,
            clearParameter);
}
