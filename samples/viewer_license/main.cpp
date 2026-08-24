#include <HtsViewerSdk.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
const char* authorizationTypeText(hts::viewer::HtsAuthorizationType type)
{
    switch (type) {
        case hts::viewer::HtsAuthorizationType::Trial: return "TRIAL";
        case hts::viewer::HtsAuthorizationType::Formal: return "FORMAL";
        case hts::viewer::HtsAuthorizationType::Unknown:
        default: return "UNKNOWN";
    }
}

std::string expirationText(std::int64_t unixSeconds)
{
    if (unixSeconds <= 0) return "Unavailable";
    const std::time_t value = static_cast<std::time_t>(unixSeconds);
    std::tm local{};
#ifdef _WIN32
    if (localtime_s(&local, &value) != 0) return "Unavailable";
#else
    if (!localtime_r(&value, &local)) return "Unavailable";
#endif
    std::ostringstream stream;
    stream << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}
}

int main(int argc, char* argv[]){

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    namespace fs = std::filesystem;
    hts::viewer::HtsViewerSdk viewer;

    fs::path licenseDirectory;
    if (argc > 1 && argv[1] && argv[1][0] != '\0') {
        licenseDirectory = fs::absolute(fs::path(argv[1])).lexically_normal();
    } else {
        std::error_code pathError;
        fs::path executablePath = fs::weakly_canonical(fs::path(argv[0]), pathError);
        if (pathError) {
            executablePath = fs::absolute(fs::path(argv[0])).lexically_normal();
        }

        // Keep generated machine code and imported files beside the installed
        // tool, never in a guessed source-tree parent directory.
        licenseDirectory = executablePath.parent_path() / "license";
    }

    std::error_code errorCode;
    fs::create_directories(licenseDirectory, errorCode);
    if (errorCode) {
        std::cerr << "Failed to create license directory: "
                  << licenseDirectory.string()
                  << std::endl;
        return EXIT_FAILURE;
    }

    const fs::path machineCodePath =
            licenseDirectory / "Hts3dViewer.machine";

    std::cout << "========================================" << std::endl;
    std::cout << " Hts3d Viewer License Tool" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "License directory:" << std::endl;
    std::cout << "  " << licenseDirectory.string() << std::endl;
    std::cout << std::endl;

    // 每次运行均重新生成当前机器的机器码，确保文件与当前机器一致。
    if (!viewer.exportMachineCode(machineCodePath.string())) {
        std::cerr << "Failed to export machine code." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Machine code exported:" << std::endl;
    std::cout << "  " << machineCodePath.string() << std::endl;
    std::cout << std::endl;

    // 搜索授权目录中的全部.lic文件。
    std::vector<fs::path> licenseFiles;

    errorCode.clear();
    for (fs::directory_iterator it(licenseDirectory, errorCode), end;
         it != end && !errorCode;
         it.increment(errorCode)) {
        if (!it->is_regular_file()) {
            continue;
        }

        std::string extension = it->path().extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char ch) {
                           return static_cast<char>(std::tolower(ch));
                       });

        if (extension == ".lic") {
            licenseFiles.emplace_back(it->path());
        }
    }

    if (errorCode) {
        std::cerr << "Failed to scan license directory: "
                  << errorCode.message()
                  << std::endl;
        return EXIT_FAILURE;
    }

    bool authorized = false;

    if (!licenseFiles.empty()) {
        std::cout << "License file found:" << std::endl;
        for (const auto& file : licenseFiles) {
            std::cout << "  " << file.filename().string() << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Importing license..." << std::endl;

        const bool importSucceeded =
                viewer.importLicense(licenseDirectory.string());

        authorized = viewer.checkAuthorization();

        if (importSucceeded) {
            std::cout << "License import completed." << std::endl;
        } else {
            std::cout << "License import failed or no valid Viewer license "
                         "was imported."
                      << std::endl;
        }
    } else {
        std::cout << "No .lic file found in license directory." << std::endl;
        std::cout << "Checking existing Viewer authorization..." << std::endl;

        authorized = viewer.checkAuthorization();
    }

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;

    if (authorized) {
        std::cout << " Viewer license status: AUTHORIZED" << std::endl;
        std::cout << " Authorization type: "
                  << authorizationTypeText(viewer.authorizationType()) << std::endl;
        std::cout << " Valid until: "
                  << expirationText(viewer.authorizationExpirationTime()) << std::endl;
        std::cout << "========================================" << std::endl;
        return EXIT_SUCCESS;
    }

    std::cout << " Viewer license status: NOT AUTHORIZED" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    if (licenseFiles.empty()) {
        std::cout << "Please send the following machine code file "
                     "to the supplier:"
                  << std::endl;
        std::cout << "  " << machineCodePath.string() << std::endl;
        std::cout << std::endl;
        std::cout << "After receiving the Viewer .lic file, place it in:"
                  << std::endl;
        std::cout << "  " << licenseDirectory.string() << std::endl;
        std::cout << std::endl;
        std::cout << "Then run this tool again." << std::endl;
    } else {
        std::cout << "A .lic file exists, but the Hts3dViewer authorization "
                     "is not valid."
                  << std::endl;
        std::cout << "Please verify that the license matches this machine "
                     "and the Hts3dViewer module."
                  << std::endl;
    }

    return EXIT_FAILURE;
}
