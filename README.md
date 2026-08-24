# Hts Viewer SDK 2.0 Samples

Hts Viewer SDK 是面向 CAD、CAE 和工业三维软件的 C++ 可视化 SDK。本仓库是面向
客户和合作开发者的独立交付工程：只链接预编译 Viewer SDK，不包含也不编译 Viewer
内核源码。

## 第一次使用

1. 打开并构建根目录 `CMakeLists.txt`（Windows x64、Release）。
2. 想先体验产品功能，运行 `HtsViewerDemo.exe`。
3. 首次授权建议通过 `HtsViewerDemo` 的“帮助 → 授权管理”。
4. 想学习 SDK 接口，从 `viewer_quickstart` 开始。
5. 离线开发手册入口是 `Documentation/index.html`。

## 本仓库中的项目

| 项目 | 面向对象 | 主要用途 |
|---|---|---|
| `apps/viewer_demo` | 最终用户、集成评估人员 | 完整 Qt 产品 Demo：授权、STEP/IGES 导入、对象/面/边选择、多选、标准视图、工作平面、地板、地面网格、HUD、颜色、透明度和 PBR 材质 |
| `samples/viewer_license` | 首次部署人员 | 导出机器码、导入授权、查询授权是否有效、授权类型以及到期时间 |
| `samples/viewer_quickstart` | 第一次接入 SDK 的开发者 | 初始化、DisplayData、增量增删、显隐、视图、地板、地网格、坐标轴和 HUD |
| `samples/viewer_interaction` | CAD 交互开发者 | 对象/面/边选择、Ctrl 多选、选择 Dim、Fit Selection、临时 Preview 和局部坐标轴 |
| `samples/viewer_appearance` | 显示和材质开发者 | 显示模式、CAD 边、线框、颜色、透明度、PBR 金属性/粗糙度/高光和背景 |
| `samples/viewer_mesh` | CAE/Solver 开发者 | 网格事务提交、分块更新、Part 样式、显隐、隔离、选择和统计 |
| `ref/hts.viewer/win` | CMake 和链接器 | 正式公开头文件、导入库、Viewer DLL 和授权运行库 |
| `Documentation` | 所有开发者 | 离线 Developer Guide、Best Practices 和 Public API Reference |

完整 Demo 用于体验“产品如何工作”；四个 API Sample 用于回答“业务代码应该调用哪个
SDK 接口”。普通 Sample 不依赖 Qt、OCCT 或 Viewer 源码。

## 目录结构

```text
hts.viewer.samples/
├─ CMakeLists.txt
├─ README.md
├─ Documentation/
│  ├─ index.html
│  └─ html/
├─ ref/hts.viewer/win/
│  ├─ include/
│  ├─ lib/
│  └─ bin/
├─ apps/viewer_demo/
└─ samples/
   ├─ common/
   ├─ viewer_license/
   ├─ viewer_quickstart/
   ├─ viewer_interaction/
   ├─ viewer_appearance/
   └─ viewer_mesh/
```

## 授权说明

Viewer 授权分为：

- `Trial`：试用许可，界面会显示“试用许可”和具体到期时间；
- `Formal`：正式许可，界面会显示“正式授权有效”和具体到期时间；
- `Unknown`：没有有效授权、授权已失效或授权信息不可用。

Demo 的授权窗口支持直接导出机器码和导入 `.lic` 文件。授权写入本机后，Demo 和所有
API Sample 通用，不需要逐个 Sample 重新授权。

## 构建

环境要求：Windows x64、Visual Studio 2019 或兼容 MSVC、CMake 3.18+、C++17。
只有完整 Demo 需要 Qt 5.12；STEP/IGES 所需 OCCT 已作为 Demo 私有依赖提供。

```powershell
cmake -S . -B build -A x64 -DQT_ROOT=C:/Qt/5.12.12/msvc2017_64
cmake --build build --config Release
```

只构建 API Samples，不需要 Qt：

```powershell
cmake -S . -B build-samples -A x64 `
  -DHTS_VIEWER_BUILD_DEMO=OFF `
  -DHTS_VIEWER_BUILD_API_SAMPLES=ON
cmake --build build-samples --config Release
```

交付的 SDK 是 Release C++ ABI，公开接口包含 `std::string` 和 `std::vector`，因此本工程
只开放 Release 配置，避免 Debug STL/CRT 混用。

## Sample 操作方式

API Sample 启动时会在控制台打印全部快捷键。保持 Viewer 窗口为当前窗口即可操作，
按 `H` 可随时重新打印帮助。常用操作如下：

| Sample | 常用按键 |
|---|---|
| quickstart | `1/2/3` 对象生命周期，`G/L` 地网格/地板，`I/T/R/F` 视图 |
| interaction | `1/2/3` 选择模式，`Ctrl+左键` 多选，`A/C/D/F` 选择操作 |
| appearance | `1/2/3` 显示模式，`C/T/M/D/R` 外观和 PBR |
| mesh | `1/2/3` Part 显隐，`S/C/M/P` 选择、样式、模式和统计 |

每个目录的 README 提供该 Sample 的完整按键表。

## 安装与打包

安装到一个可直接运行的目录：

```powershell
cmake --install build --config Release --prefix install/HtsViewerSamples
```

安装结果包含 Demo、API Sample、Viewer/授权运行库以及完整离线 Documentation。Qt 和
Demo 私有 OCCT 运行库会随 Demo 一起部署。

生成对外 ZIP 试用包：

```powershell
cpack --config build/CPackConfig.cmake -C Release
```

## SDK 与 CAD Import 边界

SDK 本身不解析 STEP、IGES 或 BREP，也不要求客户使用 OCCT。业务侧负责将 CAD、Mesh、
Solver 或自定义数据转换为 `HtsDisplayMeshData` / `HtsMeshDisplayData`，然后只通过：

```cpp
#include <HtsViewerSdk.h>

hts::viewer::HtsViewerSdk viewer;
```

进行显示和交互。OCCT 只属于 `viewer_demo` 的私有导入 DLL，不进入普通 Sample，也不属于
Viewer SDK 的公开依赖。

## 联系方式

王老师：(+86) 131 1187 9058（微信同号）  
邮箱：cae_manager@163.com  
版权：成都电科智算科技有限公司
