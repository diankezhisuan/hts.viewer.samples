#pragma once

#include <HtsDisplayData.h>

#include <string>
#include <vector>

namespace hts::viewer::importing
{
/**
 * @brief 导入后的 Viewer 模型数据。
 *
 * 该结构保存一次导入产生的源文件路径、显示名称、DisplayData 网格数据以及导入过程消息。
 * 它是导入模块向 Viewer 显示管理层传递数据的轻量结果对象。
 */
struct HtsImportedModel
{
    /** @brief 源模型文件路径，通常用于日志、缓存键或后续导入状态追踪。 */
    std::string sourceFilePath;

    /** @brief 模型显示名称，通常由文件名或导入配置生成。 */
    std::string displayName;

    /** @brief 导入得到的 Viewer 显示网格数据。 */
    hts::viewer::HtsDisplayMeshData meshData;

    /** @brief 导入过程中的提示、警告或诊断消息。 */
    std::vector<std::string> messages;

    /**
     * @brief 判断导入模型是否为空。
     *
     * @return 当 meshData 中没有任何显示对象时返回 true，否则返回 false。
     */
    bool empty() const
    {
        return meshData.objects.empty();
    }
};
}

