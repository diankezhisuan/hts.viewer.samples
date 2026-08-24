#include "import/HtsOccImportAdapter.h"

#include "import/HtsImportMemoryGuard.h"
#include "import/HtsImportPathCodec.h"
#include "import/HtsOccShapeDisplayDataBuilder.h"

#ifdef HTS_ENABLE_OCCT_IMPORT
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#if defined(HTS_OCCT_USE_DEIGES_PROVIDER)
#include <DEIGES_Provider.hxx>

#endif

#if defined(HTS_OCCT_USE_DESTEP_PROVIDER)

#include <DESTEP_Provider.hxx>

#endif

#if defined(HTS_OCCT_USE_IGES_CONTROL_READER) || defined(HTS_OCCT_USE_STEP_CONTROL_READER)

#include <IFSelect_ReturnStatus.hxx>

#endif

#if defined(HTS_OCCT_USE_IGES_CONTROL_READER)

#include <IGESControl_Reader.hxx>

#endif

#if defined(HTS_OCCT_USE_STEP_CONTROL_READER)
#include <STEPControl_Reader.hxx>
#endif
#if defined(HTS_OCCT_USE_STEP_CAF_READER)
#include <STEPCAFControl_Reader.hxx>
#include <TDocStd_Document.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <Quantity_Color.hxx>
#include <Quantity_ColorRGBA.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ColorType.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#endif
#include <Standard_Failure.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopoDS_Shape.hxx>
#endif

#include <QFileInfo>
#include <QString>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


namespace hts::viewer::importing

{

namespace
{
class ImportStageTimer
{
public:
    explicit ImportStageTimer(const std::string& name)
            : m_Name(name),
              m_Start(std::chrono::steady_clock::now())
    {
        std::cout << "[ImportStage] begin name=" << m_Name << std::endl;
    }

    ~ImportStageTimer()
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_Start).count();
        std::cout << "[ImportStage] end name=" << m_Name
                  << " elapsedMs=" << elapsed << std::endl;
    }

private:
    std::string m_Name;
    std::chrono::steady_clock::time_point m_Start;
};

