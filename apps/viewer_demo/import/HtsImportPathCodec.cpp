#include "import/HtsImportPathCodec.h"

#include "import/HtsImportOptions.h"

#include <QByteArray>
#include <QDir>

namespace hts::viewer::importing
{
namespace
{
bool hasNonAscii(const QString& text)
{
    for (const QChar ch : text) {
        if (ch.unicode() > 0x7f) {
            return true;
        }
    }
    return false;
}
}

HtsImportEncodedPath encodeImportPathForOcct(const QString& path)
{
    HtsImportEncodedPath encodedPath;
    encodedPath.originalPath = path;
    encodedPath.nativePath = QDir::toNativeSeparators(path);
    encodedPath.logPathUtf8 = encodedPath.nativePath.toUtf8().constData();
    encodedPath.containsNonAscii = hasNonAscii(path);
    const QByteArray utf8 = encodedPath.nativePath.toUtf8();
    encodedPath.occtPath.assign(utf8.constData(), utf8.size());
    encodedPath.codec = "QString::toUtf8";
    encodedPath.usedUtf8 = true;
    return encodedPath;
}

std::string encodeImportNameForStorage(const QString& name)
{
    const QByteArray encoded = name.toUtf8();
    return std::string(encoded.constData(), encoded.size());
}

void applyEncodedImportPath(HtsImportOptions& options, const HtsImportEncodedPath& encodedPath)
{
    options.originalPathUtf8 = encodedPath.logPathUtf8;
    options.pathCodec = encodedPath.codec;
    options.local8BitOcctPath = encodedPath.local8BitOcctPath;
    options.pathContainsNonAscii = encodedPath.containsNonAscii;
}
}

