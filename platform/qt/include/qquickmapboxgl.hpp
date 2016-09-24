#ifndef QQUICKMAPBOXGL_H
#define QQUICKMAPBOXGL_H

#include <QColor>
#include <QGeoCoordinate>
#include <QGeoServiceProvider>
#include <QGeoShape>
#include <QPointF>
#include <QQuickFramebufferObject>
#include <QQmlListProperty>

#include <QMapbox>
#include <QQuickMapboxGLStyle>
#include <QQuickMapboxGLMapParameter>

class QQuickItem;
class QQuickMapboxGLRenderer;

class Q_DECL_EXPORT QQuickMapboxGL : public QQuickFramebufferObject
{
    Q_OBJECT

    // Map QML Type interface.
    Q_ENUMS(QGeoServiceProvider::Error)
    Q_PROPERTY(qreal minimumZoomLevel READ minimumZoomLevel WRITE setMinimumZoomLevel NOTIFY minimumZoomLevelChanged)
    Q_PROPERTY(qreal maximumZoomLevel READ maximumZoomLevel WRITE setMaximumZoomLevel NOTIFY maximumZoomLevelChanged)
    Q_PROPERTY(qreal zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)
    Q_PROPERTY(QGeoCoordinate center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(QGeoShape visibleRegion READ visibleRegion WRITE setVisibleRegion)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

    // MapboxGL QML Type interface.
    Q_PROPERTY(QQuickMapboxGLStyle *style READ style WRITE setStyle NOTIFY styleChanged)
    Q_PROPERTY(qreal bearing READ bearing WRITE setBearing NOTIFY bearingChanged)
    Q_PROPERTY(qreal pitch READ pitch WRITE setPitch NOTIFY pitchChanged)

    // Proposed Qt interface - based on the example documentation below:
    // http://doc.qt.io/qt-5/qtqml-referenceexamples-properties-example.html
    Q_PROPERTY(QQmlListProperty<QQuickMapboxGLMapParameter> parameters READ parameters)

public:
    QQuickMapboxGL(QQuickItem *parent = 0);
    virtual ~QQuickMapboxGL();

    // QQuickFramebufferObject implementation.
    virtual Renderer *createRenderer() const Q_DECL_FINAL;

    // Map QML Type interface implementation.
    void setMinimumZoomLevel(qreal minimumZoomLevel);
    qreal minimumZoomLevel() const;

    void setMaximumZoomLevel(qreal maximumZoomLevel);
    qreal maximumZoomLevel() const;

    void setZoomLevel(qreal zoomLevel);
    qreal zoomLevel() const;

    QGeoCoordinate center() const;

    void setVisibleRegion(const QGeoShape &shape);
    QGeoShape visibleRegion() const;

    void setColor(const QColor &color);
    QColor color() const;

    Q_INVOKABLE void pan(int dx, int dy);

    // MapboxGL QML Type interface.
    void setStyle(QQuickMapboxGLStyle *);
    QQuickMapboxGLStyle* style() const;

    void setBearing(qreal bearing);
    qreal bearing() const;

    void setPitch(qreal pitch);
    qreal pitch() const;

    enum SyncState {
        NothingNeedsSync = 0,
        ZoomNeedsSync    = 1 << 0,
        CenterNeedsSync  = 1 << 1,
        StyleNeedsSync   = 1 << 2,
        PanNeedsSync     = 1 << 3,
        BearingNeedsSync = 1 << 4,
        PitchNeedsSync   = 1 << 5,
    };

    // Proposed Qt interface implementation.
    QQmlListProperty<QQuickMapboxGLMapParameter> parameters();

protected:
    // QQuickItem implementation.
    virtual void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value);

signals:
    // Map QML Type signals.
    void minimumZoomLevelChanged();
    void maximumZoomLevelChanged();
    void zoomLevelChanged(qreal zoomLevel);
    void centerChanged(const QGeoCoordinate &coordinate);
    void colorChanged(const QColor &color);

    // Mapbox-specific signals.
    void styleChanged();
    void bearingChanged(qreal angle);
    void pitchChanged(qreal angle);

public slots:
    void setCenter(const QGeoCoordinate &center);

private slots:
    void onMapChanged(QMapbox::MapChange);
    void onStyleChanged();
    void onStylePropertyUpdated(const QVariantMap &params);

private:
    static void appendParameter(QQmlListProperty<QQuickMapboxGLMapParameter> *prop, QQuickMapboxGLMapParameter *mapObject);
    static int countParameters(QQmlListProperty<QQuickMapboxGLMapParameter> *prop);
    static QQuickMapboxGLMapParameter *parameterAt(QQmlListProperty<QQuickMapboxGLMapParameter> *prop, int index);
    static void clearParameter(QQmlListProperty<QQuickMapboxGLMapParameter> *prop);

    qreal m_minimumZoomLevel = 0;
    qreal m_maximumZoomLevel = 20;
    qreal m_zoomLevel = 20;

    QPointF m_pan;

    QGeoCoordinate m_center;
    QGeoShape m_visibleRegion;
    QColor m_color;
    QList<QVariantMap> m_layoutChanges;
    QList<QVariantMap> m_paintChanges;
    QList<QQuickMapboxGLMapParameter*> m_parameters;

    QQuickMapboxGLStyle *m_style = 0;
    qreal m_bearing = 0;
    qreal m_pitch = 0;

    int m_syncState = NothingNeedsSync;
    bool m_styleLoaded = false;

    friend class QQuickMapboxGLRenderer;
};

#endif // QQUICKMAPBOXGL_H
