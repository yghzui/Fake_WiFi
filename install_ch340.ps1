# install_ch340.ps1
# 检测并自动安装 CH340 驱动程序

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "       CH340 驱动自动检测与安装脚本" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "正在检查系统中是否已安装 CH340 驱动..." -ForegroundColor Yellow

# 检查已安装的驱动列表中是否包含 CH340 相关的设备或驱动
$driverInstalled = Get-WmiObject Win32_PnPSignedDriver | Where-Object { 
    ($_.DeviceName -ne $null -and $_.DeviceName -match "CH340") -or 
    ($_.Description -ne $null -and $_.Description -match "CH340")
}

if ($driverInstalled) {
    Write-Host "✅ 检测完毕：系统已安装 CH340 驱动，无需重复安装。" -ForegroundColor Green
    Write-Host "你可以直接将 ESP32-CAM 插入电脑并开始烧录代码。" -ForegroundColor Green
    Exit
}

Write-Host "❌ 未检测到 CH340 驱动，准备自动下载并安装..." -ForegroundColor Yellow

# 官方驱动下载地址 (沁恒微电子)
$downloadUrl = "https://www.wch.cn/downloads/file/65.html" 
$installerPath = "$env:TEMP\CH341SER.EXE"

try {
    Write-Host "正在从官方服务器下载驱动程序 (CH341SER.EXE)..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $downloadUrl -OutFile $installerPath
    
    Write-Host "下载完成！准备启动安装程序..." -ForegroundColor Cyan
    Write-Host "⚠️ 注意：请在弹出的 UAC 管理员权限提示中点击『是』以允许安装。" -ForegroundColor Magenta
    
    # 使用管理员权限运行安装程序
    Start-Process -FilePath $installerPath -Wait -Verb RunAs
    
    Write-Host "✅ 驱动安装程序执行完毕！如果安装界面提示成功，请重新插拔开发板。" -ForegroundColor Green
} catch {
    Write-Host "❌ 自动下载或安装失败！" -ForegroundColor Red
    Write-Host "错误信息: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host "请手动访问官方网站下载安装: https://www.wch.cn/downloads/CH341SER_EXE.html" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "按任意键退出..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
