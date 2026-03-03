# ContextMenuProfiler 开发与演进计划

## 1. 当前架构 (Current Architecture)
本项目已完成从“模拟进程测速”到“Explorer 注入 Hook”的重大架构演进。

### 核心设计：
- **Hook 探测 (In-Process Hooking)**：
  - 通过 `ContextMenuProfiler.Injector` 将 `ContextMenuProfiler.Hook.dll` 注入到 `explorer.exe`。
  - 接管 `CoCreateInstance`，实现对插件创建过程的深度探测。
  - 在 Hook 环境中手动模拟插件生命周期（`Initialize` / `QueryContextMenu`）。
- **通信协议 (IPC)**：
  - UI 与 Hook DLL 之间通过 **Named Pipes** 使用 JSON 格式进行实时双向通信。
- **数据指标**：
  - 测速精度提升至毫秒级，支持 `create_ms`、`init_ms`、`query_ms` 分级展示。
- **图标采集**：
  - 具备从注册表、DLL 资源、OwnerDraw 绘制结果中提取高清图标的能力。

## 2. 待完成与改进点 (Roadmap)

### A. 架构加固与代码质量
- [x] **路径硬编码消除**：将 C++ 端 Hook DLL 的日志、图标输出路径由硬编码改为动态解析，避免在开发机以外的环境失效。
- [ ] **多线程 IPC 优化**：当前 C++ 端的管道服务是阻塞模式，在大并发请求下（虽然目前不多）可能会有响应瓶颈，计划引入异步管道。
- [ ] **Hook 异常处理**：在 C++ 端进一步增强异常处理（`__try...__except`），防止某个坏插件的测速操作波及 Explorer。

### B. 功能增强
- [ ] **实时监控模式**：不再局限于手动扫描，而是可以监听 Explorer 真实的菜单弹出事件，并自动记录对应的插件性能。
- [ ] **环境检测自愈**：优化 `redeploy.bat`，自动查找 VS 环境和 MSBuild，提高部署成功率。
- [ ] **孤儿项筛选/清理**：针对扫描出的“DLL 不存在”的插件，提供一键筛选并清理注册表无效路径的功能。

### C. UI/UX 体验
- [ ] **可视化报表**：目前的列表显示很直接，未来可以考虑增加耗时占比的环形图或柱状图。
- [ ] **多语言支持**：支持国际化。

### D. 已知限制 (Current Limitations)
- **分析文件 (Analyze File) 功能基本是坏的**：
  - 目前的“分析文件”功能在底层实际上执行的是全系统扫描 (`RunSystemBenchmark`)。
  - 它**不会**根据拖入文件的扩展名（如 `.txt`）来分析上下文菜单项。因此，扫描结果中仍会出现针对文件夹或所有文件的通用菜单项（例如 Bandizip 的“解压到当前文件夹”），即使这些选项并不适用于当前分析的文件。
- **缺乏增量更新 (No Incremental Updates)**：
  - UI 列表目前不支持流式更新。扫描过程中界面不会逐个显示已发现的扩展，而是需要等待整个扫描任务（包括所有 COM 和 UWP 扩展的探测）全部完成后，一次性刷新显示所有结果。这在扩展数量较多的系统中可能会导致短暂的等待感。

## 3. 已完成清理任务
- [x] 移除所有 `ContextMenuProfiler.Surrogate.*` 旧项目。
- [x] 移除 `ShellBenchmark` 命令行工具。
- [x] 清理文档目录，移除过时的研究笔记和技术草案。
- [x] 迁移至基于 JSON 的健壮 IPC 协议。
