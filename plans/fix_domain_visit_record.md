# 域名访问记录修复计划

## 修改目标

1. `MAX_DOMAIN_VISIT_NUM` 改为 60
2. 内核上报周期改为 5 秒
3. CT 标记缓存逻辑保持不变

## 修改详情

### 修改 1: [`oaf/src/af_client.h`](oaf/src/af_client.h:35)

将 `MAX_DOMAIN_VISIT_NUM` 从 24 改为 60，扩大域名记录容量，减少淘汰。

```c
// 修改前
#define MAX_DOMAIN_VISIT_NUM 24

// 修改后
#define MAX_DOMAIN_VISIT_NUM 60
```

### 修改 2: [`oaf/src/af_client.c`](oaf/src/af_client.c:616-623)

每个客户端定时器当前每 2 秒触发一次，`timer_count` 累计到 30（60 秒）才上报一次域名记录。改为每 5 秒触发一次，每次触发即上报：

```c
// 修改前
if (client->timer_count >= 30) {
    __af_visit_info_report(client);
    client->timer_count = 0;
}
...
client->timer_count++;
mod_timer(&client->client_timer, jiffies + HZ * 2);

// 修改后
if (client->timer_count >= 1) {
    __af_visit_info_report(client);
    client->timer_count = 0;
}
...
client->timer_count++;
mod_timer(&client->client_timer, jiffies + HZ * 5);
```

### 修改 3: 内核上报每次条数限制

当前 [`af_client.c:310`](oaf/src/af_client.c:310) 每次上报域名记录最多 8 条，`MAX_DOMAIN_VISIT_NUM` 放大到 60 后，8 条的限制也建议适当放宽，改为 16 条避免溢出即可。

```c
// 修改前
if (domain_count >= 8) /* limit per report to 8 to avoid msg overflow */
    break;

// 修改后
if (domain_count >= 16) /* limit per report to 16 to avoid msg overflow */
    break;
```

### 修改 4: 用户态上报间隔

用户态 [`open-app-filter/src/appfilter_netlink.c:42`](open-app-filter/src/appfilter_netlink.c:42) 的 `REPORT_INTERVAL_SECS=60` 保持不变（只影响用户态处理，内核已改为 5s 上报）。

### 不变的部分

- CT 标记缓存路径（`app_filter.c:1666-1668`、`app_filter.c:1787-1788`）：**不修改**
- 用户态 `handle_dev_domain_list` 的 `max_domain=128` 和分页逻辑：**不修改**
- 域名记录在用户态的哈希表 `domain_htable[64]`：**不修改**

## 执行顺序

1. 修改 `af_client.h` 中的 `MAX_DOMAIN_VISIT_NUM`
2. 修改 `af_client.c` 中的定时器周期和上报阈值
3. 修改 `af_client.c` 中的每次上报条数上限