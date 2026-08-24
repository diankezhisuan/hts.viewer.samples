#pragma once

#include <string>

#include <QString>

namespace hts::viewer::importing
{
struct HtsImportOptions;

/**
 * @brief 面向导入链路的路径编码结果。
 *
 * 该结构保存原始路径、Qt 原生路径、传递给 OCCT 的路径以及路径编码策略诊断信息。
 * 主要用于处理中文路径、非 ASCII 路径以及必要时的本地编码或临时 ASCII 路径回退。
 */
struct HtsImportEncodedPath
{
    /** @brief 用户传入的原始路径。 */
    QString originalPath;

    /** @brief Qt 规范化后的本机路径。 */
    QString nativePath;

    /** @brief 实际传递给 OCCT 的路径字符串。 */
    std::string occtPath;

    /** @brief 本地 8-bit 编码形式的 OCCT 路径，用于兼容性回退。 */
    std::string local8BitOcctPath;

    /** @brief UTF-8 日志路径，用于稳定输出和问题诊断。 */
    std::string logPathUtf8;

    /** @brief 当前选择的路径编码策略名称。 */
    std::string codec;

    /** @brief 路径是否包含非 ASCII 字符。 */
    bool containsNonAscii = false;

    /** @brief 是否使用 UTF-8 路径。 */
    bool usedUtf8 = false;

    /** @brief 是否使用本地 8-bit 路径。 */
    bool usedLocal8Bit = false;

    /** @brief 是否使用临时 ASCII 路径副本。 */
    bool usedTemporaryAsciiCopy = false;

    /** @brief 临时 ASCII 路径副本位置，仅在 usedTemporaryAsciiCopy 为 true 时有效。 */
    QString temporaryAsciiPath;
};

/**
 * @brief 将导入路径编码为 OCCT 可使用的路径形式。
 *
 * @param path 原始 Qt 文件路径。
 * @return 返回路径编码结果和诊断信息。
 */
HtsImportEncodedPath encodeImportPathForOcct(const QString& path);

/**
 * @brief 将导入名称编码为可用于内部存储的字符串。
 *
 * @param name 原始 Qt 名称。
 * @return 返回编码后的存储名称。
 */
std::string encodeImportNameForStorage(const QString& name);

/**
 * @brief 将路径编码结果写入导入参数。
 *
 * @param options 待更新的导入参数。
 * @param encodedPath 路径编码结果。
 */
void applyEncodedImportPath(HtsImportOptions& options, const HtsImportEncodedPath& encodedPath);
}
