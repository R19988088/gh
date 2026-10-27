# 中文本地化资源

在项目根目录运行：

```sh
python3 guardian_heroes/tools/build_chinese_font.py
```

输入为保存的原始 `guardian_heroes/localization/font_original.xprc`，输出为
`guardian_heroes/localization/font_zh.xprc`。原始资源不会改写。
输出可以直接交给游戏原有的 XPRC 解压和 XPR2 字体加载路径。
构建需要 macOS、Xcode 命令行工具和系统中文字体。

只向 Default 主字体补齐 `guardian_heroes/localization/*zh*.json` 中 `zh`
字段实际使用的字符。Ending、StageTitle、StageTitleEu、CharaName 图集保持原始尺寸。
翻译文件更新后重新运行。

英文和原有非中文字符保留原始映射、字距和图像。中文及全角标点统一使用
CoreText 的 PingFang SC Regular 重绘，避免原日文字库与新增字形混用。全角
标点与汉字使用相同字距，并共享基线。所有图集保持原始宽度，行高仍取原字体的
24、37、75、75、27 像素。

XPRC 使用游戏 `sub_8242A588` 的独立 2048 字节块压缩格式：
短字面量、三种后向引用和零终止符，解压块按四字节对齐。
XPR2 的资源偏移以 12 字节文件头末尾为基准；字体数据为原生版本 5，
图像为线性 A4R4G4B4。工具会核对保留字符映射、原字形记录和图像覆盖值，
并对输出执行完整压缩、解压回读校验。

同名 `.json` 输出记录各字体新增数量及最终图集尺寸。

## 对话、菜单与安装

翻译保存在 `localization/dialogue_zh_*.json` 和 `localization/menu_zh.json`。
保留 `id`、`en` 和 `addresses`，只编辑 `zh`。剧情打包保留事件、翻页和
路线分支控制码；中文每行按 18 个汉字宽度排版，句末标点随前文排列。
菜单译文不得超过原文 UTF-16 容量，启动时再次核对原程序文本。

修改译文或字体后，在项目根目录运行：

```sh
python3 guardian_heroes/tools/build_chinese_font.py
python3 guardian_heroes/tools/build_localization.py --install-assets \
  guardian_heroes/assets 'Guardian Heroes.app/Contents/Resources/assets'
```

安装只替换指定资源目录内的 `font.xprc`、`menu_zhs.bin`、`script_zhs.mrg`。
原始字体保存在 `localization/font_original.xprc`，供之后重复构建。
运行中的游戏已加载字库和对白，更新资源后需在下次启动时读取。

游戏标题菜单或暂停菜单的 **Help & Options → Language** 可切换
English／简体中文／日本語／Français／Italiano／Deutsch／Español。
选中语言项后按左／右循环切换，确认键也可切到下一种语言。
选择自动保存在运行时用户数据目录的 `language.txt`，启动时恢复。
菜单即时刷新，正在进行的对白从下一页起使用所选语言。
六种原版语言沿用游戏原有的菜单语言表和剧情资源；中文沿用英文资源编号。
