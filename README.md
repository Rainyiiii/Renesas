# Renesas Project Archive

这是 Renesas 嵌入式工程的版本归档仓库。每个日期目录代表一个可独立查看的工程版本。

## 当前版本

- [`2026-8-16`](./2026-8-16/)：当前工程版本

## 目录约定

后续版本按日期新增目录，例如：

```text
Renesas/
├── 2026-8-16/
├── 2026-8-17/
└── README.md
```

每个日期目录都是普通文件夹，不包含独立的 Git 仓库。

## 更新版本

在原始工程目录完成修改后，将工程复制为新的日期目录，然后在仓库根目录执行：

```powershell
git add -A
git commit -m "Add project 2026-8-17"
git push
```

例如：

```powershell
Copy-Item -LiteralPath 'D:\RA\second\rainy' `
  -Destination 'D:\RA\git\Renesas\2026-8-17' -Recurse
```

`.venv`、Python 缓存等本机环境文件会被 `.gitignore` 排除，不会上传到 GitHub。
