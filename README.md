

## 介绍

OAF 是一款基于 OpenWrt 的家长控制软件。它支持游戏、视频流媒体、即时通讯等热门应用，例如 TikTok、YouTube、Facebook。目前支持数百种不同的应用程序。

详细介绍请访问 [www.openappfilter.com](http://www.openappfilter.com)。

> [!WARNING]
> >这是自用修改版，不一定合你意
> >
> >**不支持** ip sni 拦截，比如 `||1.1.1.1^`、`/1\.1\.1\.1/`，用户依旧可以访问 http(s)://1.1.1.1/*，如有需求，请自行让 AI 实现或者依靠软路由防火墙屏蔽
> >
> >上游 Makefile 语法有点问题，直接使用上游代码+较新的 OpenWrt SDK 编译，编译 open-app-filter 时会报错 `libubox/uloop.h` 没找到，编译 `luci-app-oaf` 时，依赖 `lucihttp` 报错 `lua.h` 没找到，本仓库适配的是openwrt 25.12，openwrt 24*的sdk不一定能用，至于工作流里面的 openwrt 24相关代码，纯属屎山代码

## 功能特性

- 基于 DPI 的协议识别：支持七层协议解析和 HTTPS 域名解析，不依赖 DNS 运行。
- 行业标准架构：基于流的高效识别，硬件要求极低。
- 支持自定义协议特征：提供高度灵活性和可定制性。
- 支持作为插件安装到 OpenWrt 系统：兼容所有支持 OpenWrt 的设备。您可以从 releases 页面下载对应架构的插件包。
- ============================一点也不好看的分割线===============================
- 新增对部分 AdGuard Home 规则语法的支持，提高了使用灵活性。

>> `||example.org^`：阻止 example.org 域名及其所有子域名；
>
>> `@@||sub.example.org^`：放行 sub.example.org 及其所有多级子域名，通常配合上面的阻止规则一起使用；
>
>> `! 这是一行注释。`：仅是一条注释；
>
>> `# 这也是一行注释。`：仅是一条注释；
>
>> `/REGEX/`：阻止访问与指定正则表达式匹配的域名。
>
>> ~~`127.0.0.1 example.org`：对 example.org（不包括其子域名）以 127.0.0.1 作为响应（实现失败）；~~

## 编译方法（如果懒得在电脑上编译，github 工作流文件会帮你）

1. 准备一套已成功编译为固件的 OpenWrt 源码。
   （OpenWrt 源码的编译方法请参考其他独立教程，此处不再赘述。）

2. 克隆 OAF 源码。
   进入 OpenWrt 源码根目录，执行以下命令：

```
git clone https://github.com/Fido6/OpenAppFilter.git package/OpenAppFilter
```

3. 启用 OAF 编译选项。
   应用过滤功能包含三个独立的源码包，分别对应 LuCI 应用、服务守护进程和内核模块。
   编译前，必须启用这三个软件包的编译选项。您可以通过 `make menuconfig` 图形界面选择 `luci-app-oaf`。
   或者，执行以下命令启用（在源码根目录下运行）：

```
echo "CONFIG_PACKAGE_luci-app-oaf=y" >>.config
make defconfig
```

   这将自动启用所有三个模块的编译选项。

4. 开始编译 OAF。
   如果您之前已成功编译过 OpenWrt 源码，可以选择仅编译单个软件包：

```
make package/oaf/compile -j$(nproc) V=s
make package/open-app-filter/compile -j$(nproc) V=s
make package/luci-app-oaf/compile -j$(nproc) V=s
```

   或者，重新编译整个固件镜像，这将把插件直接集成到固件中：

```
make V=s
```

## 许可证

- 个人可以完全免费使用本软件，也允许在此基础上进行二次开发和再分发。
- 如果您基于 OAF 进行衍生开发，必须遵守 GPL 2.0 许可证，并保留对 OAF 仓库或网站信息的引用。
- 如果公司希望使用本软件，请联系作者获取授权。
