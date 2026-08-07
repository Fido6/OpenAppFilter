# OpenAppFilter (OAF) 项目深度解读

## 一、项目概述

**OpenAppFilter (OAF)** 是一款基于 **OpenWrt 平台的家长控制软件**，通过深度包检测（DPI）技术识别网络流量中的应用程序，实现对游戏、视频、社交媒体等热门应用的精细化管控。当前版本：**v6.1.8**。

---

## 二、项目定位与功能

### 核心功能
| 功能 | 说明 |
|------|------|
| **DPI 协议识别** | 七层协议解析 + HTTPS 域名解析，不依赖 DNS |
| **应用识别库** | 内置数百种应用特征，覆盖游戏、社交、视频、购物等分类 |
| **自定义规则** | 支持自定义协议特征 + AdGuard Home 规则语法 |
| **时间管控** | 手动模式 / 动态模式 / 每日限额模式 |
| **设备管理** | 基于 MAC 地址的设备识别与管控 |
| **访问记录** | 完整的设备访问历史与统计报表 |
| **LuCI 界面** | 可视化 Web 管理界面，含 ECharts 图表 |

### 支持的应用类型
通过分析 [`feature.cfg`](open-app-filter/files/feature.cfg)，可以看到应用按类别划分：
- **1xxx** - 聊天类（微信、QQ、微博、钉钉、Facebook、WhatsApp、Instagram 等）
- **2xxx** - 游戏类（王者荣耀、原神、英雄联盟、梦幻西游等）
- **3xxx** - 视频类（TikTok、YouTube、抖音、爱奇艺、B站等）
- **4xxx** - 购物类（淘宝、京东、拼多多等）
- **5xxx** - 音乐类（QQ音乐、网易云音乐等）
- **6xxx** - 办公类（WPS、腾讯会议等）
- **7xxx/8xxx** - 社交/工具类

---

## 三、系统架构

```
┌────────────────────────────────────────────────────────────────┐
│                    LuCI Web 界面 (luci-app-oaf)                  │
│    JavaScript + ECharts 图表 + 应用图标库                        │
│         ubus (RPC 调用)                                         │
├────────────────────────────────────────────────────────────────┤
│              用户空间守护进程 (open-app-filter)                   │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  oafd 主进程  │  │  Ubus 服务   │  │  UCI 配置管理        │  │
│  │  main.c      │  │  ubus.c     │  │  config.c            │  │
│  │  事件循环     │  │  API 暴露    │  │  配置文件读写        │  │
│  └──────┬───────┘  └──────────────┘  └──────────────────────┘  │
│         │                                                       │
│  ┌──────┴───────┐  ┌──────────────┐  ┌──────────────────────┐  │
│  │  Netlink 通信 │  │  设备管理    │  │  自定义规则引擎      │  │
│  │  netlink.c   │  │  user.c     │  │  custom_rule.c       │  │
│  │  与内核通信   │  │  MAC/IP管理  │  │  AdGuard 语法解析    │  │
│  └──────┬───────┘  └──────────────┘  └──────────────────────┘  │
│         │                                                       │
│         │            Netlink Socket (协议族 29)                  │
├─────────┼──────────────────────────────────────────────────────┤
│  ┌──────┴──────────────────────────────────────────────────┐   │
│  │            内核模块 (oaf/oaf.ko)                          │   │
│  │  ┌─────────────────┐  ┌──────────────────────────────┐  │   │
│  │  │  Netfilter 钩子  │  │  应用识别引擎                 │  │   │
│  │  │  app_filter.c   │  │  feature 匹配 + DPI 解析     │  │   │
│  │  │  HOOK:          │  │  ├─ HTTP 协议解析             │  │   │
│  │  │  NF_INET_PRE_ROUTING│  ├─ HTTPS/TLS SNI 解析      │  │   │
│  │  │  NF_INET_LOCAL_OUT│  │  ├─ 特征位匹配              │  │   │
│  │  └────────┬────────┘  │  │  └─ 正则表达式匹配          │  │   │
│  │           │           │  └──────────────────────────────┘  │   │
│  │  ┌────────┴────────┐  ┌──────────────┐  ┌──────────────┐  │   │
│  │  │ 连接跟踪模块    │  │ 规则配置      │  │ 白名单管理   │  │   │
│  │  │ conntrack.c    │  │ rule_config.c│  │ whitelist.c  │  │   │
│  │  └─────────────────┘  └──────────────┘  └──────────────┘  │   │
│  └────────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│   /proc/sys/oaf/  (控制接口)                                     │
│   /dev/appfilter  (规则下发)                                     │
└────────────────────────────────────────────────────────────────┘
         │
         ▼
  局域网设备流量 (LAN) ──→ 路由器 ──→ WAN (互联网)
```