std::string lowerExtension(const std::string& filePath)
{

    const std::size_t slash = filePath.find_last_of("/\\");

    const std::size_t dot = filePath.find_last_of('.');

    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {

        return {};

    }


    std::string extension = filePath.substr(dot + 1);

    std::transform(extension.begin(), extension.end(), extension.begin(),

                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

    return extension;

}


std::string fileStem(const std::string& filePath)
{

    const std::size_t slash = filePath.find_last_of("/\\");

    const std::size_t begin = slash == std::string::npos ? 0 : slash + 1;

    const std::size_t dot = filePath.find_last_of('.');

    const std::size_t end = (dot == std::string::npos || dot < begin) ? filePath.size() : dot;

    if (end <= begin) {

        return "ImportedOccShape";

    }

    return filePath.substr(begin, end - begin);
}

#ifdef HTS_ENABLE_OCCT_IMPORT
double fileSizeMB(const std::string& filePath)
{
    const QFileInfo fileInfo(QString::fromUtf8(filePath.c_str()));
    return fileInfo.exists() ? double(fileInfo.size()) / (1024.0 * 1024.0) : 0.0;
}

double shapeBboxDiagonal(const TopoDS_Shape& shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box);
    if (box.IsVoid()) {
        return 0.0;
    }

    Standard_Real xmin = 0.0;
    Standard_Real ymin = 0.0;
    Standard_Real zmin = 0.0;
    Standard_Real xmax = 0.0;
    Standard_Real ymax = 0.0;
    Standard_Real zmax = 0.0;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    const double dx = double(xmax - xmin);
    const double dy = double(ymax - ymin);
    const double dz = double(zmax - zmin);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

HtsImportOptions effectiveTessellationOptions(const HtsImportOptions& options,
                                              const std::string& filePath,
                                              const TopoDS_Shape& shape)
{
    HtsImportOptions effective = options;
    const double sizeMB = fileSizeMB(filePath);
    const double bboxDiag = shapeBboxDiagonal(shape);
    const bool hugeByFileSize = options.autoHugeModelMode && sizeMB >= options.hugeFileSizeMB;
    effective.hugeModelModeActive = hugeByFileSize;

    if (hugeByFileSize) {
        effective.linearDeflection = (std::max)(options.coarseMinDeflection,
                                                bboxDiag * options.coarseDeflectionRatio);
        effective.angularDeflection = options.coarseAngle;
        std::cout << "[ImportTessellation]"
                  << " mode=Coarse"
                  << " fileSizeMB=" << sizeMB
                  << " bboxDiag=" << bboxDiag
                  << " deflection=" << effective.linearDeflection
                  << " angle=" << effective.angularDeflection
                  << " reason=HugeModel"
                  << std::endl;
    } else {
        effective.linearDeflection = (std::max)(options.defaultMinDeflection,
                                                bboxDiag * options.defaultDeflectionRatio);
        effective.angularDeflection = options.defaultAngle;
        std::cout << "[ImportTessellation]"
                  << " mode=Default"
                  << " fileSizeMB=" << sizeMB
                  << " bboxDiag=" << bboxDiag
                  << " deflection=" << effective.linearDeflection
                  << " angle=" << effective.angularDeflection
                  << std::endl;
    }
    return effective;
}
#endif

bool isStepExtension(const std::string& extension)
{

    return extension == "step" || extension == "stp";

}


bool isIgesExtension(const std::string& extension)
{
    return extension == "iges" || extension == "igs";
}

struct ImportPathAttempt
{
    std::string path;
    std::string codec;
};

std::vector<ImportPathAttempt> buildPathAttempts(const std::string& filePath,
                                                 const HtsImportOptions& options)
{
    std::vector<ImportPathAttempt> attempts;
    attempts.push_back({filePath, options.pathCodec.empty() ? "UTF-8" : options.pathCodec});
    if (options.allowLocal8BitPathFallback
        && !options.local8BitOcctPath.empty()
        && options.local8BitOcctPath != filePath) {
        attempts.push_back({options.local8BitOcctPath, "QFile::encodeName/local8Bit"});
    }
    return attempts;
}

void logImportPathAttempt(const std::string& originalPath,
                          const ImportPathAttempt& attempt,
                          bool containsNonAscii)
{
    std::cout << "[ImportPath]"
              << " originalUtf8=" << originalPath
              << " occtPath=" << attempt.path
              << " containsNonAscii=" << (containsNonAscii ? "true" : "false")
              << " codec=" << attempt.codec
              << std::endl;
}

#ifdef HTS_ENABLE_OCCT_IMPORT
#if defined(HTS_OCCT_USE_STEP_CAF_READER)
hts::viewer::HtsColor4f toViewerColor(const Quantity_ColorRGBA& color)
{
    const Quantity_Color& rgb = color.GetRGB();
    return {static_cast<float>(rgb.Red()), static_cast<float>(rgb.Green()),
            static_cast<float>(rgb.Blue()), static_cast<float>(color.Alpha())};
}

bool readStepXcafShape(const std::string& filePath,
                       TopoDS_Shape& outShape,
                       HtsOccFaceMaterialOverrides& outMaterials,
                       std::string& errorMessage)
{
    STEPCAFControl_Reader reader;
    {
        ImportStageTimer stage("STEP XCAF read file");
        if (reader.ReadFile(filePath.c_str()) != IFSelect_RetDone) {
            errorMessage = "STEPCAF ReadFile failed: " + filePath;
            return false;
        }
    }

    if (reader.NbRootsForTransfer() <= 0) {
        errorMessage = "STEPCAF has no transferable roots: " + filePath;
        return false;
    }

    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) document;
    app->NewDocument(TCollection_ExtendedString("BinXCAF"), document);

    {
        ImportStageTimer stage("STEP XCAF transfer document");
        if (!reader.Transfer(document)) {
            errorMessage = "STEPCAF transfer failed: " + filePath;
            return false;
        }
    }

    {
        ImportStageTimer stage("STEP XCAF one shape");
        outShape = reader.Reader().OneShape();
    }
    if (outShape.IsNull()) {
        errorMessage = "STEPCAF transfer produced a null shape: " + filePath;
        return false;
    }

    Handle(XCAFDoc_ColorTool) colorTool =
            XCAFDoc_DocumentTool::ColorTool(document->Main());
    if (colorTool.IsNull()) {
        return true;
    }

    int faceTag = 0;
    int faceColorCount = 0;
    for (TopExp_Explorer explorer(outShape, TopAbs_FACE); explorer.More(); explorer.Next()) {
        ++faceTag;
        const TopoDS_Face face = TopoDS::Face(explorer.Current());
        Quantity_ColorRGBA color;
        if (colorTool->GetColor(face, XCAFDoc_ColorSurf, color)
            || colorTool->GetColor(face, XCAFDoc_ColorGen, color)) {
            outMaterials.faceColorsByTag[faceTag] = toViewerColor(color);
            ++faceColorCount;
        }
    }

    if (faceColorCount > 0) {
        std::cout << "[XCAFImport] colorSource=Face count=" << faceColorCount << std::endl;
    } else {
        std::cout << "[XCAFImport] materialCount=0 colorSource=none" << std::endl;
    }
    return true;
}
#endif

