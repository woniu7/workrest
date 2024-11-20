# ���� Windows Forms ����
Add-Type -AssemblyName System.Windows.Forms


Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinAPI {
    // ����Windows API
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


# ����һ�����������ô����϶�
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

    // ���� Windows API ������������̺�����¼�
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetAsyncKeyState(int vKey);
    
    // Windows API ����
    const int WM_KEYDOWN = 0x0100;
    const int WM_KEYUP = 0x0101;
    const int WM_MOUSEMOVE = 0x0200;
    const int WM_LBUTTONDOWN = 0x0201;
    const int WM_LBUTTONUP = 0x0202;
    const int WM_RBUTTONDOWN = 0x0204;
    const int WM_RBUTTONUP = 0x0205;
    const int WM_SYSKEYDOWN = 0x0104;
    const int VK_MENU = 0x12; // Alt ���������ֵ

    // ��д���ڹ��������ü��̺����
    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WM_NCLBUTTONDOWN && m.WParam.ToInt32() == HTCAPTION) {
            // �������϶�����������Ϣ
            return;
        }
         // ��������� Alt ������ֹ��
        if (m.Msg == WM_KEYDOWN || m.Msg == WM_KEYUP)
        {
            if ((m.WParam.ToInt32() == VK_MENU))  // VK_MENU �� Alt ���������ֵ
            {
                return;  // ��ֹ Alt ��
            }
        }

        // ��������¼�
        if (m.Msg == WM_MOUSEMOVE || m.Msg == WM_LBUTTONDOWN || m.Msg == WM_LBUTTONUP || 
            m.Msg == WM_RBUTTONDOWN || m.Msg == WM_RBUTTONUP)
        {
            return;  // ��ֹ����¼�
        }

        base.WndProc(ref m);
    }
}
"@ -ReferencedAssemblies "System.Windows.Forms"


# ���庯������ʾ����ʱ����
function Show-CountdownForm {

# ��������
#$form = New-Object System.Windows.Forms.Form
# �����Զ���Ĺ̶�����
               $form = New-Object FixedForm
	$form.Text = "��Ϣ2����"
	#$form.Size = New-Object System.Drawing.Size(300, 150)

# ���ô��ڵ���ʼλ��Ϊ��Ļ����
#$form.StartPosition =[System.Windows.Forms.FormStartPosition]::CenterScreen
$form.WindowState = [System.Windows.Forms.FormWindowState]::Maximized  # ���ô���Ϊ���
$form.FormBorderStyle = [System.Windows.Forms.FormBorderStyle]::None  # ��ѡ����ֹ������С

# ���ùرհ�ť
	$form.ControlBox = $false
	$form.MaximizeBox = $false
	$form.MinimizeBox = $false
# ���ô���ʼ������ǰ��
                $form.TopMost = $true

                #$form.MaximizeBox = $false  # ��ֹ���

# ���� Alt + F4
$form.Add_KeyDown({
    if ($_.Alt -and $_.KeyCode -eq [System.Windows.Forms.Keys]::F4) {
        $_.Handled = $true  # ���� Alt+F4 �رմ���
    }
    if ($_.Alt -and ($_.KeyCode -eq [System.Windows.Forms.Keys]::Tab -or $_.KeyCode -eq [System.Windows.Forms.Keys]::Escape)) {
        $_.Handled = $true  # ���� Alt+Tab �� Alt+Esc
    }
})


# ������ǩ�ؼ�������ʾ����ʱ
	$label = New-Object System.Windows.Forms.Label
                $label.Font = New-Object System.Drawing.Font("Arial", 180) # ��������ʹ�С

                $label.AutoSize = $true
                $label.TextAlign = 'MiddleCenter'
	$label.Location = New-Object System.Drawing.Point(850,330)
	$form.Controls.Add($label)

# ��ʼ������ʱ��������2���� = 120�룩
	$script:timeRemaining = 200
# ������ʱ���������ڸ��µ���ʱ
	$timer = New-Object System.Windows.Forms.Timer
	$timer.Interval = 1000  # ÿ���Ӵ���һ��

# ���ü�ʱ������ʱ���¼�
	$timer.Add_Tick({  
                        $script:timeRemaining -= 1
                       # Write-Host $count
    $label.Text = $script:timeRemaining
                     Write-Host $script:timeRemaining
	    # �������ʱ������ֹͣ��ʱ�����رմ���
	    if ($script:timeRemaining -le 0) {
	   	$timer.Stop()  # ֹͣ��ʱ��
	   	$form.Close()  # �رմ���
	    }
                 })

# �ڴ�����ʾ��������ʱ��
$form.Add_Shown({
    $timer.Start()  # ������ʱ��
})
# ��ʾ����
$form.ShowDialog()


}

    # ÿСʱִ��һ��
    while ($true) {
        Write-Host "��ʼ��Ϣ3���ӡ���"
        # ���ú�����ʾ����ʱ����
        Show-CountdownForm
 
       # ��ͣ1Сʱ��3600�룩
       Write-Host "��ʼ����50���ӡ���"
        Start-Sleep -Seconds 3000
    }