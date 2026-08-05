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
              └── lua  ← 提供 lua.h 头文件
```

### 直接原因

工作流中 [`.github/workflows/build-with-openwrt-sdk.yml`](.github/workflows/build-with-openwrt-sdk.yml) 的 `.config` 只配置了三个包：

```yaml
echo "CONFIG_PACKAGE_luci-app-oaf=m" >> .config
echo "CONFIG_PACKAGE_appfilter=m" >> .config
echo "CONFIG_PACKAGE_kmod-oaf=m" >> .config
```

`lua` 包没有被选中，导致其头文件 `lua.h` 未安装到 SDK 的 staging 目录中。当 `lucihttp` 编译时找不到 `lua.h`，编译终止。

## 修复方案（已实施）

### 采用方案二：同时选中 luci-compat 和 lua

在 [`build-with-openwrt-sdk.yml`](.github/workflows/build-with-openwrt-sdk.yml) 的 `Configure .config` 步骤中，添加两行：

```yaml
echo "CONFIG_PACKAGE_luci-compat=y" >> .config
echo "CONFIG_PACKAGE_lua=y" >> .config
```

**原理**：
- `CONFIG_PACKAGE_lua=y` 确保 `lua` 包被构建，其头文件 `lua.h` 安装到 staging 目录，`lucihttp` 编译时能找到
- `CONFIG_PACKAGE_luci-compat=y` 确保整个 `luci-compat` 依赖链被完整拉取，避免其他潜在依赖缺失

## 修改内容

### 修改文件

[`.github/workflows/build-with-openwrt-sdk.yml`](.github/workflows/build-with-openwrt-sdk.yml)，第 169-173 行 `Configure .config` 步骤。

### 实际变更

```diff
  - name: Configure .config
    run: |
      cd sdk
      echo "CONFIG_ALL_NONSHARED=n" > .config
      echo "CONFIG_ALL_KMODS=n" >> .config
      echo "CONFIG_ALL=n" >> .config
      echo "CONFIG_AUTOREMOVE=n" >> .config
      echo "CONFIG_SIGNED_PACKAGES=n" >> .config
      echo "CONFIG_PACKAGE_luci-app-oaf=m" >> .config
      echo "CONFIG_PACKAGE_appfilter=m" >> .config
      echo "CONFIG_PACKAGE_kmod-oaf=m" >> .config
+     echo "CONFIG_PACKAGE_luci-compat=y" >> .config
+     echo "CONFIG_PACKAGE_lua=y" >> .config
      echo "CONFIG_LUCI_LANG_zh_Hans=y" >> .config
      make defconfig
```

## 验证方法

1. 触发 CI 工作流，选择任意版本/平台（如 `snapshots` + `x86/64`）
2. 观察 `luci-app-oaf` 编译阶段是否成功通过
3. 确认产出物包含三个 `.ipk` 文件：
   - `luci-app-oaf_*.ipk`
   - `appfilter_*.ipk`
   - `kmod-oaf_*.ipk`

## 回滚方案

如果修复后仍然失败，可以：
1. 移除 `.config` 中的 `CONFIG_PACKAGE_luci-compat=y` 和 `CONFIG_PACKAGE_lua=y` 两行
2. 或在编译步骤中先单独编译 `lua` 包：`make package/lua/compile -j$(nproc) V=s`