bool readStepShape(const std::string& filePath, TopoDS_Shape& outShape, std::string& errorMessage)
{
#if defined(HTS_OCCT_USE_STEP_CONTROL_READER)
    STEPControl_Reader reader;
    {
        ImportStageTimer stage("STEP read file");
        const IFSelect_ReturnStatus readStatus = reader.ReadFile(filePath.c_str());
        std::cout << "[StepReaderStats] readStatus=" << static_cast<int>(readStatus) << std::endl;
        if (readStatus != IFSelect_RetDone) {
            errorMessage = "STEP ReadFile failed. readStatus=" + std::to_string(static_cast<int>(readStatus))
                           + " path=" + filePath;
            return false;
        }
    }

    const Standard_Integer rootCount = reader.NbRootsForTransfer();
    std::cout << "[StepReaderStats] roots=" << rootCount << std::endl;
    if (rootCount <= 0) {
        errorMessage = "STEP has no transferable roots: " + filePath;
        return false;
    }

    Standard_Integer transferredRoots = 0;
    {
        ImportStageTimer stage("STEP transfer roots");
        transferredRoots = reader.TransferRoots();
    }
    std::cout << "[StepReaderStats] transferredRoots=" << transferredRoots << std::endl;
    if (transferredRoots <= 0) {
        errorMessage = "STEP TransferRoots failed. path=" + filePath;
        return false;
    }

    {
        ImportStageTimer stage("STEP one shape");
        outShape = reader.OneShape();
    }
    std::cout << "[StepReaderStats] shapeNull=" << (outShape.IsNull() ? "true" : "false") << std::endl;
    if (outShape.IsNull()) {
        errorMessage = "STEP transfer produced a null shape: " + filePath;
        return false;
    }
    return true;
#elif defined(HTS_OCCT_USE_DESTEP_PROVIDER)
    DESTEP_Provider provider;
    {
        ImportStageTimer stage("STEP read file");
        if (!provider.Read(TCollection_AsciiString(filePath.c_str()), outShape)) {
            errorMessage = "DESTEP Read failed: " + filePath;
            return false;
        }
    }
    if (outShape.IsNull()) {
        errorMessage = "DESTEP transfer produced a null shape: " + filePath;
        return false;
    }

    return true;

#else

    errorMessage = "No STEP reader is configured.";

    return false;

#endif

}


