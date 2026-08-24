#pragma once

#include <cstddef>
#include <string>

#include <HtsViewerTypes.h>

namespace hts::viewer::importing
{
/**
 * @brief STEP 文件导入模式。
 *
 * 用于控制 STEP 导入时优先追求快速几何显示，还是通过 XCAF 路径保留颜色等属性信息。
 */
enum class HtsStepImportMode
{
    /** @brief 快速几何导入模式，优先生成显示几何数据。 */
    FastGeometry,

    /** @brief XCAF 颜色导入模式，优先保留 STEP/XCAF 中的颜色属性。 */
    XcafColors
};

/**
 * @brief 导入内存预算模式。
 *
 * 用于控制大模型导入时内存预算的计算方式和限制策略。
 */
enum class HtsMemoryBudgetMode
{
    /** @brief 自动预算模式，根据系统可用内存和保留策略动态判断。 */
    Auto,

    /** @brief 固定预算模式，使用 memoryBudgetMB 指定的固定预算。 */
    Fixed,

    /** @brief 不限制预算模式，通常仅用于明确允许的极限测试或调试场景。 */
    Unlimited
};

/**
 * @brief Viewer 模型导入参数。
 *
 * 该结构集中描述导入路径、OCCT 离散参数、颜色/边线构建、巨大模型策略和内存预算策略。
 * 调用侧应通过该结构统一控制导入链路，避免导入器、OCCT 适配层和 DisplayData 构建层出现分散配置。
 */
struct HtsImportOptions
{
    /** @brief 默认对象 ID，文件中无法解析出稳定对象 ID 时使用。 */
    std::string defaultObjectId = "ImportedObject";

    /** @brief 默认显示颜色，用于未携带材质或颜色属性的导入对象。 */
    hts::viewer::HtsColor4f defaultColor{0.70f, 0.77f, 0.88f, 1.0f};

    /** @brief STEP 文件导入模式。 */
    HtsStepImportMode stepImportMode = HtsStepImportMode::FastGeometry;

    /** @brief 单个显示分桶允许容纳的三角形上限。 */
    std::size_t bucketTriangleLimit = 500000;

    /** @brief OCCT 网格离散线性偏差。 */
    double linearDeflection = 0.1;

    /** @brief OCCT 网格离散角度偏差。 */
    double angularDeflection = 0.5;

    /** @brief 是否使用相对线性偏差进行离散。 */
    bool relativeDeflection = false;

    /** @brief 是否启用 OCCT 网格并行离散。 */
    bool meshInParallel = true;

    /** @brief 是否构建 XCAF 颜色信息。 */
    bool buildXcafColors = false;

    /** @brief 是否启用调试用 body 颜色，便于区分导入后的不同实体。 */
    bool enableDebugBodyColors = false;

    /** @brief 是否构建顶点级显示数据。 */
    bool buildVertexDisplayData = false;

    /** @brief 是否构建 CAD 边线显示数据。 */
    bool buildCadEdges = true;

    /** @brief 单条边最多采样点数量，用于控制 CAD 边线显示精度和数据规模。 */
    int maxEdgeSamplePoints = 32;

    /** @brief 是否根据文件规模自动进入巨大模型模式。 */
    bool autoHugeModelMode = true;

    /** @brief 触发巨大模型模式的文件大小阈值，单位为 MB。 */
    double hugeFileSizeMB = 512.0;

    /** @brief 巨大模型模式下根据包围盒尺度计算线性偏差的比例。 */
    double coarseDeflectionRatio = 0.0015;

    /** @brief 巨大模型模式下线性偏差最小值。 */
    double coarseMinDeflection = 0.1;

    /** @brief 巨大模型模式下角度偏差。 */
    double coarseAngle = 0.7;

    /** @brief 默认模式下根据包围盒尺度计算线性偏差的比例。 */
    double defaultDeflectionRatio = 0.0005;

    /** @brief 默认模式下线性偏差最小值。 */
    double defaultMinDeflection = 0.01;

    /** @brief 默认模式下角度偏差。 */
    double defaultAngle = 0.5;

    /** @brief 当前导入任务是否已经激活巨大模型模式。 */
    bool hugeModelModeActive = false;

    /** @brief 内存预算控制模式。 */
    HtsMemoryBudgetMode memoryBudgetMode = HtsMemoryBudgetMode::Auto;

    /** @brief 固定内存预算，单位为 MB；仅在 Fixed 模式下作为主要限制。 */
    int memoryBudgetMB = 0;

    /** @brief 系统最小保留内存，单位为 MB。 */
    int minSystemReserveMB = 4096;

    /** @brief 系统保留内存比例，用于 Auto 模式下计算安全预算。 */
    double systemReserveRatio = 0.10;

    /** @brief 可用内存低于该阈值时输出警告，单位为 MB。 */
    int lowAvailableMemoryWarnMB = 4096;

    /** @brief 可用内存低于该阈值时拒绝继续导入，单位为 MB。 */
    int lowAvailableMemoryRejectMB = 1024;

    /** @brief 是否允许用户或配置强制显示超出内存预算的模型。 */
    bool allowForceDisplayOverBudget = false;

    /** @brief 原始导入路径的 UTF-8 表示，用于日志和路径诊断。 */
    std::string originalPathUtf8;

    /** @brief 当前用于路径转换的编码名称。 */
    std::string pathCodec;

    /** @brief 面向 OCCT 的本地 8-bit 编码路径。 */
    std::string local8BitOcctPath;

    /** @brief 原始路径是否包含非 ASCII 字符。 */
    bool pathContainsNonAscii = false;

    /** @brief 是否允许使用本地 8-bit 路径作为 OCCT 路径回退方案。 */
    bool allowLocal8BitPathFallback = true;

    /** @brief 是否允许通过临时 ASCII 路径副本规避 OCCT 路径编码问题。 */
    bool allowTemporaryAsciiPathFallback = false;
};
}

