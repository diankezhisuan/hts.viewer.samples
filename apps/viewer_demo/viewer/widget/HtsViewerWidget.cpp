#include "viewer/widget/HtsViewerWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QWindow>

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kKeyLeft = 0xFF51;
constexpr int kKeyUp = 0xFF52;
constexpr int kKeyRight = 0xFF53;
constexpr int kKeyDown = 0xFF54;
constexpr int kKeyHome = 0xFF50;
constexpr int kKeyEnd = 0xFF57;
constexpr int kKeyPageUp = 0xFF55;
constexpr int kKeyPageDown = 0xFF56;
}

HtsViewerWidget::HtsViewerWidget(
        std::shared_ptr<hts::viewer::HtsViewerSdk> viewerSdk,
        QWidget* parent)
        : QOpenGLWidget(parent)
        , m_ViewerSdk(std::move(viewerSdk))
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(320, 240);

    // No always-on repaint timer: the viewer is demand-driven. Frames are
    // requested by requestNextFrame() on input/resize and kept alive while
    // HtsViewerSdk::hasPendingFrameWork() is true (see paintGL()). An idle
    // widget therefore does not burn CPU/GPU redrawing at 60 FPS.
}

HtsViewerWidget::~HtsViewerWidget()
{
    if (context()) makeCurrent();
    m_ViewerSdk.reset();
    if (context()) doneCurrent();
}

hts::viewer::HtsViewerSdk* HtsViewerWidget::sdk() const
{
    return m_ViewerSdk.get();
}

void HtsViewerWidget::initializeGL()
{
    if (m_Initialized || !m_ViewerSdk) return;

    m_DevicePixelRatio = window() ? window()->devicePixelRatio() : devicePixelRatioF();
    const int viewportWidth = std::max(1, int(width() * m_DevicePixelRatio));
    const int viewportHeight = std::max(1, int(height() * m_DevicePixelRatio));

    if (!m_ViewerSdk->initializeEmbedded(0, 0, viewportWidth, viewportHeight)) {
        emit viewerInitializationFailed(
                tr("Hts Viewer SDK 初始化失败。请检查 Hts3dViewer 授权和运行时 DLL。"));
        return;
    }

    m_Initialized = true;
    emit viewerInitialized();
}

void HtsViewerWidget::paintGL()
{
    if (!m_Initialized || !m_ViewerSdk) return;

    const bool hadPendingWork = m_ViewerSdk->hasPendingFrameWork();
    m_ViewerSdk->frame();
    const bool hasPendingWork = m_ViewerSdk->hasPendingFrameWork();
    if (hadPendingWork && !hasPendingWork) emit displayCommandFinished();
    if (hasPendingWork) update();
}

void HtsViewerWidget::paintEvent(QPaintEvent* event)
{
    QOpenGLWidget::paintEvent(event);
    if (!m_BoxSelecting) return;

    QPainter painter(this);
    painter.setPen(QPen(QColor(50, 145, 255), 1, Qt::DashLine));
    painter.setBrush(QColor(50, 145, 255, 32));
    painter.drawRect(QRect(m_MousePressPoint, m_BoxSelectCurrentPoint).normalized());
}

void HtsViewerWidget::resizeEvent(QResizeEvent* event)
{
    m_DevicePixelRatio = window() ? window()->devicePixelRatio() : devicePixelRatioF();
    if (m_Initialized && m_ViewerSdk) {
        const int viewportWidth = std::max(1, int(event->size().width() * m_DevicePixelRatio));
        const int viewportHeight = std::max(1, int(event->size().height() * m_DevicePixelRatio));
        m_ViewerSdk->resizeViewport(0, 0, viewportWidth, viewportHeight);

        hts::viewer::HtsInputEvent resize;
        resize.type = hts::viewer::HtsInputEventType::Resize;
        resize.width = viewportWidth;
        resize.height = viewportHeight;
        m_ViewerSdk->submitInput(resize);
    }
    QOpenGLWidget::resizeEvent(event);
}

