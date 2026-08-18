<#
.SYNOPSIS
  يلتقط نافذة عميل ROK2 من مخزنها الخاص عبر PrintWindow.

.DESCRIPTION
  CopyFromScreen يلتقط البكسلات المعروضة فعلاً على الشاشة، فأي نافذة تعلو
  اللعبة تُصوَّر مكانها — وهذا ما حدث: التقاط أول أعاد مستكشف الملفات.
  PrintWindow بعلم PW_RENDERFULLCONTENT (2) يطلب من النافذة أن ترسم نفسها في
  سياق جهاز نُمرّره، فالنتيجة محتوى النافذة لا ما يعلوها.
#>
param(
  [string]$Name = 'shot',
  [string]$OutDir = (Join-Path $env:TEMP 'rokshots')
)
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Cap {
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint flags);
  [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
  [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
'@
$proc = Get-Process UnrealEditor, Rok2 -ErrorAction SilentlyContinue |
        Where-Object { $_.MainWindowTitle -like 'Rok2*' } | Select-Object -First 1
if (-not $proc) { Write-Output 'NO_GAME_WINDOW'; exit 1 }
$h = $proc.MainWindowHandle
if ([Cap]::IsIconic($h)) { [Cap]::ShowWindow($h, 9) | Out-Null; Start-Sleep -Milliseconds 800 }
$r = New-Object Cap+RECT
[Cap]::GetClientRect($h, [ref]$r) | Out-Null
$w = $r.Right - $r.Left; $ht = $r.Bottom - $r.Top
if ($w -le 0 -or $ht -le 0) { Write-Output "BAD_RECT ${w}x${ht}"; exit 1 }
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
$bmp = New-Object System.Drawing.Bitmap $w, $ht
$g = [System.Drawing.Graphics]::FromImage($bmp)
$hdc = $g.GetHdc()
$okFull = [Cap]::PrintWindow($h, $hdc, 2)
$g.ReleaseHdc($hdc)
$g.Dispose()
$out = Join-Path $OutDir ($Name + '.png')
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Output "SAVED $out (${w}x${ht}) renderFullContent=$okFull title=$($proc.MainWindowTitle)"