---

## 四、核心模块详解

### 4.1 内核模块 [`oaf/`](oaf/)

| 文件 | 职责 |
|------|------|
| [`app_filter.c`](oaf/src/app_filter.c) (1960行) | 核心引擎：Netfilter 钩子注册、数据包拦截、DPI 解析、应用识别、RST 阻断 |
| [`app_filter.h`](oaf/src/app_filter.h) | 数据结构定义：`flow_info_t` 流信息、`af_feature_node_t` 特征节点、协议解析结构 |
| [`af_conntrack.c`](oaf/src/af_conntrack.c) | 连接跟踪管理，与 Linux conntrack 子系统交互 |
| [`af_config.c`](oaf/src/af_config.c) | 内核侧配置管理，通过 `/proc/sys/oaf/` 暴露接口 |
| [`af_rule_config.c`](oaf/src/af_rule_config.c) | 规则配置管理，JSON 格式规则解析与下发 |
| [`af_client.c`](oaf/src/af_client.c) | 用户态客户端通信模块 |
| [`af_client_fs.c`](oaf/src/af_client_fs.c) | 文件系统接口客户端 |
| [`af_user_config.c`](oaf/src/af_user_config.c) | 用户配置管理 |
| [`af_whitelist_config.c`](oaf/src/af_whitelist_config.c) | MAC 白名单过滤 |
| [`af_utils.c`](oaf/src/af_utils.c) | 工具函数（MAC/IP 处理等） |
| [`af_log.c`](oaf/src/af_log.c) | 内核日志模块 |
| [`regexp.c`](oaf/src/regexp.c) | 正则表达式匹配引擎 |
| [`cJSON.c`](oaf/src/cJSON.c) | JSON 解析库 |

**关键机制：**
- 通过 Netfilter 在 `NF_INET_PRE_ROUTING` 和 `NF_INET_LOCAL_OUT` 钩子点拦截数据包
- 对前 64 个数据包进行 DPI 分析（`MAX_DPI_PKT_NUM`）
- 支持 HTTP/HTTPS/TCP/UDP 等多协议应用识别
- 识别结果通过 `nf_conntrack_acct` 的 `mark` 字段标记
- 通过 Netlink socket（协议号 29）与用户态通信

### 4.2 用户空间守护进程 [`open-app-filter/`](open-app-filter/)

| 文件 | 职责 |
|------|------|
| [`main.c`](open-app-filter/src/main.c) | 主程序入口：uloop 事件循环、配置加载、定时任务、时间管控逻辑 |
| [`appfilter_ubus.c`](open-app-filter/src/appfilter_ubus.c) (2486行) | ubus RPC 服务：设备列表、访问记录、统计报表、配置下发 |
| [`appfilter_netlink.c`](open-app-filter/src/appfilter_netlink.c) | Netlink 通信：特征库加载、内核消息收发 |
| [`appfilter_user.c`](open-app-filter/src/appfilter_user.c) | 设备管理：哈希表管理、访问记录、在线状态、活跃时间统计 |
| [`appfilter_config.c`](open-app-filter/src/appfilter_config.c) | UCI 配置读写封装 |
| [`appfilter_custom_rule.c`](open-app-filter/src/appfilter_custom_rule.c) | 自定义规则引擎（AdGuard Home 语法） |

**时间管控三种模式：**
- **模式 0 (手动)**：指定时间段允许/禁止上网
- **模式 1 (动态)**：允许/拒绝时间轮换（如允许20分钟，阻断60分钟）
- **模式 2 (每日限额)**：按星期设置每天的上/下午可用时长

### 4.3 LuCI 前端 [`luci-app-oaf/`](luci-app-oaf/)

- 基于 OpenWrt LuCI JavaScript 框架
- 使用 ECharts 绘制统计图表
- 包含 200+ 应用图标（PNG 格式，按应用 ID 命名）
- 通过 ubus 调用后端 API
- 支持中文界面（`po/zh_Hans/oaf.po`）

### 4.4 配置文件

