#pragma once

#include "import/HtsImportOptions.h"
#include "import/HtsImportedModel.h"

#include <string>

class QString;

namespace hts::viewer::importing
{
/**
 * @brief OCCT 文件导入适配器。
 *
 * 该类负责将外部文件路径、导入参数和 OCCT 导入实现连接起来，并输出 Viewer 可使用的
 * HtsImportedModel。调用侧可通过 isAvailable() 判断当前构建是否启用了 OCCT 导入能力。
 */
class HtsOccImportAdapter
{
public:
    /**
     * @brief 判断当前程序是否具备 OCCT 导入能力。
     *
     * @return OCCT 导入实现可用时返回 true，否则返回 false。
     */
    bool isAvailable() const;

    /**
     * @brief 使用 Qt 字符串路径通过 OCCT 导入模型文件。
     *
     * @param filePath 待导入文件路径。
     * @param options 导入参数配置。
     * @param outModel 输出参数，保存导入得到的模型数据。
     * @param errorMessage 输出参数，导入失败时保存错误信息。
     * @return 导入成功返回 true，否则返回 false。
     */
    bool importFile(const QString& filePath,
                    const HtsImportOptions& options,
                    HtsImportedModel& outModel,
                    std::string& errorMessage) const;

    /**
     * @brief 使用标准字符串路径通过 OCCT 导入模型文件。
     *
     * @param filePath 待导入文件路径。
     * @param options 导入参数配置。
     * @param outModel 输出参数，保存导入得到的模型数据。
     * @param errorMessage 输出参数，导入失败时保存错误信息。
     * @return 导入成功返回 true，否则返回 false。
     */
    bool importFile(const std::string& filePath,
                    const HtsImportOptions& options,
                    HtsImportedModel& outModel,
                    std::string& errorMessage) const;
};
}

