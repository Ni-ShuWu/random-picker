<h1 align="center">🎲 Random Picker · 随机抽签工具</h1>

<p align="center">
  <em>从本地表格中随机抽取人员名单，支持 Excel / CSV，自带 GUI 图形界面</em>
</p>

<p align="center">
  <a href="https://github.com/Ni-ShuWu/random-picker/releases"><img src="https://img.shields.io/github/v/release/Ni-ShuWu/random-picker" alt="Release"></a>
  <a href="https://github.com/Ni-ShuWu/random-picker/releases"><img src="https://img.shields.io/github/downloads/Ni-ShuWu/random-picker/total" alt="Downloads"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue" alt="C++17">
  <img src="https://img.shields.io/badge/Windows-10%2F11-brightgreen" alt="Windows">
</p>

---

**给老师/组织者的工具**：从 Excel 或 CSV 名单中随机抽取指定数量的人，并展示附加信息（学号、班级等）。

| GUI 模式 | CLI 模式 |
|---|---|
| 双击即用，所见即所得 | 适合定时/批量任务 |
| 浏览选择文件 → 输入人数 → 一键抽签 | `--file list.xlsx --count 5` |

---

## ⬇️ 下载

<a href="https://github.com/Ni-ShuWu/random-picker/releases/download/v1.0.0/RandomPicker_Windows_v1.0.0.zip">
  📥 下载 Windows 版 (1.6 MB)
</a>
<br>
<a href="https://github.com/Ni-ShuWu/random-picker/archive/refs/tags/v1.0.0.zip">
  📥 下载源码
</a>

解压 `RandomPicker_Windows_v1.0.0.zip` → 双击 `启动.bat` 即可。

---

## ✨ 功能

- **格式支持** — CSV / XLSX / TSV / TXT，自动识别
- **随机抽取** — `std::mt19937_64` 高随机性，不放回抽样
- **GUI 图形界面** — Win32 原生，无需安装任何框架
- **CLI 命令行** — 适合自动化脚本、定时任务
- **AI 增强** — 可选调用 DeepSeek / OpenAI 生成推荐语
- **结果导出** — 文本 / CSV / 剪贴板，UTF-8 编码
- **种子复现** — `--seed 42` 固定种子，抽签结果可复现

---

## 🖥️ 图形界面

| 步骤 | 说明 |
|------|------|
| ① 选文件 | 点击「浏览…」选择 `.csv` 或 `.xlsx` |
| ② 填人数 | 输入要抽取的数量 |
| ③ 可选：AI | 勾选「启用 AI」，填入 DeepSeek/OpenAI API Key |
| ④ 开始 | 点击「🎲 开始抽签」 |
| ⑤ 导出 | 保存为 txt/csv，或复制到剪贴板 |

---

## ⌨️ 命令行

```shell
# 基本用法
random_picker.exe --file students.xlsx --count 3

# 保存结果
random_picker.exe --file list.csv --count 5 --output result.txt

# 指定随机种子（复现结果）
random_picker.exe --file list.csv --count 3 --seed 42

# 启用 AI 推荐（DeepSeek 默认）
random_picker.exe --file list.csv --count 3 --with-ai --api-key sk-xxx

# 自定义 AI 接口（如换其他 API）
random_picker.exe --file list.csv --count 3 --with-ai --api-key sk-xxx --api-baseurl https://api.openai.com
```

完整参数：`random_picker.exe --help`

---

## 📋 表格格式

第一行必须是表头，至少包含 `姓名` 列（也识别 `name`、`xingming`、`名字`、`student`、`学生`）。

```csv
姓名,学号,班级,性别,备注
张三,2024001,计算机一班,男,班委
李四,2024002,计算机一班,男,
王五,2024003,计算机一班,女,
```

---

## 🔧 从源码构建

**依赖**：CMake 3.16+、GCC 14+ / MSVC 2022、libzip

```bash
# MSYS2 UCRT64 终端
pacman -S --needed mingw-w64-ucrt-x86_64-{gcc,cmake,ninja,libzip,zlib}
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# 成品: build/random_picker.exe
```

Windows 一键构建：双击 `build.bat`

---

## 🧱 技术栈

| 模块 | 实现 |
|------|------|
| 随机数 | `std::random_device` + `std::mt19937_64` |
| CSV 解析 | 手写（支持引号、转义、BOM） |
| XLSX 解析 | libzip 解压 + XML 解析（zip/xml） |
| HTTP | WinHTTP（系统原生） |
| GUI | Win32 API（Common Controls 6） |
| AI | OpenAI / DeepSeek Chat Completions 协议 |
| 日志 | 内置（级别控制、文件输出） |

---

## 📄 License

MIT