| 文件 | 用途 |
|------|------|
| [`appfilter.config`](open-app-filter/files/appfilter.config) | 主配置：全局开关、工作模式、时间管控、每日限额 |
| [`user_info.config`](open-app-filter/files/user_info.config) | 用户信息配置 |
| [`feature.cfg`](open-app-filter/files/feature.cfg) | 应用特征库（核心 DPI 规则） |
| [`feature_cn.cfg`](open-app-filter/files/feature_cn.cfg) | 中文版特征库 |
| [`feature_en.cfg`](open-app-filter/files/feature_en.cfg) | 英文版特征库 |
| [`oaf_rule`](open-app-filter/files/oaf_rule) | 规则下发脚本（JSON 格式写入 `/dev/appfilter`） |
| [`gen_class.sh`](open-app-filter/files/gen_class.sh) | 特征库分类生成脚本 |
| [`hnat.sh`](open-app-filter/files/hnat.sh) | 硬件 NAT 加速控制脚本 |

---

## 五、通信机制

### 5.1 内核 ↔ 用户态：Netlink
- 自定义 Netlink 协议，协议号 **29**
- 消息类型：`AF_MSG_INIT`、`AF_MSG_ADD_FEATURE`、`AF_MSG_CLEAN_FEATURE`
- 用户态发送特征规则到内核，内核上报识别结果

### 5.2 用户态 ↔ LuCI 前端：ubus
- 通过 OpenWrt ubus 总线暴露 RPC 接口
- 提供设备列表查询、访问记录、统计报表、配置修改等 API

### 5.3 内核配置接口：`/proc/sys/oaf/`
- 通过 `/proc/sys/oaf/enable`、`record_enable`、`lan_ip` 等控制内核行为

### 5.4 规则下发：`/dev/appfilter`
- JSON 格式规则通过设备文件 `/dev/appfilter` 写入内核

---

## 六、工作模式

| 模式 | 说明 |
|------|------|
| **Gateway (0)** | 网关模式，路由器作为网关使用 |
| **Bypass (1)** | 旁路模式，仅监控不阻断 |
| **Bridge (2)** | 桥接模式，透明桥接 |

### 应用过滤模式
| 模式 | 说明 |
|------|------|
| **指定应用 (0)** | 仅阻断列表中指定的应用 |
| **全部应用 (1)** | 阻断所有已识别的应用 |

---

## 七、项目特点

1. **开源免费**：GPL v2 许可证，个人可免费使用，允许二次开发
2. **硬件要求低**：基于流的 DPI 识别，不需要大量计算资源
3. **不依赖 DNS**：直接分析数据包内容，DNS 劫持类方案无法绕过
4. **丰富的特征库**：内置数百种应用识别规则
5. **自定义扩展**：支持自定义特征 + AdGuard Home 规则语法
6. **OpenWrt 原生集成**：作为标准 OpenWrt 软件包，遵循 SDK 规范

---

## 八、目录结构概览

```
OpenAppFilter/
├── oaf/                    # 内核模块 (oaf.ko)
│   └── src/               # C 源码
│       ├── app_filter.c   # 核心 DPI 引擎 (1960行)
│       ├── af_conntrack.c # 连接跟踪
│       ├── af_rule_config.c # 规则配置
│       ├── af_whitelist_config.c # 白名单
│       ├── regexp.c       # 正则引擎
│       └── cJSON.c        # JSON 解析
├── open-app-filter/        # 用户空间守护进程 (oafd)
│   ├── src/               # C 源码
│   │   ├── main.c         # 主程序 (886行)
│   │   ├── appfilter_ubus.c # ubus 服务 (2486行)
│   │   ├── appfilter_user.c # 设备管理
│   │   ├── appfilter_netlink.c # Netlink 通信
│   │   ├── appfilter_custom_rule.c # 自定义规则
│   │   └── appfilter_config.c # UCI 配置
│   └── files/             # 配置和脚本文件
│       ├── appfilter.config  # 默认配置
│       ├── feature.cfg       # 应用特征库
│       ├── oaf_rule          # 规则下发脚本
│       └── gen_class.sh      # 分类生成脚本
├── luci-app-oaf/           # LuCI 管理界面
│   └── htdocs/luci-static/resources/
│       ├── app_icons/     # 200+ 应用图标
│       ├── css/           # 样式文件
│       └── echarts.min.js # 图表库
└── .github/workflows/     # CI 构建脚本