void HtsViewerWidget::mousePressEvent(QMouseEvent* event)
{
    m_MousePressPoint = event->pos();
    m_BoxSelectCurrentPoint = event->pos();
    m_BoxSelecting = event->button() == Qt::LeftButton
                     && (event->modifiers() & Qt::ControlModifier);
    if (m_ViewerSdk) m_ViewerSdk->clearHover();
    if (!m_BoxSelecting) submitMouse(hts::viewer::HtsInputEventType::MouseButtonPress, event);
    setFocus(Qt::MouseFocusReason);
    QOpenGLWidget::mousePressEvent(event);
    requestNextFrame();
}

void HtsViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    const QPoint delta = event->pos() - m_MousePressPoint;
    if (m_BoxSelecting && event->button() == Qt::LeftButton) {
        if (m_ViewerSdk && delta.manhattanLength() > 8) {
            const QRect rect = QRect(m_MousePressPoint, event->pos()).normalized();
            m_ViewerSdk->boxSelect(rect.left() * m_DevicePixelRatio,
                                   rect.top() * m_DevicePixelRatio,
                                   rect.right() * m_DevicePixelRatio,
                                   rect.bottom() * m_DevicePixelRatio);
            emit viewerStateChanged();
        } else if (m_ViewerSdk) {
            // Ctrl + click is additive/toggle selection. Ctrl + drag becomes
            // box selection only after crossing the drag threshold.
            m_ViewerSdk->selectAt(event->x() * m_DevicePixelRatio,
                                  event->y() * m_DevicePixelRatio,
                                  true);
            emit viewerStateChanged();
        }
        m_BoxSelecting = false;
    } else {
        submitMouse(hts::viewer::HtsInputEventType::MouseButtonRelease, event);
        if (m_ViewerSdk && event->button() == Qt::LeftButton
            && delta.manhattanLength() <= 4) {
            const bool additive = (event->modifiers() & Qt::ControlModifier) != 0;
            m_ViewerSdk->selectAt(event->x() * m_DevicePixelRatio,
                                  event->y() * m_DevicePixelRatio,
                                  additive);
            emit viewerStateChanged();
        }
    }
    QOpenGLWidget::mouseReleaseEvent(event);
    requestNextFrame();
}

void HtsViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_BoxSelecting && (event->buttons() & Qt::LeftButton)) {
        m_BoxSelectCurrentPoint = event->pos();
    } else {
        submitMouse(hts::viewer::HtsInputEventType::MouseMove, event);
        if (m_ViewerSdk && event->buttons() == Qt::NoButton) {
            m_ViewerSdk->updateHover(event->x() * m_DevicePixelRatio,
                                     event->y() * m_DevicePixelRatio);
            emit viewerStateChanged();
        }
    }
    QOpenGLWidget::mouseMoveEvent(event);
    requestNextFrame();
}

void HtsViewerWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    submitMouse(hts::viewer::HtsInputEventType::MouseDoubleClick, event);
    QOpenGLWidget::mouseDoubleClickEvent(event);
    requestNextFrame();
}

void HtsViewerWidget::wheelEvent(QWheelEvent* event)
{
    if (m_ViewerSdk) {
        const QPoint delta = event->angleDelta();
        hts::viewer::HtsInputEvent input;
        input.type = hts::viewer::HtsInputEventType::MouseWheel;
        input.x = event->pos().x() * m_DevicePixelRatio;
        input.y = event->pos().y() * m_DevicePixelRatio;
        if (std::abs(delta.y()) >= std::abs(delta.x())) {
            input.scrollMotion = delta.y() >= 0
                                 ? hts::viewer::HtsScrollMotion::Up
                                 : hts::viewer::HtsScrollMotion::Down;
            input.wheelDelta = delta.y();
        } else {
            input.scrollMotion = delta.x() >= 0
                                 ? hts::viewer::HtsScrollMotion::Right
                                 : hts::viewer::HtsScrollMotion::Left;
            input.wheelDelta = delta.x();
        }
        m_ViewerSdk->submitInput(input);
    }
    event->accept();
    requestNextFrame();
}

void HtsViewerWidget::leaveEvent(QEvent* event)
{
    if (m_ViewerSdk) m_ViewerSdk->clearHover();
    emit viewerStateChanged();
    QOpenGLWidget::leaveEvent(event);
    requestNextFrame();
}

