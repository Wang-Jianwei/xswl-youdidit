<#
工具: convert_encoding.ps1
用途: 安全地将单个文件转换为指定编码（原子写入，避免截断）
用法: .\tools\convert_encoding.ps1 -Path .\build_and_test.ps1 -Encoding utf8bom|unicode
示例: .\tools\convert_encoding.ps1 -Path .\build_and_test.ps1 -Encoding utf8bom
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$Path,

    [ValidateSet('utf8bom','unicode')]
    [string]$Encoding = 'utf8bom'
)

if (-not (Test-Path $Path)) { Write-Error "找不到文件: $Path"; exit 1 }

$content = Get-Content -Raw $Path

switch ($Encoding) {
    'utf8bom' { $enc = [System.Text.Encoding]::UTF8 }
    'unicode' { $enc = [System.Text.Encoding]::Unicode }
}

$tmp = "$Path.tmp"
[System.IO.File]::WriteAllText($tmp, $content, $enc)
Move-Item -Force $tmp $Path
Write-Host "已将 $Path 保存为 $Encoding 编码（原子替换完成）"