bool readIgesShape(const std::string& filePath, TopoDS_Shape& outShape, std::string& errorMessage)
{
#if defined(HTS_OCCT_USE_IGES_CONTROL_READER)
    ImportStageTimer stage("IGES read / transfer");
    IGESControl_Reader reader;
    // IGES 文件中可能包含用于曲面裁剪或辅助构造的隐藏根实体。
    // 这些实体若被一并 Transfer，最终会作为 Free Edge 显示在模型之外。
    reader.SetReadVisible(Standard_True);
    if (reader.ReadFile(filePath.c_str()) != IFSelect_RetDone) {

        errorMessage = "IGES ReadFile failed: " + filePath;

        return false;

    }


    const Standard_Integer rootCount = reader.NbRootsForTransfer();

    if (rootCount <= 0) {

        errorMessage = "IGES has no transferable roots: " + filePath;

        return false;

    }


    const Standard_Integer transferredRoots = reader.TransferRoots();

    if (transferredRoots <= 0) {

        errorMessage = "IGES visible root transfer failed: " + filePath;

        return false;

    }


    outShape = reader.OneShape();

    if (outShape.IsNull()) {

        errorMessage = "IGES transfer produced a null shape: " + filePath;

        return false;

    }

    return true;

#elif defined(HTS_OCCT_USE_DEIGES_PROVIDER)

    DEIGES_Provider provider;

    if (!provider.Read(TCollection_AsciiString(filePath.c_str()), outShape)) {

        errorMessage = "DEIGES Read failed: " + filePath;

        return false;

    }

    if (outShape.IsNull()) {

        errorMessage = "DEIGES transfer produced a null shape: " + filePath;

        return false;

    }

    return true;

#else

    errorMessage = "No IGES reader is configured.";

    return false;

#endif

}

#endif

}


bool HtsOccImportAdapter::isAvailable() const

{

#ifdef HTS_ENABLE_OCCT_IMPORT

    return true;

#else

    return false;

#endif

}


bool HtsOccImportAdapter::importFile(const std::string& filePath,
                                     const HtsImportOptions& options,
                                     HtsImportedModel& outModel,
                                     std::string& errorMessage) const
{
    return importFile(QString::fromUtf8(filePath.c_str()), options, outModel, errorMessage);
}

