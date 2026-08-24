#pragma once

#include "import/HtsImportOptions.h"
#include "import/HtsImportedModel.h"
#include "import/HtsViewerDemoImporterApi.h"

#include <string>

namespace hts::viewer::importing
{
/**
 * @brief STEP/IGES CAD 模型导入入口。
 *
 * 该类负责根据文件路径和导入参数生成 HtsImportedModel。它是 Viewer 导入链路的上层入口，
 * 用于屏蔽 QString/std::string 路径差异。文件读取和 OCCT 离散均封装在 Demo Importer DLL 内。
 */
class HTS_VIEWER_DEMO_IMPORTER_API HtsDisplayDataImporter
{
public:
    /**
     * @brief 使用标准字符串路径导入模型文件。
     *
     * @param filePath 待导入文件路径。
     * @param options 导入参数配置。
     * @param outModel 输出参数，保存导入得到的模型显示数据和附加信息。
     * @param errorMessage 输出参数，导入失败时保存错误信息。
     * @return 导入成功返回 true，否则返回 false。
     */
    bool importFile(const std::string& filePath,
                    const HtsImportOptions& options,
                    HtsImportedModel& outModel,
                    std::string& errorMessage) const;

};
}

