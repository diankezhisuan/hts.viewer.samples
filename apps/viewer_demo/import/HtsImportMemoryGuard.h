#pragma once

#include "import/HtsImportOptions.h"

#include <cstdint>
#include <string>

namespace hts::viewer::importing
{
/**
 * @brief 导入阶段的系统内存快照。
 *
 * 用于记录模型导入或 DisplayData 构建过程中与内存预算相关的关键指标，
 * 便于在大模型导入前后进行预算判断和日志诊断。
 */
struct HtsImportMemorySnapshot
{
    /** @brief 系统物理内存总量，单位为 MB。 */
    std::uint64_t totalPhysicalMB = 0;

    /** @brief 当前可用物理内存，单位为 MB。 */
    std::uint64_t availablePhysicalMB = 0;

    /** @brief 当前进程工作集内存，单位为 MB。 */
    std::uint64_t workingSetMB = 0;

    /** @brief 当前进程私有内存，单位为 MB。 */
    std::uint64_t privateBytesMB = 0;
};

/**
 * @brief 导入内存预算评估结果。
 *
 * 该结构描述当前导入阶段是否允许继续、是否由用户或配置强制允许、是否需要警告，
 * 以及计算得到的安全内存上限和系统保留量。
 */
struct HtsImportMemoryBudgetDecision
{
    /** @brief 是否允许继续当前导入或显示数据构建流程。 */
    bool allowed = true;

    /** @brief 是否通过强制策略放行超预算导入。 */
    bool forced = false;

    /** @brief 是否需要输出内存风险警告。 */
    bool warning = false;

    /** @brief 预算判断原因，通常用于日志或错误提示。 */
    std::string reason;

    /** @brief 当前策略要求预留给系统的内存，单位为 MB。 */
    std::uint64_t reserveMB = 0;

    /** @brief 当前导入流程可使用的安全内存上限，单位为 MB。 */
    std::uint64_t safeLimitMB = 0;

    /** @brief 执行预算判断时采集到的内存快照。 */
    HtsImportMemorySnapshot snapshot;
};

/**
 * @brief 获取当前导入相关的系统内存快照。
 *
 * @return 返回当前系统和进程内存信息。
 */
HtsImportMemorySnapshot currentImportMemorySnapshot();

/**
 * @brief 评估当前导入阶段的内存预算是否允许继续。
 *
 * @param options 导入参数配置。
 * @param estimatedCpuMB 预计 CPU 侧内存占用，单位为 MB。
 * @param estimatedGpuMB 预计 GPU 侧显存或顶点数据占用，单位为 MB。
 * @param stageName 当前导入阶段名称，用于日志和错误说明。
 * @return 返回内存预算评估结果。
 */
HtsImportMemoryBudgetDecision evaluateImportMemoryBudget(const HtsImportOptions& options,
                                                         double estimatedCpuMB,
                                                         double estimatedGpuMB,
                                                         const std::string& stageName);

/**
 * @brief 输出导入阶段的实时内存诊断日志。
 *
 * @param stageName 当前导入阶段名称。
 * @param options 导入参数配置。
 * @param decision 内存预算评估结果。
 */
void logImportMemoryLive(const std::string& stageName,
                         const HtsImportOptions& options,
                         const HtsImportMemoryBudgetDecision& decision);
}
