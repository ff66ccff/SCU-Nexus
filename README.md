# SCU-Nexus

面向四川大学学生的一站式桌面校园信息助手。SCU-Nexus 使用 Qt 6、C++20 与 QML 构建，将课表、校历、考试安排和成绩查询集中在统一的 Windows 客户端中。

> [!IMPORTANT]
> SCU-Nexus 是社区维护的非官方项目，与四川大学及其信息化部门无隶属或授权关系。教务系统接口变化可能导致部分功能暂时不可用，请勿将本项目用于任何违反学校规定或法律法规的用途。

## 功能

- 统一身份认证与教务系统会话管理
- 本地课表管理、教务课表导入、单双周及时间段设置
- 校历、考试安排和成绩查询
- 成绩统计、筛选与多种分数展示方式
- 本地缓存、主题设置与现代化 Qt Quick 界面

项目仍处于早期开发阶段。发布包、已知问题和版本变化以 [Releases](https://github.com/ff66ccff/SCU-Nexus/releases) 与 [CHANGELOG](CHANGELOG.md) 为准。

## 下载与安装

Windows 用户可从 [Releases](https://github.com/ff66ccff/SCU-Nexus/releases) 下载：

- `SCU-Nexus-<version>-windows-x86_64.exe`：推荐的安装程序；
- `SCU-Nexus-<version>-windows-x86_64.zip`：免安装便携包。

安装程序与 ZIP 均包含 Qt、MinGW 和运行所需的动态库，不要求用户预装 Qt。当前发布包未进行商业代码签名，Windows SmartScreen 可能在首次运行时显示提示；请核对 Release 中的 SHA-256 文件后再运行。

## 从源码构建

### 环境要求

- Windows 10/11 x64
- Qt 6.8 或更高版本（MinGW 64-bit，包含 Qt Quick Controls 2）
- CMake 3.24 或更高版本
- Ninja
- 与 Qt 工具链兼容的 OpenSSL 1.1.1 或 3.x Crypto 开发库

推荐使用 Qt Online Installer 安装 Qt、CMake、Ninja 和 MinGW。项目不会再依赖写死的本机路径。

在 Qt Creator 中打开 `SCU_Nexus/CMakeLists.txt` 即可。命令行构建示例：

```powershell
cmake -S SCU_Nexus -B out/build/dev -G Ninja `
  -DCMAKE_BUILD_TYPE=Debug `
  -DQt6_ROOT=C:/Qt/6.11.1/mingw_64 `
  -DOPENSSL_ROOT_DIR=C:/Qt/Tools/mingw1310_64/opt
cmake --build out/build/dev
ctest --test-dir out/build/dev --output-on-failure
```

也可以进入 `SCU_Nexus` 目录后使用 `CMakePresets.json`。若 Qt 不在 `PATH` 中，请先设置 `CMAKE_PREFIX_PATH` 或在 Qt Creator 中选择对应 Kit。

## 构建 Windows 发布包

仓库提供了可复现的 PowerShell 打包脚本：

```powershell
./scripts/build-windows.ps1
```

脚本会自动查找常见位置中的 Qt、MinGW、CMake、Ninja 和 OpenSSL，执行 Release 构建与全部测试，再把产物写入 `out/packages/`。默认总会生成便携 ZIP；安装 [NSIS](https://nsis.sourceforge.io/) 后会同时生成 `Setup.exe`：

```powershell
winget install --id NSIS.NSIS --exact
./scripts/build-windows.ps1
```

自定义工具位置时可显式传参：

```powershell
./scripts/build-windows.ps1 `
  -QtDir D:/Qt/6.11.1/mingw_64 `
  -OpenSSLRoot D:/Qt/Tools/mingw1310_64/opt
```

推送形如 `v0.1.0` 的标签后，GitHub Actions 会构建、测试并将安装程序、便携包和校验文件发布到对应 Release。

## 项目结构

```text
SCU_Nexus/
├── qml/                 # 页面、组件与主题
├── src/
│   ├── app/             # 应用生命周期、路由与设置
│   ├── core/            # 网络、加密与日志基础设施
│   ├── models/          # 领域模型
│   ├── repositories/    # SQLite 持久化与缓存
│   ├── services/        # 教务、认证、校历、课表与成绩服务
│   └── viewmodels/      # QML 视图模型
└── tests/               # Qt Test 自动化测试及测试夹具
```

## 隐私与安全

登录凭据仅用于建立学校系统会话，项目不提供远程遥测或自建中转服务。缓存、课表和设置保存在当前用户的应用数据目录中。请勿在 Issue、日志或截图中提交学号、密码、Cookie、成绩等敏感信息。

发现安全问题时请遵循 [SECURITY.md](SECURITY.md)，不要创建公开 Issue。

## 参与贡献

欢迎提交 Issue 和 Pull Request。开始前请阅读 [贡献指南](CONTRIBUTING.md) 与 [行为准则](CODE_OF_CONDUCT.md)。提交应包含必要测试，并确保：

```powershell
ctest --test-dir out/build/dev --output-on-failure
```

全部通过。

## 许可证

本项目以 [MIT License](LICENSE) 开源。项目名称、学校名称及第三方服务的商标权归各自权利人所有。
