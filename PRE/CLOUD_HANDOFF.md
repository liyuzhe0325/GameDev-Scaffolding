# PRE v0.5.0 云端交接

## 当前基线

- 源码来自本地 Git 标签 v0.5.0，提交 ec9f88a。
- PRE 是独立的像素 2D／2.5D 引擎与编辑器，使用 C++20、SDL3 GPU、Dear ImGui 和 Lua。当前验证过的图形后端为 Windows x64 D3D12。
- 已实现低分辨率渲染目标与最近邻显示、PNG 精灵导入与点选、单层瓦片绘制、场景保存冲突检查和双语界面。版本历史由 releases.json 与 locales/ 驱动。
- v0.5.0 加入 Play / Stop。引擎实例化编辑场景，Lua 脚本可读取键盘和帧时间、修改所属实体位置。Stop 后恢复编辑数据。示例运动规则在 scenes/scripts/ 中。
- 已通过 Windows Release 构建、启动检查、5 项 CTest 和 6 项 Python CLI 测试。图形交互的实际点击与按键需要在 Windows 上验收。

## 关键入口

- src/main.cpp：SDL 主循环、ImGui 和 GPU 提交。
- src/PixelViewport.cpp、shaders/sprite.hlsl：低分辨率精灵和瓦片渲染。
- src/SceneDocument.cpp、src/EditorUI.cpp：场景持久化、编辑面板和 Play / Stop 入口。
- src/SceneRuntime.cpp：运行时场景副本、实体 Lua 生命周期、错误隔离与基础 API。
- tools/scene_cli.py、docs/AI_CONTRACT.md：场景数据验证和自动化编辑约定。
- README.md：构建、Lua API 和验收步骤。

## 构建与验证

在 Windows、VS 2022 C++ 工具链、带 dxc 的 Windows SDK 和 D3D12 GPU 环境中执行：

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
    ctest --test-dir build -C Release --output-on-failure
    python -m unittest discover -s tests -v

首次 CMake 配置需要下载依赖。当前 ImGui 配置跟随 docking 分支，此基线的实测提交见 CHANGELOG.md。云端若缺 Windows／D3D12，可先运行 CLI 测试并检查代码，再把图形编译和验收留给 Windows。

    cd build/Release
    ./pre_editor.exe --scene scenes/runtime_demo.json

点运行，上方精灵自动往返，下方精灵由 WASD 控制。点停止，两者恢复编辑位置。在检查器交换 Lua 脚本引用后再次运行，行为应互换。

## 推进方向

目标是让 PRE 能搭建有前后遮挡、灯光、湿地反射、粒子和载具运动的像素街景。按可验证的渲染切片逐步推进，先用一个小场景验证通用能力，再换资产和配置验证复用。当前图层、序列帧、动态灯光、反射区域和粒子仍待实现。载具运动等内容规则由示例或项目脚本定义，引擎负责通用渲染、数据和运行接口。后续保留旧场景兼容性及已有运行时测试，新增字段时同步检查 CLI 与编辑器。
