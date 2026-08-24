# viewer_appearance

集中演示显示模式、CAD 边、三角线框、颜色、透明度和 PBR 材质。所有修改均通过
`HtsViewerSdk` 的持久外观接口完成，不访问 DisplayManager 或 OSG 状态。

| 按键 | 操作 |
|---|---|
| `1/2/3` | 工程默认、着色、着色加 CAD 边 |
| `E/W` | CAD 边、三角线框开关 |
| `C/T` | 蓝色外观、55% 透明 |
| `M/D` | 抛光金属、粗糙介质 PBR |
| `R/B` | 重置覆盖、切换背景 |
| `S/H` | 输出统计、重新打印帮助 |

`metallic`、`roughness` 和 `specular` 的范围均为 `[0, 1]`；透明度 `0` 表示不透明，
`1` 表示完全透明。
