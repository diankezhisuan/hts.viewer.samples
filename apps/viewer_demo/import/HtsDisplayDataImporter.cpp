#include "import/HtsDisplayDataImporter.h"
#include "import/HtsOccImportAdapter.h"
#include <QFileInfo>
#include <QString>

namespace hts::viewer::importing
{
namespace
{
bool isOccImportExtension(const std::string& extension)
{
    return extension == "step" || extension == "stp"
            || extension == "iges" || extension == "igs";
}

bool importFileWithQtPath(const QString& filePath,
                          const HtsImportOptions& options,
                          HtsImportedModel& outModel,
                          std::string& errorMessage);
}

bool HtsDisplayDataImporter::importFile(const std::string& filePath,
                                        const HtsImportOptions& options,
                                        HtsImportedModel& outModel,
                                        std::string& errorMessage) const
{
    return importFileWithQtPath(QString::fromUtf8(filePath.c_str()),
                                options,
                                outModel,
                                errorMessage);
}

namespace
{
bool importFileWithQtPath(const QString& filePath,
                          const HtsImportOptions& options,
                          HtsImportedModel& outModel,
                          std::string& errorMessage)
{
    const std::string extension = QFileInfo(filePath).suffix().toLower().toStdString();
    if (isOccImportExtension(extension)) {
        HtsOccImportAdapter occAdapter;
        return occAdapter.importFile(filePath, options, outModel, errorMessage);
    }

    outModel = HtsImportedModel();
    errorMessage = "Unsupported import file extension: " + extension;
    return false;
}
}
}

