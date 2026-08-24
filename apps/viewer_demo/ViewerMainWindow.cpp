#include "ViewerMainWindow.h"

#include "LicenseInfoDialog.h"
#include "import/HtsDisplayDataImporter.h"
#include "import/HtsImportOptions.h"
#include "import/HtsImportedModel.h"
#include "viewer/widget/HtsViewerWidget.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QByteArray>
#include <QColorDialog>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStatusBar>
#include <QToolBar>
#include <QThread>
#include <QVBoxLayout>

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <map>
#include <sstream>
#include <utility>
#include <vector>

namespace
{
QString targetText(const hts::viewer::HtsSelectionTarget& target)
{
    using namespace hts::viewer;
    switch (target.type) {
        case HtsSelectionTargetType::Object:
            return QString("Object · %1").arg(QString::fromStdString(target.objectId));
        case HtsSelectionTargetType::Body:
            return QString("Body · %1 / %2")
                    .arg(QString::fromStdString(target.objectId),
                         QString::fromStdString(target.bodyId));
        case HtsSelectionTargetType::Face:
            return QString("Face · %1 #%2")
                    .arg(QString::fromStdString(target.objectId)).arg(target.faceTag);
        case HtsSelectionTargetType::Edge:
            return QString("Edge · %1 #%2")
                    .arg(QString::fromStdString(target.objectId)).arg(target.edgeTag);
        case HtsSelectionTargetType::Vertex:
            return QString("Vertex · %1 #%2")
                    .arg(QString::fromStdString(target.objectId)).arg(target.vertexTag);
        case HtsSelectionTargetType::None:
        default:
            return QObject::tr("无选择");
    }
}

void addMaterial(QComboBox* combo, const QString& text, hts::viewer::HtsMaterialPreset preset)
{
    combo->addItem(text, static_cast<int>(preset));
}

hts::viewer::HtsRenderMaterial presetMaterial(hts::viewer::HtsMaterialPreset preset)
{
    using namespace hts::viewer;
    HtsRenderMaterial material;
    material.baseColor = {0.70f, 0.77f, 0.88f, 1.0f};
    material.metallic = 0.0f;
    material.roughness = 0.55f;
    material.specular = 0.35f;
    switch (preset) {
        case HtsMaterialPreset::NeutralSolid:
            material.baseColor = {0.72f, 0.74f, 0.77f, 1.0f}; material.roughness = 0.50f; break;
        case HtsMaterialPreset::PECMetal:
            material.baseColor = {0.92f, 0.94f, 0.96f, 1.0f}; material.metallic = 1.0f;
            material.roughness = 0.20f; material.specular = 0.92f; break;
        case HtsMaterialPreset::Aluminum:
            material.baseColor = {0.82f, 0.83f, 0.82f, 1.0f}; material.metallic = 1.0f;
            material.roughness = 0.34f; material.specular = 0.76f; break;
        case HtsMaterialPreset::Copper:
            material.baseColor = {0.95f, 0.42f, 0.18f, 1.0f}; material.metallic = 1.0f;
            material.roughness = 0.28f; material.specular = 0.88f; break;
        case HtsMaterialPreset::Dielectric:
            material.baseColor = {0.70f, 0.86f, 0.88f, 1.0f}; material.roughness = 0.42f;
            material.specular = 0.55f; break;
        case HtsMaterialPreset::FR4:
            material.baseColor = {0.06f, 0.30f, 0.16f, 1.0f}; material.roughness = 0.84f;
            material.specular = 0.16f; break;
        case HtsMaterialPreset::RubberAbsorber:
            material.baseColor = {0.045f, 0.048f, 0.052f, 1.0f}; material.roughness = 0.95f;
            material.specular = 0.06f; break;
        default: break;
    }
    return material;
}

hts::viewer::HtsMaterialAppearance materialAppearance(
        const hts::viewer::HtsRenderMaterial& material)
{
    hts::viewer::HtsMaterialAppearance appearance;
    appearance.color = material.baseColor;
    appearance.transparency = 1.0f - std::clamp(material.opacity, 0.0f, 1.0f);
    appearance.metallic = material.metallic;
    appearance.roughness = material.roughness;
    appearance.specular = material.specular;
    return appearance;
}
}

