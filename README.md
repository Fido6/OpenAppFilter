## 介绍

OAF 是一款基于 OpenWrt 的家长控制和应用过滤软件。它通过 DPI 识别游戏、视频、即时通讯等应用流量，例如 TikTok、YouTube、Facebook 等，目前支持数百种应用。

详细介绍可访问 [www.openappfilter.com](http://www.openappfilter.com)。

## 功能特性

- 基于 DPI 的协议识别：支持七层协议解析和 HTTPS 域名识别，不依赖 DNS。
- 基于流的识别架构：效率较高，对硬件要求较低。
- 支持自定义协议特征：可按现有特征库格式扩展应用识别规则。
- 支持作为 OpenWrt 插件安装：包含 LuCI 页面、用户态服务和内核模块。
- 支持自定义规则：可在 LuCI 页面中维护部分 AdGuard Home 风格的域名过滤规则。

## 本分支改动

本分支在 OAF 基础上新增了自定义规则能力，入口位于 LuCI：

```text
服务 / App Filter / Custom Rules
```

自定义规则保存后会写入：

```text
/etc/appfilter/custom_rules.txt
```

保存页面会触发 `oafd` 重新加载规则，无需更新应用特征库。

### 支持的自定义规则语法

```text
||example.org^
@@||sub.example.org^
! comment
# comment
/REGEX/
```

说明：

- `||example.org^`：阻止 `example.org` 及其所有子域名。
- `@@||sub.example.org^`：放行 `sub.example.org` 及其所有子域名，放行规则优先级高于阻止规则。
- `! comment` 和 `# comment`：注释行。
- `/REGEX/`：使用正则表达式匹配域名，例如 `/allaw(os|tech)\.[cn]/`。
- `||...^` 和 `@@||...^` 的域名部分支持 `*`，例如 `||aa*.example.com^`。

不支持将指定地址重定向到给定 IP 的规则，例如 hosts/DNS 类规则；这类能力应由 DNS 组件处理。

### 自定义规则开关

自定义规则页面包含：

- `启用自定义规则`：关闭后所有自定义规则不生效。
- `时间段执行`：开启后，自定义规则只在 OAF 时间规则生效时执行；关闭时，即使当前不在应用过滤时间段内，自定义规则也可以单独保持过滤器运行。
- 规则输入框：每行一条规则。

<!-- 当仅自定义规则处于生效状态时，运行状态页面会显示：

```text
（仅自定义规则状态）
``` -->

### 规则数量上限

自定义规则页面会按设备可用空间动态计算规则上限：

```text
floor(磁盘剩余可用字节数 / (80 * 1000))
```

计算使用十进制单位，即：

```text
1 MB = 1000 KB
```

计算结果会写入 UCI 配置 `appfilter.global.custom_rule_max_num`，后端加载规则时按该值限制有效规则数量。

## 编译方法

1. 准备一套已经成功编译过固件的 OpenWrt 源码。

2. 将本仓库放入 OpenWrt 源码的 `package` 目录，例如：

```sh
git clone <this-repository-url> package/OpenAppFilter
```

3. 启用编译选项。

应用过滤包含三个包：LuCI 应用、用户态服务和内核模块。可以在 `make menuconfig` 中选择 `luci-app-oaf`，也可以执行：

```sh
echo "CONFIG_PACKAGE_luci-app-oaf=y" >> .config
make defconfig
```

4. 编译单个包：

```sh
make package/luci-app-oaf/compile V=s
make package/open-app-filter/compile V=s
make package/oaf/compile V=s
```

也可以重新编译完整固件：

```sh
make V=s
```

## 许可证

- 个人可以免费使用，也允许在此基础上进行二次开发和再分发。
- 如果基于 OAF 进行衍生开发，需要遵守 GPL 2.0 许可证，并保留 OAF 仓库或网站信息。
- 公司使用请联系上游原作者获取授权。
