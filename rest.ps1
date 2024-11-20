# 加载 Windows Forms 程序集
Add-Type -AssemblyName System.Windows.Forms


Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinAPI {
    // 定义Windows API
    [DllImport("user32.dll")]
    public static extern bool RegisterHotKey(IntPtr hWnd, int id, int fsModifiers, int vk);
    
    [DllImport("user32.dll")]
    public static extern bool UnregisterHotKey(IntPtr hWnd, int id);
    
    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();
    
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    
    public const int SW_RESTORE = 9;
    public const int MOD_ALT = 0x0001;
    public const int VK_TAB = 0x09;
    public const int VK_ESCAPE = 0x1B;
}
"@


# 定义一个函数来禁用窗口拖动
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;

public class FixedForm : Form {
    private const int WM_NCLBUTTONDOWN = 0xA1;
    private const int HTCAPTION = 0x2;

    [DllImport("user32.dll")]
    private static extern bool ReleaseCapture();

    [DllImport("user32.dll")]
    private static extern IntPtr SendMessage(IntPtr hWnd, int Msg, int wParam, int lParam);

    // 导入 Windows API 函数来处理键盘和鼠标事件
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetAsyncKeyState(int vKey);
    
    // Windows API 常量
    const int WM_KEYDOWN = 0x0100;
    const int WM_KEYUP = 0x0101;
    const int WM_MOUSEMOVE = 0x0200;
    const int WM_LBUTTONDOWN = 0x0201;
    const int WM_LBUTTONUP = 0x0202;
    const int WM_RBUTTONDOWN = 0x0204;
    const int WM_RBUTTONUP = 0x0205;
    const int WM_SYSKEYDOWN = 0x0104;
    const int VK_MENU = 0x12; // Alt 键的虚拟键值

    // 重写窗口过程来禁用键盘和鼠标
    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WM_NCLBUTTONDOWN && m.WParam.ToInt32() == HTCAPTION) {
            // 不处理拖动标题栏的消息
            return;
        }
         // 如果按下了 Alt 键，阻止它
        if (m.Msg == WM_KEYDOWN || m.Msg == WM_KEYUP)
        {
            if ((m.WParam.ToInt32() == VK_MENU))  // VK_MENU 是 Alt 键的虚拟键值
            {
                return;  // 阻止 Alt 键
            }
        }

        // 禁用鼠标事件
        if (m.Msg == WM_MOUSEMOVE || m.Msg == WM_LBUTTONDOWN || m.Msg == WM_LBUTTONUP || 
            m.Msg == WM_RBUTTONDOWN || m.Msg == WM_RBUTTONUP)
        {
            return;  // 阻止鼠标事件
        }

        base.WndProc(ref m);
    }
}
"@ -ReferencedAssemblies "System.Windows.Forms"


# 定义函数，显示倒计时窗体
function Show-CountdownForm {

# 创建窗体
#$form = New-Object System.Windows.Forms.Form
# 创建自定义的固定窗体
               $form = New-Object FixedForm
	$form.Text = "休息2分钟"
	#$form.Size = New-Object System.Drawing.Size(300, 150)

# 设置窗口的起始位置为屏幕中央
#$form.StartPosition =[System.Windows.Forms.FormStartPosition]::CenterScreen
$form.WindowState = [System.Windows.Forms.FormWindowState]::Maximized  # 设置窗口为最大化
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::None  # 可选，防止调整大小

# 禁用关闭按钮
	$form.ControlBox = $false
	$form.MaximizeBox = $false
	$form.MinimizeBox = $false
# 设置窗体始终在最前面
                $form.TopMost = $true

                #$form.MaximizeBox = $false  # 禁止最大化

# 禁用 Alt + F4
$form.Add_KeyDown({
    if ($_.Alt -and $_.KeyCode -eq [System.Windows.Forms.Keys]::F4) {
        $_.Handled = $true  # 禁用 Alt+F4 关闭窗口
    }
    if ($_.Alt -and ($_.KeyCode -eq [System.Windows.Forms.Keys]::Tab -or $_.KeyCode -eq [System.Windows.Forms.Keys]::Escape)) {
        $_.Handled = $true  # 禁用 Alt+Tab 和 Alt+Esc
    }
})


# 创建标签控件用于显示倒计时
	$label = New-Object System.Windows.Forms.Label
                $label.Font = New-Object System.Drawing.Font("Arial", 180) # 设置字体和大小

                $label.AutoSize = $true
                $label.TextAlign = 'MiddleCenter'
	$label.Location = New-Object System.Drawing.Point(850,330)
	$form.Controls.Add($label)

# 初始化倒计时的秒数（2分钟 = 120秒）
	$script:timeRemaining = 200
# 创建计时器对象，用于更新倒计时
	$timer = New-Object System.Windows.Forms.Timer
	$timer.Interval = 1000  # 每秒钟触发一次

# 设置计时器触发时的事件
	$timer.Add_Tick({  
                        $script:timeRemaining -= 1
                       # Write-Host $count
    $label.Text = $script:timeRemaining
                     Write-Host $script:timeRemaining
	    # 如果倒计时结束，停止计时器并关闭窗体
	    if ($script:timeRemaining -le 0) {
	   	$timer.Stop()  # 停止计时器
	   	$form.Close()  # 关闭窗体
	    }
                 })

# 在窗体显示后启动计时器
$form.Add_Shown({
    $timer.Start()  # 启动计时器
})
# 显示窗体
$form.ShowDialog()


}

    # 每小时执行一次
    while ($true) {
        Write-Host "开始休息3分钟……"
        # 调用函数显示倒计时窗体
        Show-CountdownForm
 
       # 暂停1小时（3600秒）
       Write-Host "开始工作50分钟……"
        Start-Sleep -Seconds 3000
    }