ViewerMainWindow::ViewerMainWindow(
        std::shared_ptr<hts::viewer::HtsViewerSdk> viewerSdk,
        QWidget* parent)
        : QMainWindow(parent)
        , m_ViewerSdk(std::move(viewerSdk))
{
    setWindowTitle(tr("Hts Viewer SDK · Product Sandbox"));
    m_ViewerWidget = new HtsViewerWidget(m_ViewerSdk, this);
    setCentralWidget(m_ViewerWidget);
    createActions();
    createMenus();
    createToolBar();
    createDockPanel();

    m_ViewerWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_ViewerWidget, &QWidget::customContextMenuRequested,
            this, [this](const QPoint& position) {
                QMenu menu(this);
                menu.addAction(tr("适合全部"), this, [this]() {
                    m_ViewerSdk->fitView(); m_ViewerWidget->update();
                });
                menu.addAction(tr("适合选择"), this, [this]() {
                    m_ViewerSdk->fitSelection(); m_ViewerWidget->update();
                });
                menu.addAction(tr("清除选择"), this, [this]() {
                    m_ViewerSdk->clearSelection(); updateUiState(); m_ViewerWidget->update();
                });
                menu.addSeparator();
                menu.addAction(tr("隐藏选择"), this, [this]() {
                    runDisplayAction(tr("隐藏选择"), [this]() { return m_ViewerSdk->hideSelected(); });
                });
                menu.addAction(tr("显示选择"), this, [this]() {
                    runDisplayAction(tr("显示选择"), [this]() { return m_ViewerSdk->showSelected(); });
                });
                menu.addAction(tr("隔离选择"), this, [this]() {
                    runDisplayAction(tr("隔离选择"), [this]() { return m_ViewerSdk->isolateSelected(); });
                });
                menu.addAction(tr("清除隔离"), this, [this]() {
                    runDisplayAction(tr("清除隔离"), [this]() { return m_ViewerSdk->clearIsolate(); });
                });
                menu.addAction(tr("显示全部"), this, [this]() {
                    runDisplayAction(tr("显示全部"), [this]() { return m_ViewerSdk->showAll(); });
                });
                menu.addSeparator();
                menu.addAction(tr("恢复原始外观"), this, [this]() {
                    runDisplayAction(tr("恢复原始外观"), [this]() {
                        const bool handled = m_ViewerSdk->resetSelectedStyle();
                        if (handled) m_ViewerSdk->clearSelection();
                        return handled;
                    });
                });
                menu.exec(m_ViewerWidget->mapToGlobal(position));
            });

    m_StatusLabel = new QLabel(tr("正在初始化 Viewer SDK…"), this);
    statusBar()->addWidget(m_StatusLabel, 1);
    statusBar()->addPermanentWidget(new QLabel(tr("Hts3dViewerSDK"), this));

    connect(m_ViewerWidget, &HtsViewerWidget::viewerInitialized,
            this, [this]() {
                m_ViewerSdk->setSelectionMode(hts::viewer::HtsSelectionMode::Object);
                m_ViewerSdk->setAxisVisible(m_AxisVisibleAction->isChecked());
                m_ViewerSdk->setHudAxisVisible(m_HudVisibleAction->isChecked());
                m_ViewerSdk->setScaleBarVisible(m_HudVisibleAction->isChecked());
                m_ViewerSdk->setFloorVisible(m_FloorVisibleAction->isChecked());
                m_ViewerSdk->setGroundGridVisible(m_GroundGridVisibleAction->isChecked());
                setGridPlane(hts::viewer::HtsGridPlane::XOY);
                updateUiState(tr("Viewer SDK 已就绪，请导入模型"));
            });
    connect(m_ViewerWidget, &HtsViewerWidget::viewerInitializationFailed,
            this, [this](const QString& message) {
                m_StatusLabel->setText(message);
                QMessageBox::critical(this, tr("Viewer 初始化失败"), message);
            });
    connect(m_ViewerWidget, &HtsViewerWidget::viewerStateChanged,
            this, [this]() { updateUiState(); });
    connect(m_ViewerWidget, &HtsViewerWidget::displayCommandFinished,
            this, [this]() {
                if (m_FitAfterDisplayCommand) {
                    m_FitAfterDisplayCommand = false;
                    m_ViewerSdk->fitView();
                    m_ViewerWidget->update();
                }
                updateUiState(tr("后台显示任务已完成"));
            });
}

ViewerMainWindow::~ViewerMainWindow() = default;

