#pragma once

#include <HtsViewerSdk.h>

#include <QOpenGLWidget>
#include <QPoint>

#include <memory>

/**
 * Qt OpenGL host for the delivered, OSG-free Hts Viewer SDK facade.
 *
 * Rendering is demand-driven: a frame is requested only when input arrives
 * (mouse/wheel/keyboard), on resize, or while HtsViewerSdk::hasPendingFrameWork()
 * reports that chunked display work still needs more frames. No always-on
 * repaint timer is used, so an idle viewer consumes (almost) no CPU/GPU.
 */
class HtsViewerWidget final : public QOpenGLWidget
{
Q_OBJECT

public:
    explicit HtsViewerWidget(
            std::shared_ptr<hts::viewer::HtsViewerSdk> viewerSdk,
            QWidget* parent = nullptr);
    ~HtsViewerWidget() override;

    hts::viewer::HtsViewerSdk* sdk() const;
    bool isInitialized() const { return m_Initialized; }

signals:
    void viewerInitialized();
    void viewerInitializationFailed(const QString& message);
    void viewerStateChanged();
    void displayCommandFinished();

protected:
    void initializeGL() override;
    void paintGL() override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    hts::viewer::HtsMouseButton mouseButton(Qt::MouseButton button) const;
    int viewerKey(QKeyEvent* event) const;
    void submitMouse(hts::viewer::HtsInputEventType type, QMouseEvent* event);
    void requestNextFrame();

private:
    std::shared_ptr<hts::viewer::HtsViewerSdk> m_ViewerSdk;
    bool m_Initialized = false;
    qreal m_DevicePixelRatio = 1.0;
    QPoint m_MousePressPoint;
    QPoint m_BoxSelectCurrentPoint;
    bool m_BoxSelecting = false;
};
