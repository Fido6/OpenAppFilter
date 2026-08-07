# luci-app-oaf 暗色主题适配说明

## 概述
LuCI 主题切换时自动在 `<body>` 上添加 `dark` 类。本适配通过 CSS 变量实现主题切换，无需额外 JS。

## 设计原则

### 1. 所有颜色使用 `--oaf-*` 前缀的 CSS 变量
避免与 LuCI 原生变量冲突。变量定义在 [`common.css`](luci-app-oaf/htdocs/luci-static/resources/css/common.css) 中。

### 2. 双套变量：亮色/暗色
- `:root { ... }` — 亮色主题默认值
- `body.dark { ... }` — 暗色主题覆盖值

### 3. 变量命名规范

| 变量名 | 用途 |
|--------|------|
| `--oaf-primary` | 主按钮背景色 |
| `--oaf-primary-dark` | 主按钮 hover 色 |
| `--oaf-text-on-primary` | 主按钮上的文字色 |
| `--oaf-success` | 成功/确认按钮背景色 |
| `--oaf-danger` | 危险按钮背景色 |
| `--oaf-danger-alt` | 警告/清除按钮背景色 |
| `--oaf-warning-bg/border/text` | 警告框背景/边框/文字 |
| `--oaf-slider-bg/knob-bg` | 开关轨道/滑块颜色 |
| `--oaf-checked-slider-bg` | 开关选中色 |
| `--oaf-border-low/medium/high` | 边框色 |
| `--oaf-bg-high/low` | 背景色 |
| `--oaf-bg-card` | 卡片背景色（深色提示框等） |
| `--oaf-text-on-bg-card` | 深色卡片上的文字色 |
| `--oaf-text-muted` | 次要文字色 |
| `--oaf-checkbox-border` | 复选框边框色 |
| `--oaf-progress-bg/fill` | 进度条背景/填充色 |
| `--oaf-offline-color` | 离线状态文字色 |
| `--oaf-active-color` | 活跃状态文字色 |
| `--oaf-table-border` | 表格边框色 |
| `--oaf-text-used` | 已用尽文字色（红色） |
| `--oaf-link-color` | 链接/刷新按钮文字色 |

### 4. 新页面添加暗色适配的方法

在页面 `<style>` 中将硬编码颜色替换为对应变量：

```html
<!-- 之前 -->
<style>
    .my-button { background-color: #1a73e8; color: white; }
</style>

<!-- 之后 -->
<style>
    .my-button { 
        background-color: var(--oaf-primary); 
        color: var(--oaf-text-on-primary); 
    }
</style>
```

### 5. 内联样式中的颜色

内联 `style="..."` 中的颜色无法使用 CSS 变量直接写十六进制，应：
- **优先**：提取到 `<style>` 中定义类
- **次优**：使用 JS 动态设置 `element.style.color = getComputedStyle(...).getPropertyValue('--oaf-primary')`
- **兜底**：`<style>` 中定义 `body.dark .xxx { ... }` 覆盖

### 6. JS 动态生成 HTML 中的颜色

```javascript
// 之前
html += '<span style="color: #1a73e8;">Active</span>';

// 之后
var activeColor = getComputedStyle(document.documentElement)
    .getPropertyValue('--oaf-active-color').trim() || '#1a73e8';
html += '<span style="color: ' + activeColor + ';">Active</span>';
```

## 注意事项

1. **LuCI 自带 `body.dark` 类**，无需手动添加 JS 切换逻辑
2. 新变量必须前缀 `--oaf-`，避免与 LuCI 原生变量冲突
3. `getComputedStyle(document.documentElement)` 在 `body.dark` 生效时需要读取 `document.body`：
   ```javascript
   getComputedStyle(document.body).getPropertyValue('--oaf-primary')
   ```

## 未完全适配的部分（已知）

以下页面仍有部分硬编码颜色，因内联 `style="..."` 或 JS 动态生成难以全部替换，建议后续逐个处理：
- `app_filter.htm`: 内联 `style="color: #1a73e8"` 约 10 处
- `user_status.htm`: JS 分页按钮颜色约 6 处
- `user.htm`: 内联按钮颜色约 5 处
- `time.htm`: JS 生成进度条颜色约 3 处
