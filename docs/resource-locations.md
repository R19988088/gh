# 图标与标题画面资源

项目根目录：`/Users/ddd/Documents/xbla-守护英雄`。

## Mac 应用图标

| 资源 | 位置 |
| --- | --- |
| 项目内的 Icon Composer 源文件 | `guardian_heroes/assets/macos/Guardian Heroes.icon/` |
| 用户提供的原始文件 | `/Users/ddd/Desktop/Guardian Heroes.icon/` |
| 图层与外观配置 | `guardian_heroes/assets/macos/Guardian Heroes.icon/icon.json` |
| 图标内使用的 PNG | `guardian_heroes/assets/macos/Guardian Heroes.icon/Assets/` |

图标保留 Icon Composer 中的图层位置、缩放、背景、玻璃效果与阴影。

已安装到这两个应用包：

- `Guardian Heroes.app`
- `dist/2026-10-17-macos-arm64/Guardian Heroes.app`

每个应用包的 `Contents/Resources/` 包含：

- `Assets.car`：Xcode 编译的图标资源。
- `Guardian Heroes.icns`：Mac 图标兼容资源。

每个应用包的 `Contents/Info.plist` 均设置：

```xml
<key>CFBundleIconFile</key>
<string>Guardian Heroes</string>
<key>CFBundleIconName</key>
<string>Guardian Heroes</string>
```

修改项目内的 `.icon` 后，在项目根目录用 Xcode 重新编译：

```sh
mkdir -p /tmp/guardian-heroes-icon-compile
xcrun actool 'guardian_heroes/assets/macos/Guardian Heroes.icon' \
  --compile /tmp/guardian-heroes-icon-compile \
  --platform macosx --target-device mac --minimum-deployment-target 13.3 \
  --app-icon 'Guardian Heroes' \
  --output-partial-info-plist /tmp/guardian-heroes-icon-compile/icon-info.plist \
  --output-format human-readable-text --warnings --errors
```

将输出的 `Assets.car` 和 `Guardian Heroes.icns` 复制到目标应用包的
`Contents/Resources/`，将 `icon-info.plist` 的图标字段合并到应用的
`Contents/Info.plist`。已签名的应用在资源更新后需要重新签名。

## 从游戏归档导出的图片

| 内容 | PNG 位置 | 原始尺寸 | 归档来源（从 0 编号） |
| --- | --- | --- | --- |
| 红黄圆形 Guardian Heroes 标志，透明背景 | `guardian_heroes/assets/guardian_heroes_title_logo.png` | 544 × 640 | `guardian_heroes/assets/media/title.mrg`，条目 10 |
| 蓝色石刻标志标题背景 | `guardian_heroes/assets/guardian_heroes_title_background.png` | 1280 × 720 | `guardian_heroes/assets/media/title.mrg`，条目 0 |

Logo 解压后的纹理格式编号为 2，像素数据从解压结果的 4096 字节处开始，
使用 Xbox 360 平铺布局。解除平铺后按 **ARGB → RGBA** 转换；第一个字节
是透明度。不能将原始四字节直接视为 RGBA，否则主体会几乎透明且颜色错误。

背景纹理格式编号为 4，解除平铺并交换每个 16 位数据的字节序后按 BC1
解码。上表 PNG 保留游戏资源的原始分辨率。

## 更新前备份

本次图标更新前的应用信息、发布包签名文件及已有运行日志保存在：

`backup/2026-10-18-145404-macos-icon/`
