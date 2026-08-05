# 修复 CI 构建失败：lucihttp 编译找不到 lua.h

## 问题描述

在 GitHub Actions 中构建 `luci-app-oaf` 时，编译依赖包 `lucihttp` 失败，错误信息：

```
fatal error: lua.h: No such file or directory
   22 | #include <lua.h>
```

## 根因分析

### 依赖链

```
luci-app-oaf  (LUCI_DEPENDS: +appfilter +kmod-oaf +luci-compat)
  └── luci-compat
        └── lucihttp  ← 编译时需要 lua.h
              └── lua  ← 提供 lua.h 头文件 (来自 packages feed)
```

### 直接原因

工作流中 [`.github/workflows/build-with-openwrt-sdk.yml`](.github/workflows/build-with-openwrt-sdk.yml) 的 `Compile OAF packages` 步骤直接执行 `make package/luci-app-oaf/compile`，但 `lucihttp` 编译时需要 `lua.h` 头文件。

`lucihttp` 是 `luci` feed 的包，`lua` 是 `packages` feed 的包。跨 feed 的构建依赖未被自动解析，导致 `lua` 在 `lucihttp` 编译之前未构建，`lua.h` 未安装到 staging 目录。

### 第一次修复尝试失败原因

仅添加 `CONFIG_PACKAGE_lua=y` 和 `CONFIG_PACKAGE_luci-compat=y` 到 `.config` 不足以解决问题，因为 OpenWrt 的 `make package/<name>/compile` 目标在构建依赖链时，`lucihttp` 的构建脚本可能先于 `lua` 的 staging 安装完成之前执行。

## 修复方案（最终版）

### 双重保障：显式预编译 lua + 在 .config 中选中

**修改一**：在 `.config` 中选中 `lua` 和 `luci-compat` 包（确保它们被纳入构建计划）

**修改二**：在编译步骤中，**显式先编译 `lua` 包**，确保 `lua.h` 头文件已安装到 staging 目录，然后再编译 `luci-app-oaf`

## 修改内容

### 修改文件

[`.github/workflows/build-with-openwrt-sdk.yml`](.github/workflows/build-with-openwrt-sdk.yml)

### 变更 1：Configure .config 步骤（L169-174）

```diff
  echo "CONFIG_PACKAGE_luci-app-oaf=m" >> .config
  echo "CONFIG_PACKAGE_appfilter=m" >> .config
  echo "CONFIG_PACKAGE_kmod-oaf=m" >> .config
+ echo "CONFIG_PACKAGE_luci-compat=y" >> .config
+ echo "CONFIG_PACKAGE_lua=y" >> .config
  echo "CONFIG_LUCI_LANG_zh_Hans=y" >> .config
```

### 变更 2：Compile OAF packages 步骤（L185-190）

```diff
+ # Build lua first to ensure lua.h is available in the staging directory
+ # (lucihttp, a dependency of luci-app-oaf, requires lua.h at compile time)
+ echo "========== compile lua (dependency for luci-app-oaf) =========="
+ make package/lua/compile -j$(nproc) V=s || make package/lua/compile -j1 V=s
  echo "========== compile luci-app-oaf =========="
  make package/luci-app-oaf/compile -j$(nproc) V=s || make package/luci-app-oaf/compile -j1 V=s
```

## 验证方法

1. 在 GitHub Actions 页面手动触发工作流
2. 选择 `aarch64_generic` + `mediatek/filogic`（或任意之前失败的组合）
3. 观察日志中 `compile lua` 步骤是否成功
4. 观察 `compile luci-app-oaf` 步骤是否通过
5. 确认产出物包含三个 `.ipk` 文件

## 回滚方案

如果修复后仍然失败，可以从编译步骤中删除 `make package/lua/compile` 行，恢复到仅依赖 `.config` 中的配置。