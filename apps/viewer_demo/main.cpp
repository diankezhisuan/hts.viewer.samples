#include "LicenseInfoDialog.h"
#include "ViewerMainWindow.h"

#include <HtsViewerSdk.h>

#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>

#include <memory>

int main(int argc, char* argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setSamples(0);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("HtsViewerDemo"));
    app.setApplicationDisplayName(QStringLiteral("Hts Viewer SDK"));
    app.setOrganizationName(QStringLiteral("Hts3d"));

    auto viewerSdk = std::make_shared<hts::viewer::HtsViewerSdk>();

    // Authorization is checked before OpenGL/Viewer initialization. An
    // unauthorized user remains inside the product UI and can export the
    // machine code or import the Viewer license without a separate tool.
    if (!viewerSdk->checkAuthorization()) {
        LicenseInfoDialog authorization(viewerSdk, true);
        if (authorization.exec() != QDialog::Accepted
                || !viewerSdk->checkAuthorization()) {
            return 0;
        }
    }

    ViewerMainWindow window(viewerSdk);
    window.resize(1360, 860);
    window.show();
    return app.exec();
}