void ViewerMainWindow::createActions()
{
    m_ImportAction = new QAction(tr("导入模型"), this);
    m_ImportAction->setShortcut(QKeySequence::Open);
    m_ClearSceneAction = new QAction(tr("清空场景"), this);
    m_LicenseAction = new QAction(tr("授权管理…"), this);
    connect(m_ImportAction, &QAction::triggered, this, &ViewerMainWindow::importModel);
    connect(m_ClearSceneAction, &QAction::triggered, this, [this]() {
        m_ViewerSdk->clearScene();
        updateUiState(tr("场景已清空")); m_ViewerWidget->update();
    });
    connect(m_LicenseAction, &QAction::triggered, this, &ViewerMainWindow::showLicenseDialog);

    m_SelectionGroup = new QActionGroup(this);
    m_SelectionGroup->setExclusive(true);
    auto makeSelection = [this](const QString& name, int shortcut,
                                hts::viewer::HtsSelectionMode mode) {
        QAction* action = new QAction(name, this);
        action->setCheckable(true);
        action->setShortcut(QKeySequence(QString::number(shortcut)));
        m_SelectionGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, mode]() { setSelectionMode(mode); });
        return action;
    };
    m_ObjectModeAction = makeSelection(tr("对象选择"), 1, hts::viewer::HtsSelectionMode::Object);
    m_FaceModeAction = makeSelection(tr("面选择"), 2, hts::viewer::HtsSelectionMode::Face);
    m_EdgeModeAction = makeSelection(tr("边选择"), 3, hts::viewer::HtsSelectionMode::Edge);
    m_ObjectModeAction->setChecked(true);

    m_GridPlaneGroup = new QActionGroup(this);
    m_GridPlaneGroup->setExclusive(true);
    auto makeGridPlane = [this](const QString& name, hts::viewer::HtsGridPlane plane) {
        QAction* action = new QAction(name, this);
        action->setCheckable(true);
        m_GridPlaneGroup->addAction(action);
        connect(action, &QAction::triggered, this,
                [this, plane]() { setGridPlane(plane); });
        return action;
    };
    m_XyPlaneAction = makeGridPlane(tr("XY 平面"), hts::viewer::HtsGridPlane::XOY);
    m_YzPlaneAction = makeGridPlane(tr("YZ 平面"), hts::viewer::HtsGridPlane::YOZ);
    m_XzPlaneAction = makeGridPlane(tr("XZ 平面"), hts::viewer::HtsGridPlane::XOZ);
    m_XyPlaneAction->setChecked(true);

    m_StyleGroup = new QActionGroup(this);
    m_StyleGroup->setExclusive(true);
    auto makeStyle = [this](const QString& name, hts::viewer::HtsDisplayStyle style) {
        QAction* action = new QAction(name, this);
        action->setCheckable(true);
        m_StyleGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, style]() { setDisplayStyle(style); });
        return action;
    };
    m_EngineeringStyleAction = makeStyle(tr("工程默认"), hts::viewer::HtsDisplayStyle::EngineeringDefault);
    m_ShadedStyleAction = makeStyle(tr("着色"), hts::viewer::HtsDisplayStyle::Shaded);
    m_ShadedEdgesStyleAction = makeStyle(tr("着色 + CAD 边"), hts::viewer::HtsDisplayStyle::ShadedWithCadEdges);
    makeStyle(tr("三角线框"), hts::viewer::HtsDisplayStyle::TriangleWireframe);
    m_EngineeringStyleAction->setChecked(true);

    m_CadEdgesAction = new QAction(tr("显示 CAD 边"), this);
    m_CadEdgesAction->setCheckable(true);
    m_CadEdgesAction->setChecked(true);
    connect(m_CadEdgesAction, &QAction::toggled, this, [this](bool checked) {
        m_ViewerSdk->showCadEdges(checked); updateUiState(); m_ViewerWidget->update();
    });
    m_WireframeAction = new QAction(tr("显示三角线框"), this);
    m_WireframeAction->setCheckable(true);
    connect(m_WireframeAction, &QAction::toggled, this, [this](bool checked) {
        m_ViewerSdk->showTriangleWireframe(checked); updateUiState(); m_ViewerWidget->update();
    });

    m_AxisVisibleAction = new QAction(tr("显示坐标轴"), this);
    m_AxisVisibleAction->setCheckable(true);
    m_AxisVisibleAction->setChecked(true);
    connect(m_AxisVisibleAction, &QAction::toggled, this, [this](bool checked) {
        m_ViewerSdk->setAxisVisible(checked);
        updateUiState(checked ? tr("坐标轴已显示") : tr("坐标轴已隐藏"));
        m_ViewerWidget->update();
    });

    m_HudVisibleAction = new QAction(tr("显示 HUD"), this);
    m_HudVisibleAction->setCheckable(true);
    m_HudVisibleAction->setChecked(true);
    connect(m_HudVisibleAction, &QAction::toggled, this, [this](bool checked) {
        m_ViewerSdk->setHudAxisVisible(checked);
        m_ViewerSdk->setScaleBarVisible(checked);
        updateUiState(checked ? tr("HUD 已显示") : tr("HUD 已隐藏"));
        m_ViewerWidget->update();
    });

    m_FloorVisibleAction = new QAction(tr("显示地板"), this);
    m_FloorVisibleAction->setCheckable(true);
    m_FloorVisibleAction->setChecked(true);
    connect(m_FloorVisibleAction, &QAction::toggled, this, [this](bool checked) {
        m_ViewerSdk->setFloorVisible(checked);
        updateUiState(checked ? tr("地板已显示") : tr("地板已隐藏"));
        m_ViewerWidget->update();
    });

    m_GroundGridVisibleAction = new QAction(tr("显示地面网格"), this);
    m_GroundGridVisibleAction->setCheckable(true);
    m_GroundGridVisibleAction->setChecked(true);
    connect(m_GroundGridVisibleAction, &QAction::toggled, this, [this](bool checked) {
        m_ViewerSdk->setGroundGridVisible(checked);
        updateUiState(checked ? tr("地面网格已显示") : tr("地面网格已隐藏"));
        m_ViewerWidget->update();
    });
}

