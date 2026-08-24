#pragma once

#include "import/HtsImportedModel.h"
#include "import/HtsImportOptions.h"

#include <string>
#include <unordered_map>

#include <HtsViewerTypes.h>

#ifdef HTS_ENABLE_OCCT_IMPORT
class TopoDS_Shape;
#endif

namespace hts::viewer::importing
{
/**
 * @brief OCCT 面级材质覆盖配置。
 *
 * 用于在由 TopoDS_Shape 构建 Viewer DisplayData 时，为指定 faceTag 覆盖默认颜色或 XCAF 颜色。
 */
struct HtsOccFaceMaterialOverrides
{
    /** @brief 以 faceTag 为键的面颜色覆盖表。 */
    std::unordered_map<int, hts::viewer::HtsColor4f> faceColorsByTag;
};

/**
 * @brief OCCT Shape 到 Viewer DisplayData 的构建器。
 *
 * 该类负责将 OCCT TopoDS_Shape 离散并转换为 HtsImportedModel 中的显示网格数据，
 * 包括三角面、CAD 边线、面标签和可选的面级材质覆盖。
 */
class HtsOccShapeDisplayDataBuilder
{
public:
    /**
     * @brief 判断当前程序是否具备 OCCT Shape 显示数据构建能力。
     *
     * @return OCCT 构建能力可用时返回 true，否则返回 false。
     */
    bool isAvailable() const;
#ifdef HTS_ENABLE_OCCT_IMPORT
    /**
     * @brief 根据 OCCT Shape 构建 Viewer 导入模型数据。
     *
     * @param shape 待转换的 OCCT Shape。
     * @param options 导入和离散参数配置。
     * @param displayName 导入模型显示名称。
     * @param outModel 输出参数，保存构建得到的导入模型数据。
     * @param errorMessage 输出参数，构建失败时保存错误信息。
     * @param materialOverrides 可选面级材质覆盖配置；为空时使用默认颜色或导入属性颜色。
     * @return 构建成功返回 true，否则返回 false。
     */
    bool build(const TopoDS_Shape& shape,
               const HtsImportOptions& options,
               const std::string& displayName,
               HtsImportedModel& outModel,
               std::string& errorMessage,
               const HtsOccFaceMaterialOverrides* materialOverrides = nullptr) const;
#endif
    /**
     * @brief 在 OCCT Shape 构建能力不可用时生成失败结果。
     *
     * @param outModel 输出参数，保存占位或诊断用导入模型数据。
     * @param errorMessage 输出参数，保存不可用原因。
     * @return 通常返回 false，表示无法从 OCCT Shape 构建显示数据。
     */
    bool buildFromUnavailableOccShape(HtsImportedModel& outModel, std::string& errorMessage) const;
};
}

