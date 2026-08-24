#pragma once

#include <HtsViewerSdk.h>

#include <QColor>
#include <QMainWindow>

#include <functional>
#include <memory>

class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSlider;
class HtsViewerWidget;

class ViewerMainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit ViewerMainWindow(
            std::shared_ptr<hts::viewer::HtsViewerSdk> viewerSdk,
            QWidget* parent = nullptr);
    ~ViewerMainWindow() override;

private:
    void createActions();
    void createMenus();
    void createToolBar();
    void createDockPanel();
    void importModel();
    void startImportModelFile(const QString& filePath);
    void showLicenseDialog();
    void setSelectionMode(hts::viewer::HtsSelectionMode mode);
    void setDisplayStyle(hts::viewer::HtsDisplayStyle style);
    void setGridPlane(hts::viewer::HtsGridPlane plane);
    void applyStandardView(const QString& name,
                           const std::function<void()>& setView);
    bool applySelectionColor(const hts::viewer::HtsColor4f& color);
    bool applySelectionOpacity(float opacity);
    bool applySelectionMaterial(const hts::viewer::HtsMaterialAppearance& appearance);
    bool runDisplayAction(const QString& name, const std::function<bool()>& action);
    void updateUiState(const QString& message = {});
    QString selectionModeText() const;
    QString displayStyleText() const;

private:
    std::shared_ptr<hts::viewer::HtsViewerSdk> m_ViewerSdk;
    HtsViewerWidget* m_ViewerWidget = nullptr;
    QPlainTextEdit* m_InfoPanel = nullptr;
    QLabel* m_StatusLabel = nullptr;
    QLabel* m_ModeLabel = nullptr;
    QLabel* m_StyleLabel = nullptr;
    QLabel* m_TargetLabel = nullptr;
    QLabel* m_VisibleLabel = nullptr;
    QLabel* m_ColorLabel = nullptr;
    QLabel* m_OpacityLabel = nullptr;
    QLabel* m_HoverLabel = nullptr;
    QSlider* m_OpacitySlider = nullptr;
    QComboBox* m_MaterialCombo = nullptr;
    QSlider* m_MetallicSlider = nullptr;
    QSlider* m_RoughnessSlider = nullptr;
    QSlider* m_SpecularSlider = nullptr;
    QPushButton* m_MaterialColorButton = nullptr;
    QColor m_MaterialColor{180, 196, 224, 255};
    bool m_FitAfterDisplayCommand = false;

    QAction* m_ImportAction = nullptr;
    QAction* m_ClearSceneAction = nullptr;
    QAction* m_LicenseAction = nullptr;
    QAction* m_ObjectModeAction = nullptr;
    QAction* m_FaceModeAction = nullptr;
    QAction* m_EdgeModeAction = nullptr;
    QAction* m_XyPlaneAction = nullptr;
    QAction* m_YzPlaneAction = nullptr;
    QAction* m_XzPlaneAction = nullptr;
    QAction* m_AxisVisibleAction = nullptr;
    QAction* m_HudVisibleAction = nullptr;
    QAction* m_FloorVisibleAction = nullptr;
    QAction* m_GroundGridVisibleAction = nullptr;
    QAction* m_CadEdgesAction = nullptr;
    QAction* m_WireframeAction = nullptr;
    QAction* m_EngineeringStyleAction = nullptr;
    QAction* m_ShadedStyleAction = nullptr;
    QAction* m_ShadedEdgesStyleAction = nullptr;
    QActionGroup* m_SelectionGroup = nullptr;
    QActionGroup* m_GridPlaneGroup = nullptr;
    QActionGroup* m_StyleGroup = nullptr;
};