void ViewerMainWindow::createMenus()
{
    QMenu* file = menuBar()->addMenu(tr("产品"));
    file->addAction(m_ImportAction);
    file->addAction(m_ClearSceneAction);
    file->addSeparator();
    file->addAction(tr("退出"), qApp, &QApplication::quit, QKeySequence::Quit);

    QMenu* view = menuBar()->addMenu(tr("视图"));
    view->addAction(tr("适合全部"), this, [this]() { m_ViewerSdk->fitView(); m_ViewerWidget->update(); }, QKeySequence("F"));
    view->addAction(tr("适合选择"), this, [this]() { m_ViewerSdk->fitSelection(); m_ViewerWidget->update(); }, QKeySequence("Shift+F"));
    view->addSeparator();
    view->addAction(tr("轴测图"), this, [this]() {
        applyStandardView(tr("轴测图"), [this]() { m_ViewerSdk->setIsoView(); });
    });
    view->addAction(tr("俯视图"), this, [this]() {
        applyStandardView(tr("俯视图"), [this]() { m_ViewerSdk->setTopView(); });
    });
    view->addAction(tr("仰视图"), this, [this]() {
        applyStandardView(tr("仰视图"), [this]() { m_ViewerSdk->setBottomView(); });
    });
    view->addAction(tr("前视图"), this, [this]() {
        applyStandardView(tr("前视图"), [this]() { m_ViewerSdk->setFrontView(); });
    });
    view->addAction(tr("后视图"), this, [this]() {
        applyStandardView(tr("后视图"), [this]() { m_ViewerSdk->setRearView(); });
    });
    view->addAction(tr("左视图"), this, [this]() {
        applyStandardView(tr("左视图"), [this]() { m_ViewerSdk->setLeftView(); });
    });
    view->addAction(tr("右视图"), this, [this]() {
        applyStandardView(tr("右视图"), [this]() { m_ViewerSdk->setRightView(); });
    });
    view->addSeparator();
    QMenu* workPlane = view->addMenu(tr("工作平面"));
    workPlane->addActions(m_GridPlaneGroup->actions());

    QMenu* selection = menuBar()->addMenu(tr("选择"));
    selection->addActions(m_SelectionGroup->actions());
    selection->addSeparator();
    selection->addAction(tr("清除选择"), this, [this]() {
        m_ViewerSdk->clearSelection(); updateUiState(tr("选择已清除")); m_ViewerWidget->update();
    }, QKeySequence(Qt::Key_Escape));

    QMenu* display = menuBar()->addMenu(tr("显示"));
    display->addActions(m_StyleGroup->actions());
    display->addSeparator();
    display->addAction(m_CadEdgesAction);
    display->addAction(m_WireframeAction);
    display->addAction(m_AxisVisibleAction);
    display->addAction(m_HudVisibleAction);
    display->addAction(m_FloorVisibleAction);
    display->addAction(m_GroundGridVisibleAction);
    display->addSeparator();
    display->addAction(tr("隐藏选择"), this, [this]() {
        runDisplayAction(tr("隐藏选择"), [this]() { return m_ViewerSdk->hideSelected(); });
    });
    display->addAction(tr("显示选择"), this, [this]() {
        runDisplayAction(tr("显示选择"), [this]() { return m_ViewerSdk->showSelected(); });
    });
    display->addAction(tr("隔离选择"), this, [this]() {
        runDisplayAction(tr("隔离选择"), [this]() { return m_ViewerSdk->isolateSelected(); });
    });
    display->addAction(tr("清除隔离"), this, [this]() {
        runDisplayAction(tr("清除隔离"), [this]() { return m_ViewerSdk->clearIsolate(); });
    });
    display->addAction(tr("显示全部"), this, [this]() {
        runDisplayAction(tr("显示全部"), [this]() { return m_ViewerSdk->showAll(); });
    });
    display->addAction(tr("重置显示覆盖"), this, [this]() {
        runDisplayAction(tr("重置显示"), [this]() { return m_ViewerSdk->resetDisplayOverrides(); });
    });
    display->addAction(tr("重置可见性"), this, [this]() {
        runDisplayAction(tr("重置可见性"), [this]() { return m_ViewerSdk->resetVisibility(); });
    });

    QMenu* help = menuBar()->addMenu(tr("帮助"));
    help->addAction(m_LicenseAction);
    help->addAction(tr("关于"), this, [this]() {
        QMessageBox::about(this, tr("关于 Hts Viewer SDK Demo"),
                           tr("面向交付验证的 Hts Viewer SDK 可视化宿主。\n"
                              "本程序只调用公开 HtsViewerSdk，不编译或访问 Viewer 内部源码。"));
    });
}

void ViewerMainWindow::createToolBar()
{
    QToolBar* toolbar = addToolBar(tr("Viewer"));
    toolbar->setMovable(false);
    toolbar->addAction(m_ImportAction);
    toolbar->addSeparator();
    toolbar->addActions(m_SelectionGroup->actions());
    toolbar->addSeparator();
    toolbar->addActions(m_GridPlaneGroup->actions());
    toolbar->addSeparator();
    toolbar->addAction(m_CadEdgesAction);
    toolbar->addAction(m_WireframeAction);
    toolbar->addAction(m_AxisVisibleAction);
    toolbar->addAction(m_HudVisibleAction);
    toolbar->addAction(m_FloorVisibleAction);
    toolbar->addAction(m_GroundGridVisibleAction);
    toolbar->addSeparator();
    toolbar->addAction(m_LicenseAction);
}

