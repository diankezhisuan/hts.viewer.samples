#include "import/HtsImportMemoryGuard.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

namespace hts::viewer::importing
{
namespace
{
std::uint64_t bytesToMB(std::uint64_t bytes)
{
    return bytes / (1024ULL * 1024ULL);
}

const char* budgetModeName(HtsMemoryBudgetMode mode)
{
    switch (mode) {
        case HtsMemoryBudgetMode::Fixed:
            return "Fixed";
        case HtsMemoryBudgetMode::Unlimited:
            return "Unlimited";
        case HtsMemoryBudgetMode::Auto:
        default:
            return "Auto";
    }
}

std::uint64_t computeReserveMB(const HtsImportOptions& options,
                               const HtsImportMemorySnapshot& snapshot)
{
    const double ratioReserve = double(snapshot.totalPhysicalMB) * options.systemReserveRatio;
    const std::uint64_t minimumReserve =
            static_cast<std::uint64_t>((std::max)(0, options.minSystemReserveMB));
    return (std::max)(static_cast<std::uint64_t>(std::ceil(ratioReserve)), minimumReserve);
}
}

HtsImportMemorySnapshot currentImportMemorySnapshot()
{
    HtsImportMemorySnapshot snapshot;
#ifdef _WIN32
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        snapshot.totalPhysicalMB = bytesToMB(memoryStatus.ullTotalPhys);
        snapshot.availablePhysicalMB = bytesToMB(memoryStatus.ullAvailPhys);
    }

    PROCESS_MEMORY_COUNTERS_EX counters;
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters))) {
        snapshot.workingSetMB = bytesToMB(counters.WorkingSetSize);
        snapshot.privateBytesMB = bytesToMB(counters.PrivateUsage);
    }
#endif
    return snapshot;
}

HtsImportMemoryBudgetDecision evaluateImportMemoryBudget(const HtsImportOptions& options,
                                                         double estimatedCpuMB,
                                                         double estimatedGpuMB,
                                                         const std::string& stageName)
{
    HtsImportMemoryBudgetDecision decision;
    decision.snapshot = currentImportMemorySnapshot();
    decision.reserveMB = computeReserveMB(options, decision.snapshot);
    decision.safeLimitMB = decision.snapshot.totalPhysicalMB > decision.reserveMB
            ? decision.snapshot.totalPhysicalMB - decision.reserveMB
            : 0;

    if (options.memoryBudgetMode == HtsMemoryBudgetMode::Unlimited) {
        decision.reason = "unlimited";
        return decision;
    }

    if (decision.snapshot.availablePhysicalMB > 0
            && options.lowAvailableMemoryWarnMB > 0
            && decision.snapshot.availablePhysicalMB < static_cast<std::uint64_t>(options.lowAvailableMemoryWarnMB)) {
        decision.warning = true;
    }

    if (options.memoryBudgetMode == HtsMemoryBudgetMode::Fixed) {
        if (options.memoryBudgetMB <= 0) {
            decision.reason = "fixed_budget_invalid_fallback_auto";
        } else if (estimatedCpuMB > double(options.memoryBudgetMB)) {
            decision.allowed = options.allowForceDisplayOverBudget;
            decision.forced = options.allowForceDisplayOverBudget;
            decision.reason = options.allowForceDisplayOverBudget
                    ? "fixed_budget_overridden"
                    : "fixed_memory_budget";
        } else {
            decision.reason = "fixed_budget_ok";
            return decision;
        }
    }

    if (options.memoryBudgetMode == HtsMemoryBudgetMode::Auto
            || decision.reason == "fixed_budget_invalid_fallback_auto") {
        if (decision.snapshot.availablePhysicalMB > 0
                && options.lowAvailableMemoryRejectMB > 0
                && decision.snapshot.availablePhysicalMB < static_cast<std::uint64_t>(options.lowAvailableMemoryRejectMB)) {
            decision.allowed = options.allowForceDisplayOverBudget;
            decision.forced = options.allowForceDisplayOverBudget;
            decision.reason = options.allowForceDisplayOverBudget
                    ? "low_available_memory_overridden"
                    : "low_available_memory";
            return decision;
        }

        const double estimatedCommitExtraMB = estimatedCpuMB + estimatedGpuMB * 0.5;
        const double projectedPrivateMB = double(decision.snapshot.privateBytesMB) + estimatedCommitExtraMB;
        if (decision.safeLimitMB > 0 && projectedPrivateMB > double(decision.safeLimitMB)) {
            decision.allowed = options.allowForceDisplayOverBudget;
            decision.forced = options.allowForceDisplayOverBudget;
            decision.reason = options.allowForceDisplayOverBudget
                    ? "auto_budget_overridden"
                    : "auto_memory_budget";
            return decision;
        }
        decision.reason = "auto_budget_ok";
    }

    (void)stageName;
    return decision;
}

void logImportMemoryLive(const std::string& stageName,
                         const HtsImportOptions& options,
                         const HtsImportMemoryBudgetDecision& decision)
{
    std::cout << "[ImportMemoryLive]"
              << " stage=" << stageName
              << " totalMB=" << decision.snapshot.totalPhysicalMB
              << " availableMB=" << decision.snapshot.availablePhysicalMB
              << " workingSetMB=" << decision.snapshot.workingSetMB
              << " privateBytesMB=" << decision.snapshot.privateBytesMB
              << " reserveMB=" << decision.reserveMB
              << " safeLimitMB=" << decision.safeLimitMB
              << std::endl;
    std::cout << "[ImportMemoryBudget]"
              << " mode=" << budgetModeName(options.memoryBudgetMode)
              << " decision=" << (decision.allowed ? "Allow" : "Reject")
              << " forced=" << (decision.forced ? "true" : "false")
              << " warning=" << (decision.warning ? "true" : "false")
              << " reason=" << decision.reason
              << std::endl;
}
}