bool HtsOccImportAdapter::importFile(const QString& filePath,
                                     const HtsImportOptions& options,
                                     HtsImportedModel& outModel,
                                     std::string& errorMessage) const
{
    ImportStageTimer importStage("OCCT import transaction");
    outModel = HtsImportedModel();
    HtsImportOptions encodedOptions = options;
    const HtsImportEncodedPath encodedPath = encodeImportPathForOcct(filePath);
    applyEncodedImportPath(encodedOptions, encodedPath);
    const std::string extension = QFileInfo(filePath).suffix().toLower().toStdString();
    const std::string originalPath = encodedOptions.originalPathUtf8;
    const std::vector<ImportPathAttempt> pathAttempts = buildPathAttempts(encodedPath.occtPath, encodedOptions);
    logImportPathAttempt(originalPath, pathAttempts.front(), encodedOptions.pathContainsNonAscii);

    if (!isStepExtension(extension) && !isIgesExtension(extension)) {
        errorMessage = "OCCT import supports only STEP/IGES in this stage: "
                       + originalPath + " occtPath=" + pathAttempts.front().path + " codec=" + pathAttempts.front().codec;
        return false;
    }

#ifndef HTS_ENABLE_OCCT_IMPORT
    errorMessage = "OCCT import is not configured in this sandbox build: "
            + originalPath + " occtPath=" + pathAttempts.front().path + " codec=" + pathAttempts.front().codec;
    return false;
#else
    try {
        TopoDS_Shape shape;
        HtsOccFaceMaterialOverrides materialOverrides;
        bool hasMaterialOverrides = false;
        bool readOk = false;
        ImportPathAttempt activeAttempt = pathAttempts.front();
        std::string lastReadError;

        if (isStepExtension(extension)) {
            const bool useXcafColors =
                    encodedOptions.stepImportMode == HtsStepImportMode::XcafColors
                    || encodedOptions.buildXcafColors;
            std::cout << "[ImportMode] STEP mode="
                      << (useXcafColors ? "XcafColors" : "FastGeometry")
                      << std::endl;
            for (std::size_t attemptIndex = 0; attemptIndex < pathAttempts.size() && !readOk; ++attemptIndex) {
                const ImportPathAttempt& attempt = pathAttempts[attemptIndex];
                if (attemptIndex > 0) {
                    std::cout << "[ImportPath] utf8 failed, retry local8bit"
                              << " previousError=" << lastReadError << std::endl;
                    logImportPathAttempt(originalPath, attempt, encodedOptions.pathContainsNonAscii);
                }
#if defined(HTS_OCCT_USE_STEP_CAF_READER)
                if (useXcafColors) {
                    std::string xcafError;
                    readOk = readStepXcafShape(attempt.path, shape, materialOverrides, xcafError);
                    hasMaterialOverrides = readOk && !materialOverrides.faceColorsByTag.empty();
                    if (!readOk) {
                        std::cout << "[XCAFImport] fallback=FastGeometry reason=" << xcafError << std::endl;
                        lastReadError = xcafError;
                    }
                }
#else
                if (useXcafColors) {
                    std::cout << "[XCAFImport] unavailable=true fallback=FastGeometry" << std::endl;
                }
#endif
                if (!readOk) {
                    readOk = readStepShape(attempt.path, shape, lastReadError);
                }
                if (readOk) {
                    activeAttempt = attempt;
                    errorMessage.clear();
                } else {
                    errorMessage = lastReadError;
                }
            }
        } else {
            for (std::size_t attemptIndex = 0; attemptIndex < pathAttempts.size() && !readOk; ++attemptIndex) {
                const ImportPathAttempt& attempt = pathAttempts[attemptIndex];
                if (attemptIndex > 0) {
                    std::cout << "[ImportPath] utf8 failed, retry local8bit"
                              << " previousError=" << lastReadError << std::endl;
                    logImportPathAttempt(originalPath, attempt, encodedOptions.pathContainsNonAscii);
                }
                readOk = readIgesShape(attempt.path, shape, lastReadError);
                if (readOk) {
                    activeAttempt = attempt;
                    errorMessage.clear();
                } else {
                    errorMessage = lastReadError;
                }
            }
        }

        if (!readOk) {
            errorMessage += " originalUtf8=" + originalPath
                            + " occtPath=" + activeAttempt.path
                            + " codec=" + activeAttempt.codec;
            if (encodedOptions.allowTemporaryAsciiPathFallback) {
                errorMessage += " temporaryAsciiCopyFallback=not_implemented";
            }
            return false;
        }

        {
            const HtsImportMemoryBudgetDecision memoryDecision =
                    evaluateImportMemoryBudget(encodedOptions, 0.0, 0.0, "STEP/IGES transfer complete");
            logImportMemoryLive("STEP/IGES transfer complete", encodedOptions, memoryDecision);
            if (!memoryDecision.allowed) {
                std::cout << "[ImportRejected]"
                          << " reason=live_memory_budget"
                          << " stage=STEP/IGES transfer complete"
                          << " availableMB=" << memoryDecision.snapshot.availablePhysicalMB
                          << " privateBytesMB=" << memoryDecision.snapshot.privateBytesMB
                          << " safeLimitMB=" << memoryDecision.safeLimitMB
                          << std::endl;
                errorMessage = "Import rejected by live memory budget after STEP/IGES transfer. reason="
                               + memoryDecision.reason;
                outModel = HtsImportedModel();
                return false;
            }
        }

        std::cout << "[ImportStage] begin name=root shape obtained" << std::endl;
        std::cout << "[ImportStage] end name=root shape obtained elapsedMs=0" << std::endl;

        HtsImportOptions effectiveOptions = effectiveTessellationOptions(encodedOptions, activeAttempt.path, shape);

        HtsOccShapeDisplayDataBuilder builder;
        if (!builder.build(shape,
                           effectiveOptions,
                           fileStem(originalPath),
                           outModel,
                           errorMessage,
                           hasMaterialOverrides ? &materialOverrides : nullptr)) {
            errorMessage += " originalUtf8=" + originalPath
                            + " occtPath=" + activeAttempt.path
                            + " codec=" + activeAttempt.codec;
            return false;
        }

        outModel.sourceFilePath = originalPath;
        return true;
    } catch (const Standard_Failure& failure) {
        errorMessage = std::string("OCCT import failed: ") + failure.GetMessageString()
                       + " originalUtf8=" + originalPath
                       + " occtPath=" + pathAttempts.front().path
                       + " codec=" + pathAttempts.front().codec;
        outModel = HtsImportedModel();
        return false;
    } catch (...) {
        errorMessage = "OCCT import failed with unknown exception: originalUtf8="
                       + originalPath + " occtPath=" + pathAttempts.front().path + " codec=" + pathAttempts.front().codec;
        outModel = HtsImportedModel();
        return false;
    }
#endif

}

}