void ViewerMainWindow::createDockPanel()
{
    QDockWidget* dock = new QDockWidget(tr("Viewer 控制台"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget* panel = new QWidget(dock);
    QVBoxLayout* root = new QVBoxLayout(panel);

    QGroupBox* stateGroup = new QGroupBox(tr("当前状态"), panel);
    QFormLayout* state = new QFormLayout(stateGroup);
    m_ModeLabel = new QLabel(stateGroup);
    m_StyleLabel = new QLabel(stateGroup);
    m_TargetLabel = new QLabel(stateGroup);
    m_TargetLabel->setWordWrap(true);
    m_VisibleLabel = new QLabel(stateGroup);
    m_ColorLabel = new QLabel(stateGroup);
    m_OpacityLabel = new QLabel(stateGroup);
    m_HoverLabel = new QLabel(stateGroup);
    m_HoverLabel->setWordWrap(true);
    state->addRow(tr("选择模式"), m_ModeLabel);
    state->addRow(tr("显示风格"), m_StyleLabel);
    state->addRow(tr("选择目标"), m_TargetLabel);
    state->addRow(tr("可见"), m_VisibleLabel);
    state->addRow(tr("颜色"), m_ColorLabel);
    state->addRow(tr("不透明度"), m_OpacityLabel);
    state->addRow(tr("Hover"), m_HoverLabel);
    root->addWidget(stateGroup);

    QGroupBox* appearanceGroup = new QGroupBox(tr("选择对象外观"), panel);
    QVBoxLayout* appearance = new QVBoxLayout(appearanceGroup);
    QFormLayout* appearanceForm = new QFormLayout;
    m_OpacitySlider = new QSlider(Qt::Horizontal, appearanceGroup);
    m_OpacitySlider->setRange(5, 100);
    m_OpacitySlider->setValue(100);
    appearanceForm->addRow(tr("不透明度"), m_OpacitySlider);
    appearance->addLayout(appearanceForm);

    QHBoxLayout* colors = new QHBoxLayout;
    auto addColorButton = [appearanceGroup, colors](const QString& text) {
        QPushButton* button = new QPushButton(text, appearanceGroup);
        colors->addWidget(button);
        return button;
    };
    auto connectColor = [this](QPushButton* button, const hts::viewer::HtsColor4f& color) {
        connect(button, &QPushButton::clicked, this, [this, color]() {
            runDisplayAction(tr("设置颜色"), [this, color]() {
                return applySelectionColor(color);
            });
        });
    };
    connectColor(addColorButton(tr("红")), {0.86f, 0.20f, 0.18f, 1.0f});
    connectColor(addColorButton(tr("绿")), {0.22f, 0.72f, 0.34f, 1.0f});
    connectColor(addColorButton(tr("蓝")), {0.20f, 0.40f, 0.88f, 1.0f});
    connectColor(addColorButton(tr("黄")), {0.94f, 0.78f, 0.16f, 1.0f});
    connectColor(addColorButton(tr("灰")), {0.64f, 0.66f, 0.70f, 1.0f});
    appearance->addLayout(colors);

    QHBoxLayout* styleButtons = new QHBoxLayout;
    QPushButton* customColor = new QPushButton(tr("自定义颜色"), appearanceGroup);
    QPushButton* transparent = new QPushButton(tr("半透明"), appearanceGroup);
    QPushButton* resetStyle = new QPushButton(tr("恢复原始外观"), appearanceGroup);
    styleButtons->addWidget(customColor);
    styleButtons->addWidget(transparent);
    styleButtons->addWidget(resetStyle);
    appearance->addLayout(styleButtons);
    connect(customColor, &QPushButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(
                m_MaterialColor, this, tr("选择颜色"), QColorDialog::ShowAlphaChannel);
        if (!selected.isValid()) return;
        m_MaterialColor = selected;
        const hts::viewer::HtsColor4f color{
                float(selected.redF()), float(selected.greenF()),
                float(selected.blueF()), float(selected.alphaF())};
        runDisplayAction(tr("设置颜色"), [this, color]() {
            return applySelectionColor(color);
        });
    });
    connect(transparent, &QPushButton::clicked, this, [this]() {
        runDisplayAction(tr("设置半透明"), [this]() {
            return applySelectionOpacity(0.28f);
        });
    });
    connect(resetStyle, &QPushButton::clicked, this, [this]() {
        runDisplayAction(tr("恢复原始外观"), [this]() {
            const bool handled = m_ViewerSdk->resetSelectedStyle();
            if (handled) m_ViewerSdk->clearSelection();
            return handled;
        });
    });
    connect(m_OpacitySlider, &QSlider::sliderReleased, this, [this]() {
        runDisplayAction(tr("设置透明度"), [this]() {
            return applySelectionOpacity(m_OpacitySlider->value() / 100.0f);
        });
    });

    QHBoxLayout* visibility = new QHBoxLayout;
    auto addButton = [appearanceGroup, visibility](const QString& text) {
        QPushButton* button = new QPushButton(text, appearanceGroup);
        visibility->addWidget(button);
        return button;
    };
    connect(addButton(tr("隐藏")), &QPushButton::clicked, this, [this]() {
        runDisplayAction(tr("隐藏选择"), [this]() { return m_ViewerSdk->hideSelected(); });
    });
    connect(addButton(tr("隔离")), &QPushButton::clicked, this, [this]() {
        runDisplayAction(tr("隔离选择"), [this]() { return m_ViewerSdk->isolateSelected(); });
    });
    connect(addButton(tr("显示全部")), &QPushButton::clicked, this, [this]() {
        runDisplayAction(tr("显示全部"), [this]() { return m_ViewerSdk->showAll(); });
    });
    appearance->addLayout(visibility);
    root->addWidget(appearanceGroup);

    QGroupBox* materialGroup = new QGroupBox(tr("PBR 材质"), panel);
    QVBoxLayout* materialLayout = new QVBoxLayout(materialGroup);
    m_MaterialCombo = new QComboBox(materialGroup);
    addMaterial(m_MaterialCombo, tr("工程默认"), hts::viewer::HtsMaterialPreset::EngineeringDefault);
    addMaterial(m_MaterialCombo, tr("中性实体"), hts::viewer::HtsMaterialPreset::NeutralSolid);
    addMaterial(m_MaterialCombo, tr("PEC 金属"), hts::viewer::HtsMaterialPreset::PECMetal);
    addMaterial(m_MaterialCombo, tr("铝"), hts::viewer::HtsMaterialPreset::Aluminum);
    addMaterial(m_MaterialCombo, tr("铜"), hts::viewer::HtsMaterialPreset::Copper);
    addMaterial(m_MaterialCombo, tr("介质"), hts::viewer::HtsMaterialPreset::Dielectric);
    addMaterial(m_MaterialCombo, tr("FR4"), hts::viewer::HtsMaterialPreset::FR4);
    addMaterial(m_MaterialCombo, tr("吸波橡胶"), hts::viewer::HtsMaterialPreset::RubberAbsorber);
    addMaterial(m_MaterialCombo, tr("自定义"), hts::viewer::HtsMaterialPreset::Custom);
    materialLayout->addWidget(m_MaterialCombo);

    m_MaterialColorButton = new QPushButton(tr("自定义颜色"), materialGroup);
    materialLayout->addWidget(m_MaterialColorButton);
    connect(m_MaterialColorButton, &QPushButton::clicked, this, [this]() {
        const QColor selected = QColorDialog::getColor(
                m_MaterialColor, this, tr("选择材质颜色"), QColorDialog::ShowAlphaChannel);
        if (selected.isValid()) m_MaterialColor = selected;
    });

    QFormLayout* pbr = new QFormLayout;
    m_MetallicSlider = new QSlider(Qt::Horizontal, materialGroup);
    m_RoughnessSlider = new QSlider(Qt::Horizontal, materialGroup);
    m_SpecularSlider = new QSlider(Qt::Horizontal, materialGroup);
    for (QSlider* slider : {m_MetallicSlider, m_RoughnessSlider, m_SpecularSlider}) slider->setRange(0, 100);
    m_MetallicSlider->setValue(0);
    m_RoughnessSlider->setValue(55);
    m_SpecularSlider->setValue(35);
    pbr->addRow(tr("金属性"), m_MetallicSlider);
    pbr->addRow(tr("粗糙度"), m_RoughnessSlider);
    pbr->addRow(tr("高光"), m_SpecularSlider);
    materialLayout->addLayout(pbr);

    QPushButton* applyMaterial = new QPushButton(tr("应用到当前选择"), materialGroup);
    QPushButton* resetMaterial = new QPushButton(tr("恢复原始材质"), materialGroup);
    materialLayout->addWidget(applyMaterial);
    materialLayout->addWidget(resetMaterial);
    connect(applyMaterial, &QPushButton::clicked, this, [this]() {
        const auto preset = static_cast<hts::viewer::HtsMaterialPreset>(m_MaterialCombo->currentData().toInt());
        if (preset != hts::viewer::HtsMaterialPreset::Custom) {
            runDisplayAction(tr("应用材质预设"), [this, preset]() {
                return applySelectionMaterial(materialAppearance(presetMaterial(preset)));
            });
            return;
        }
        hts::viewer::HtsRenderMaterial material;
        material.baseColor = {float(m_MaterialColor.redF()), float(m_MaterialColor.greenF()),
                              float(m_MaterialColor.blueF()), float(m_MaterialColor.alphaF())};
        material.opacity = material.baseColor.a;
        material.metallic = m_MetallicSlider->value() / 100.0f;
        material.roughness = m_RoughnessSlider->value() / 100.0f;
        material.specular = m_SpecularSlider->value() / 100.0f;
        material.transparent = material.opacity < 0.999f;
        runDisplayAction(tr("应用自定义材质"), [this, material]() {
            return applySelectionMaterial(materialAppearance(material));
        });
    });
    connect(resetMaterial, &QPushButton::clicked, this, [this]() {
        runDisplayAction(tr("恢复材质"), [this]() {
            const bool handled = m_ViewerSdk->resetSelectedStyle();
            if (handled) m_ViewerSdk->clearSelection();
            return handled;
        });
    });
    connect(m_MaterialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int) {
                const auto preset = static_cast<hts::viewer::HtsMaterialPreset>(m_MaterialCombo->currentData().toInt());
                if (preset == hts::viewer::HtsMaterialPreset::Custom) return;
                const auto material = presetMaterial(preset);
                const QSignalBlocker metallicBlock(m_MetallicSlider);
                const QSignalBlocker roughnessBlock(m_RoughnessSlider);
                const QSignalBlocker specularBlock(m_SpecularSlider);
                m_MetallicSlider->setValue(int(material.metallic * 100.0f));
                m_RoughnessSlider->setValue(int(material.roughness * 100.0f));
                m_SpecularSlider->setValue(int(material.specular * 100.0f));
                m_MaterialColor = QColor::fromRgbF(material.baseColor.r, material.baseColor.g,
                                                   material.baseColor.b, material.opacity);
            });
    root->addWidget(materialGroup);

    m_InfoPanel = new QPlainTextEdit(panel);
    m_InfoPanel->setReadOnly(true);
    m_InfoPanel->setMinimumHeight(120);
    root->addWidget(m_InfoPanel, 1);
    panel->setLayout(root);
    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, dock);
}

