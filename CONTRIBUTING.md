# 贡献指南

感谢你帮助改进 SCU-Nexus。项目接受缺陷修复、测试、文档、可访问性与新功能贡献。

## 开始之前

1. 搜索现有 Issue，避免重复工作；较大的功能请先创建讨论 Issue。
2. 从 `main` 创建短生命周期分支，例如 `feat/schedule-export` 或 `fix/login-timeout`。
3. 不要提交真实账号、Cookie、成绩、教务响应或其他个人信息。测试数据必须匿名化。
4. 保持改动聚焦；不要在功能 PR 中混入无关格式化或大范围重构。

## 开发与验证

项目要求 C++20、Qt 6.8+、CMake 3.24+。构建方式见 [README](README.md#从源码构建)。提交前请运行：

```powershell
cmake --build out/build/dev
ctest --test-dir out/build/dev --output-on-failure
```

修改业务逻辑时补充 Qt Test；修改 QML 时至少覆盖关键对象名、信号或交互契约。网络测试不得依赖真实教务服务，应使用夹具或测试替身。

## 提交与 Pull Request

建议使用 Conventional Commits 风格：

- `feat: add schedule export`
- `fix: handle expired authentication session`
- `docs: clarify Windows packaging`
- `test: cover empty exam response`

PR 描述应说明问题、实现方式、验证结果和界面变化；涉及 UI 时附上截图。维护者可能要求拆分改动或补充测试。

提交贡献即表示你有权提交相关内容，并同意其按仓库的 MIT License 发布。