void HtsViewerWidget::keyPressEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat() && m_ViewerSdk) {
        bool handled = true;
        const bool control = event->modifiers() & Qt::ControlModifier;
        const bool shift = event->modifiers() & Qt::ShiftModifier;
        switch (event->key()) {
            case Qt::Key_1: m_ViewerSdk->setSelectionMode(hts::viewer::HtsSelectionMode::Object); break;
            case Qt::Key_2: m_ViewerSdk->setSelectionMode(hts::viewer::HtsSelectionMode::Body); break;
            case Qt::Key_3: m_ViewerSdk->setSelectionMode(hts::viewer::HtsSelectionMode::Face); break;
            case Qt::Key_4: m_ViewerSdk->setSelectionMode(hts::viewer::HtsSelectionMode::Edge); break;
            case Qt::Key_5: m_ViewerSdk->setSelectionMode(hts::viewer::HtsSelectionMode::Vertex); break;
            case Qt::Key_A:
                if (control) m_ViewerSdk->selectAllVisible(); else handled = false;
                break;
            case Qt::Key_F:
            case Qt::Key_Home:
                shift || control ? m_ViewerSdk->fitSelection() : m_ViewerSdk->fitView();
                break;
            case Qt::Key_H: shift ? m_ViewerSdk->showAll() : m_ViewerSdk->hideSelected(); break;
            case Qt::Key_I: control && shift ? m_ViewerSdk->clearIsolate() : m_ViewerSdk->isolateSelected(); break;
            case Qt::Key_Escape: m_ViewerSdk->clearSelection(); m_ViewerSdk->clearHover(); break;
            default: handled = false; break;
        }

        if (!handled) {
            hts::viewer::HtsInputEvent input;
            input.type = hts::viewer::HtsInputEventType::KeyPress;
            input.key = viewerKey(event);
            input.unmodifiedKey = input.key;
            if (input.key != 0) m_ViewerSdk->submitInput(input);
        }
        emit viewerStateChanged();
    }
    QOpenGLWidget::keyPressEvent(event);
    requestNextFrame();
}

void HtsViewerWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat() && m_ViewerSdk) {
        hts::viewer::HtsInputEvent input;
        input.type = hts::viewer::HtsInputEventType::KeyRelease;
        input.key = viewerKey(event);
        input.unmodifiedKey = input.key;
        if (input.key != 0) m_ViewerSdk->submitInput(input);
    }
    QOpenGLWidget::keyReleaseEvent(event);
    requestNextFrame();
}

hts::viewer::HtsMouseButton HtsViewerWidget::mouseButton(Qt::MouseButton button) const
{
    switch (button) {
        case Qt::LeftButton: return hts::viewer::HtsMouseButton::Left;
        case Qt::MiddleButton: return hts::viewer::HtsMouseButton::Middle;
        case Qt::RightButton: return hts::viewer::HtsMouseButton::Right;
        default: return hts::viewer::HtsMouseButton::None;
    }
}

int HtsViewerWidget::viewerKey(QKeyEvent* event) const
{
    if (!event) return 0;
    switch (event->key()) {
        case Qt::Key_Left: return kKeyLeft;
        case Qt::Key_Up: return kKeyUp;
        case Qt::Key_Right: return kKeyRight;
        case Qt::Key_Down: return kKeyDown;
        case Qt::Key_Home: return kKeyHome;
        case Qt::Key_End: return kKeyEnd;
        case Qt::Key_PageUp: return kKeyPageUp;
        case Qt::Key_PageDown: return kKeyPageDown;
        case Qt::Key_Escape: return 0x1B;
        case Qt::Key_Return:
        case Qt::Key_Enter: return 0x0D;
        case Qt::Key_Backspace: return 0x08;
        case Qt::Key_Tab: return 0x09;
        default: break;
    }
    const QString text = event->text();
    return text.isEmpty() ? 0 : text.at(0).unicode();
}

void HtsViewerWidget::submitMouse(
        hts::viewer::HtsInputEventType type,
        QMouseEvent* event)
{
    if (!m_ViewerSdk || !event) return;
    hts::viewer::HtsInputEvent input;
    input.type = type;
    input.x = event->x() * m_DevicePixelRatio;
    input.y = event->y() * m_DevicePixelRatio;
    input.button = mouseButton(event->button());
    m_ViewerSdk->submitInput(input);
}

void HtsViewerWidget::requestNextFrame()
{
    update();
}