void ViewerMainWindow::importModel()
{
    const QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("导入模型"),
            QString(),
            tr("CAD 模型 (*.step *.stp *.iges *.igs)"));
    if (!filePath.isEmpty()) startImportModelFile(filePath);
}

void ViewerMainWindow::startImportModelFile(const QString& filePath)
{
    hts::viewer::importing::HtsImportOptions options;
    const QByteArray objectId = QFileInfo(filePath).completeBaseName().toUtf8();
    options.defaultObjectId.assign(objectId.constData(), static_cast<std::size_t>(objectId.size()));

    // Demo 默认只导入几何显示数据，不读取 STEP/XCAF 原始颜色和材质。
    // CAD Edge 保持开启，用于工程化 CAD 显示。
    options.stepImportMode = hts::viewer::importing::HtsStepImportMode::FastGeometry;
    options.buildXcafColors = false;
    options.buildCadEdges = true;

    auto* progress = new QProgressDialog(
            tr("正在读取并离散 %1…").arg(QFileInfo(filePath).fileName()),
            tr("取消"), 0, 0, this);
    progress->setWindowTitle(tr("导入模型"));
    progress->setWindowModality(Qt::ApplicationModal);
    progress->setMinimumDuration(0);
    progress->show();

    struct ImportResult
    {
        bool success = false;
        hts::viewer::importing::HtsImportedModel model;
        std::string errorMessage;
    };
    auto result = std::make_shared<ImportResult>();
    auto cancelled = std::make_shared<std::atomic_bool>(false);
    connect(progress, &QProgressDialog::canceled, this, [cancelled, progress]() {
        cancelled->store(true);
        progress->setLabelText(QObject::tr("已请求取消；当前 OCCT 阶段完成后将停止提交。"));
    });

    QPointer<ViewerMainWindow> self(this);
    QPointer<QProgressDialog> progressGuard(progress);
    QThread* worker = QThread::create([self, progressGuard, cancelled, result, filePath, options]() {
        hts::viewer::importing::HtsDisplayDataImporter importer;
        const QByteArray utf8Path = filePath.toUtf8();
        result->success = importer.importFile(
                std::string(utf8Path.constData(), static_cast<std::size_t>(utf8Path.size())),
                options,
                result->model,
                result->errorMessage);
        if (!self) return;

        QMetaObject::invokeMethod(self.data(), [self, progressGuard, cancelled, result, filePath]() {
            if (!self) return;
            if (progressGuard) {
                progressGuard->hide();
                progressGuard->deleteLater();
            }
            if (cancelled->load()) {
                self->updateUiState(QObject::tr("导入已取消；原场景保持不变"));
                return;
            }
            if (!result->success || result->model.empty()) {
                const QString detail = !result->errorMessage.empty()
                                       ? QString::fromStdString(result->errorMessage)
                                       : QObject::tr("导入结果中没有可显示的三角形。");
                QMessageBox::warning(self, QObject::tr("导入失败"), detail);
                self->updateUiState(QObject::tr("导入失败"));
                return;
            }

            self->m_ViewerSdk->clearSelection();
            self->m_ViewerSdk->clearHover();
            self->m_ViewerSdk->clearScene();
            self->m_ViewerSdk->setDisplayStyle(
                    hts::viewer::HtsDisplayStyle::EngineeringDefault);
            self->m_ViewerSdk->showCadEdges(true);
            self->m_ViewerSdk->showTriangleWireframe(false);

            if (!self->m_ViewerSdk->upsertDisplayData(result->model.meshData)) {
                QMessageBox::warning(self, QObject::tr("导入失败"),
                                     QObject::tr("Viewer SDK 未接受导入后的显示数据。"));
                self->updateUiState(QObject::tr("导入提交失败"));
                return;
            }

            self->m_FitAfterDisplayCommand = self->m_ViewerSdk->hasPendingFrameWork();
            if (!self->m_FitAfterDisplayCommand) self->m_ViewerSdk->fitView();

            self->updateUiState(QObject::tr("已导入 %1").arg(QFileInfo(filePath).fileName()));
            self->m_ViewerWidget->update();
        }, Qt::QueuedConnection);
    });

    connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void ViewerMainWindow::showLicenseDialog()
{
    LicenseInfoDialog dialog(m_ViewerSdk, false, this);
    dialog.exec();
    updateUiState(dialog.isAuthorized() ? tr("Viewer 授权有效") : tr("Viewer 未授权"));
}

void ViewerMainWindow::setSelectionMode(hts::viewer::HtsSelectionMode mode)
{
    m_ViewerSdk->setSelectionMode(mode);
    updateUiState();
    m_ViewerWidget->update();
}

void ViewerMainWindow::setDisplayStyle(hts::viewer::HtsDisplayStyle style)
{
    m_ViewerSdk->setDisplayStyle(style);
    updateUiState();
    m_ViewerWidget->update();
}

void ViewerMainWindow::setGridPlane(hts::viewer::HtsGridPlane plane)
{
    const bool changed = m_ViewerSdk->setGridFrame(
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 1.0f},
            plane);
    if (!changed) return;

    m_XyPlaneAction->setChecked(plane == hts::viewer::HtsGridPlane::XOY);
    m_YzPlaneAction->setChecked(plane == hts::viewer::HtsGridPlane::YOZ);
    m_XzPlaneAction->setChecked(plane == hts::viewer::HtsGridPlane::XOZ);

    QString name = tr("XY");
    if (plane == hts::viewer::HtsGridPlane::YOZ) name = tr("YZ");
    if (plane == hts::viewer::HtsGridPlane::XOZ) name = tr("XZ");
    updateUiState(tr("工作平面已切换到 %1").arg(name));
    m_ViewerWidget->update();
}

