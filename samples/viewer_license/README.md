# viewer_license

演示 Hts3d Viewer SDK 的授权流程：导出机器码、从 SDK 根目录 `license/` 搜索并导入
`.lic` 文件，以及查询授权状态。

首次运行后，将生成的机器码发送给授权提供方。取得 `.lic` 文件后放入 Sample 的
`license/` 目录并再次运行。授权成功时程序输出：

```text
Viewer license status: AUTHORIZED
```

授权成功后，其他 Sample 在调用
`hts::viewer::HtsViewerSdk::initializeStandalone()` 或
`initializeEmbedded()` 时会完成授权检查，不需要直接接触授权库接口。

工具还会输出授权类型（`TRIAL` 或 `FORMAL`）和本地时区的授权到期时间。试用授权
在到期前可以运行全部公开 Sample；正式授权则显示为 `FORMAL`。
