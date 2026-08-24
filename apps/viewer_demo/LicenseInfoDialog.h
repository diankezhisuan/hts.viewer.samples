#pragma once

#include <QDialog>

#include <memory>
#include <string>

namespace Ui { class LicenseInfoDialog; }
namespace hts::viewer { class HtsViewerSdk; }

/** Product-facing authorization dialog for the Hts3dViewer module only. */
class LicenseInfoDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit LicenseInfoDialog(
            std::shared_ptr<hts::viewer::HtsViewerSdk> viewerSdk,
            bool startupGate,
            QWidget* parent = nullptr);
    ~LicenseInfoDialog() override;

    void refresh();
    bool isAuthorized() const { return m_Authorized; }

private slots:
    void onExportMachineCode();
    void onImportLicense();

private:
    void updateDisplay();
    std::string nativePath(const QString& path) const;

private:
    std::unique_ptr<Ui::LicenseInfoDialog> ui;
    std::shared_ptr<hts::viewer::HtsViewerSdk> m_ViewerSdk;
    bool m_StartupGate = false;
    bool m_Authorized = false;
};