void ViewerMainWindow::applyStandardView(
        const QString& name,
        const std::function<void()>& setView)
{
    if (setView) setView();
    // The SDK standard-view call changes orientation while preserving the
    // current pivot. Re-fit after the orientation change so an imported model
    // is centered exactly as it is in the engineering application.
    m_ViewerSdk->fitView();
    updateUiState(tr("已切换到%1").arg(name));
    m_ViewerWidget->update();
}

bool ViewerMainWindow::applySelectionColor(const hts::viewer::HtsColor4f& color)
{
    const bool handled = m_ViewerSdk->setSelectedColor(color);
    if (handled) {
        // Keep the Demo's current post-edit visual semantics: the applied
        // appearance is shown without the selection tint overlay.
        m_ViewerSdk->clearHover();
    }
    return handled;
}

bool ViewerMainWindow::applySelectionOpacity(float opacity)
{
    const float clampedOpacity = std::clamp(opacity, 0.05f, 1.0f);
    const bool handled = m_ViewerSdk->setSelectedOpacity(clampedOpacity);
    if (handled) {
        // Keep the Demo's current post-edit visual semantics: the applied
        // appearance is shown without the selection tint overlay.
        m_ViewerSdk->clearSelection();
        m_ViewerSdk->clearHover();
    }
    return handled;
}

