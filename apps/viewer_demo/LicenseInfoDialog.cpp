#include "LicenseInfoDialog.h"

#include "ui_LicenseInfoDialog.h"

#include <HtsViewerSdk.h>

#include <QDir>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QTableWidgetItem>

LicenseInfoDialog::LicenseInfoDialog(
        std::shared_ptr<hts::viewer::HtsViewerSdk> viewerSdk,
        bool startupGate,
        QWidget* parent)
    : QDialog(parent)
    , ui(std::make_unique<Ui::LicenseInfoDialog>())
    , m_ViewerSdk(std::move(viewerSdk))
    , m_StartupGate(startupGate)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowModality(Qt::ApplicationModal);
    connect(ui->btnExportMachineCode, &QPushButton::clicked,
            this, &LicenseInfoDialog::onExportMachineCode);
    connect(ui->btnImportLicense, &QPushButton::clicked,
            this, &LicenseInfoDialog::onImportLicense);
    refresh();
}

LicenseInfoDialog::~LicenseInfoDialog() = default;

void LicenseInfoDialog::refresh()
{
    m_Authorized = m_ViewerSdk && m_ViewerSdk->checkAuthorization();
    updateDisplay();
}

void LicenseInfoDialog::updateDisplay()
{
    const hts::viewer::HtsAuthorizationType type = m_ViewerSdk
            ? m_ViewerSdk->authorizationType()
            : hts::viewer::HtsAuthorizationType::Unknown;
    const std::int64_t expiration = m_ViewerSdk
            ? m_ViewerSdk->authorizationExpirationTime() : 0;
    QString typeText = tr("未知");
    if (type == hts::viewer::HtsAuthorizationType::Trial) typeText = tr("试用许可");
    if (type == hts::viewer::HtsAuthorizationType::Formal) typeText = tr("正式许可");
    const QDateTime expirationDate = expiration > 0
            ? QDateTime::fromSecsSinceEpoch(expiration).toLocalTime() : QDateTime();
    const QString expirationText = expirationDate.isValid()
            ? expirationDate.toString("yyyy-MM-dd HH:mm:ss") : tr("暂不可用");
    const qint64 remainingSeconds = expirationDate.isValid()
            ? QDateTime::currentDateTime().secsTo(expirationDate) : 0;
    const qint64 remainingDays = remainingSeconds > 0
            ? (remainingSeconds + 86399) / 86400 : 0;

    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setHorizontalHeaderLabels({tr("产品模块"), tr("授权类型"), tr("有效期至")});
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableWidget->verticalHeader()->setVisible(false);
    ui->tableWidget->setRowCount(1);
    ui->tableWidget->setItem(0, 0, new QTableWidgetItem(QStringLiteral("Hts3dViewer")));
    ui->tableWidget->setItem(0, 1, new QTableWidgetItem(typeText));
    ui->tableWidget->setItem(0, 2, new QTableWidgetItem(expirationText));
    for (int column = 0; column < 3; ++column) {
        ui->tableWidget->item(0, column)->setTextAlignment(Qt::AlignCenter);
        ui->tableWidget->item(0, column)->setFlags(
                ui->tableWidget->item(0, column)->flags() & ~Qt::ItemIsEditable);
    }

    if (m_Authorized && type == hts::viewer::HtsAuthorizationType::Formal) {
        ui->labelStatus->setText(tr("✓ Viewer 正式授权有效"));
        ui->labelStatus->setStyleSheet("font-size:18px;font-weight:600;color:#18794e;padding:8px;");
        ui->descriptionLabel->setText(
                tr("当前设备已获得 Hts3dViewer 正式使用许可，有效期至 %1。")
                        .arg(expirationText));
    } else if (m_Authorized && type == hts::viewer::HtsAuthorizationType::Trial) {
        ui->labelStatus->setText(tr("Viewer 试用许可"));
        ui->labelStatus->setStyleSheet("font-size:18px;font-weight:600;color:#9a6700;padding:8px;");
        ui->descriptionLabel->setText(
                tr("当前为功能试用许可，有效期至 %1，剩余约 %2 天。请在到期前联系供应商获取正式授权。")
                        .arg(expirationText).arg(remainingDays));
    } else {
        ui->labelStatus->setText(tr("Viewer 尚未获得有效授权"));
        ui->labelStatus->setStyleSheet("font-size:18px;font-weight:600;color:#b42318;padding:8px;");
        ui->descriptionLabel->setText(expirationDate.isValid() && remainingSeconds <= 0
                ? tr("当前许可已于 %1 到期。请导出机器码并联系供应商更新授权。")
                        .arg(expirationText)
                : tr("请先导出本机机器码并发送给供应商，收到授权文件后在此导入。"));
    }
    ui->operationLabel->clear();
}

void LicenseInfoDialog::onExportMachineCode()
{
    if (!m_ViewerSdk) return;

    const QString documents = QStandardPaths::writableLocation(
            QStandardPaths::DocumentsLocation);
    const QString filePath = QFileDialog::getSaveFileName(
            this,
            tr("导出 Hts Viewer 机器码"),
            QDir(documents).filePath(QStringLiteral("Hts3dViewer.machine")),
            tr("Viewer 机器码 (*.machine)"));
    if (filePath.isEmpty()) return;

    if (!m_ViewerSdk->exportMachineCode(nativePath(filePath))) {
        QMessageBox::critical(this, tr("导出失败"),
                              tr("无法写入机器码文件，请检查目标目录权限。"));
        return;
    }

    ui->operationLabel->setText(tr("机器码已导出：%1").arg(QDir::toNativeSeparators(filePath)));
    QMessageBox::information(this, tr("导出完成"),
                             tr("Hts3dViewer 机器码已成功导出。"));
}

void LicenseInfoDialog::onImportLicense()
{
    if (!m_ViewerSdk) return;

    const QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("导入 Hts Viewer 授权文件"),
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
            tr("Hts 授权文件 (*.lic)"));
    if (filePath.isEmpty()) return;

    const QString directory = QFileInfo(filePath).absolutePath();
    const bool imported = m_ViewerSdk->importLicense(nativePath(directory));
    const bool authorized = m_ViewerSdk->checkAuthorization();
    m_Authorized = authorized;
    updateDisplay();

    if (!authorized) {
        QMessageBox::critical(
                this,
                tr("授权失败"),
                              tr("未能获得有效的 Hts3dViewerSDK 授权。请确认授权文件属于本机且包含 Viewer SDK 模块。"));
        return;
    }

    QMessageBox::information(this, tr("授权成功"),
                             imported
                             ? tr("Hts3dViewer 授权已生效，现在可以进入可视化界面。")
                             : tr("当前目录中的授权已生效，现在可以进入可视化界面。"));
    if (m_StartupGate) accept();
}

std::string LicenseInfoDialog::nativePath(const QString& path) const
{
    const QByteArray encoded = QDir::toNativeSeparators(path).toLocal8Bit();
    return std::string(encoded.constData(), static_cast<std::size_t>(encoded.size()));
}