bool ViewerMainWindow::applySelectionMaterial(
        const hts::viewer::HtsMaterialAppearance& appearance)
{
    const std::vector<hts::viewer::HtsSelectionTarget> targets =
            m_ViewerSdk->selectionTargets();
    if (targets.empty()) return false;

    m_ViewerSdk->clearSelection();
    m_ViewerSdk->clearHover();

    const bool onlyObjects = std::all_of(
            targets.begin(), targets.end(), [](const hts::viewer::HtsSelectionTarget& target) {
                return target.type == hts::viewer::HtsSelectionTargetType::Object;
            });
    if (onlyObjects) {
        std::vector<std::string> objectIds;
        objectIds.reserve(targets.size());
        for (const auto& target : targets) objectIds.push_back(target.objectId);
        return m_ViewerSdk->setObjectsMaterialAppearance(objectIds, appearance);
    }

    bool handled = false;
    for (const auto& target : targets) {
        if (target.type == hts::viewer::HtsSelectionTargetType::Edge
            || target.type == hts::viewer::HtsSelectionTargetType::Vertex) {
            handled = m_ViewerSdk->setTargetColorAndTransparency(
                    target, appearance.color, appearance.transparency) || handled;
        } else {
            handled = m_ViewerSdk->setTargetMaterialAppearance(
                    target, appearance) || handled;
        }
    }
    return handled;
}

bool ViewerMainWindow::runDisplayAction(
        const QString& name,
        const std::function<bool()>& action)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const bool success = action && action();
    QApplication::restoreOverrideCursor();
    updateUiState(success ? tr("%1完成").arg(name) : tr("%1未执行：请检查当前选择").arg(name));
    m_ViewerWidget->update();
    return success;
}

void ViewerMainWindow::updateUiState(const QString& message)
{
    if (!m_ViewerSdk) return;
    m_ModeLabel->setText(selectionModeText());
    m_StyleLabel->setText(displayStyleText());
    const QSignalBlocker cadEdgesBlocker(m_CadEdgesAction);
    const QSignalBlocker wireframeBlocker(m_WireframeAction);
    m_CadEdgesAction->setChecked(m_ViewerSdk->cadEdgesVisible());
    m_WireframeAction->setChecked(m_ViewerSdk->triangleWireframeVisible());

    const bool selected = m_ViewerSdk->hasSelection();
    const auto target = selected ? m_ViewerSdk->selectionTarget()
                                 : hts::viewer::HtsSelectionTarget{};
    const auto state = m_ViewerSdk->selectionDisplayState();
    m_TargetLabel->setText(targetText(target));
    m_VisibleLabel->setText(selected ? (state.visible ? tr("是") : tr("否")) : QStringLiteral("—"));
    m_OpacityLabel->setText(selected ? QString::number(state.opacity, 'f', 2) : QStringLiteral("—"));
    m_ColorLabel->setText(selected
                          ? QString("%1, %2, %3").arg(state.color.r, 0, 'f', 2)
                                  .arg(state.color.g, 0, 'f', 2)
                                  .arg(state.color.b, 0, 'f', 2)
                          : QStringLiteral("—"));
    m_HoverLabel->setText(QString::fromStdString(m_ViewerSdk->hoverSummary()));
    if (selected && !m_OpacitySlider->isSliderDown()) {
        const QSignalBlocker blocker(m_OpacitySlider);
        m_OpacitySlider->setValue(std::clamp(int(state.opacity * 100.0f), 5, 100));
    }
    m_InfoPanel->setPlainText(QString::fromStdString(m_ViewerSdk->displayStatsText()));
    if (!message.isEmpty()) m_StatusLabel->setText(message);
}

QString ViewerMainWindow::selectionModeText() const
{
    switch (m_ViewerSdk->selectionMode()) {
        case hts::viewer::HtsSelectionMode::Object: return tr("对象");
        case hts::viewer::HtsSelectionMode::Body: return tr("实体");
        case hts::viewer::HtsSelectionMode::Face: return tr("面");
        case hts::viewer::HtsSelectionMode::Edge: return tr("边");
        case hts::viewer::HtsSelectionMode::Vertex: return tr("点");
    }
    return tr("未知");
}

QString ViewerMainWindow::displayStyleText() const
{
    switch (m_ViewerSdk->displayStyle()) {
        case hts::viewer::HtsDisplayStyle::Shaded: return tr("着色");
        case hts::viewer::HtsDisplayStyle::ShadedWithCadEdges: return tr("着色 + CAD 边");
        case hts::viewer::HtsDisplayStyle::TriangleWireframe: return tr("三角线框");
        case hts::viewer::HtsDisplayStyle::EngineeringDefault: return tr("工程默认");
    }
    return tr("未知");
}
