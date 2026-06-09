#include "stdafx.h"
#include "VolumeSerial.h"
#include "VolumeSerialDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVolumeSerialDlg dialog

CVolumeSerialDlg::CVolumeSerialDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVolumeSerialDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVolumeSerialDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVolumeSerialDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVolumeSerialDlg)
	DDX_Control(pDX, IDC_EDT_NEWSERIAL, m_edtNewSerial);
	DDX_Control(pDX, IDC_EDT_INFORMATION, m_edtInformation);
	DDX_Control(pDX, IDC_LB_DRIVES, m_lbDrives);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVolumeSerialDlg, CDialog)
    //{{AFX_MAP(CVolumeSerialDlg)
    ON_BN_CLICKED(IDC_BTN_ABOUT, OnBtnAbout)
    ON_BN_CLICKED(IDC_BTN_CHANGESERIAL, OnBtnChangeserial)
    ON_CBN_SELCHANGE(IDC_LB_DRIVES, OnSelchangeLbDrives)
    ON_EN_CHANGE(IDC_EDT_NEWSERIAL, OnEnChangeEdtNewSerial)
    ON_BN_CLICKED(IDBACKUP, OnBtnBackup)
    //}}AFX_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVolumeSerialDlg message handlers

BOOL CVolumeSerialDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// Populate the drives
    PopulateDriveListBox();
  
    // m_edtNewSerial.SetWindowText(_T("1234-ABCD"));

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CVolumeSerialDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVolumeSerialDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CVolumeSerialDlg::ShowErrorString(const DWORD nErr)
{
  TCHAR szErrorMsg[100];

  ::FormatMessage(
     FORMAT_MESSAGE_FROM_SYSTEM,
     NULL,
     nErr,
     LANG_USER_DEFAULT,
     (LPTSTR)szErrorMsg,
     sizeof(szErrorMsg),
     NULL);
  
  MessageBox(szErrorMsg, _T("Error"), MB_ICONERROR);
}

void CVolumeSerialDlg::OnBtnBackup()
{
    // 1. Resolve the path of the currently executing module (.exe)
    TCHAR szExePath[MAX_PATH];
    ::GetModuleFileName(NULL, szExePath, MAX_PATH);

    // Find the last trailing backslash to truncate the filename
    TCHAR* pSlash = _tcsrchr(szExePath, _T('\\'));
    if (pSlash != NULL)
    {
        *(pSlash + 1) = _T('\0'); // Preserve the trailing slash (e.g., "C:\App\")
    }

    // Combine paths to create the backup log file location
    CString strFilePath;
    strFilePath.Format(_T("%sDiskInfoBack.txt"), szExePath);

    // 2. Open or create the log file in standard text mode
    CStdioFile file;
    if (!file.Open(strFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeText))
    {
        MessageBox(_T("Failed to create log file for exporting disk info!"), _T("Error"), MB_ICONERROR);
        return;
    }

    // 3. Iterate through all discovered drives present in the ComboBox
    int nCount = m_lbDrives.GetCount();
    for (int i = 0; i < nCount; i++)
    {
        DWORD_PTR driveData = m_lbDrives.GetItemData(i);
        if (driveData == 0) continue;

        TCHAR cDrive = (TCHAR)(driveData & 0xFF);
        TCHAR szRootPath[5];
        _stprintf_s(szRootPath, _countof(szRootPath), _T("%c:\\"), cDrive);

        TCHAR szVolumeName[MAX_PATH] = { 0 };
        TCHAR szFsName[MAX_PATH] = { 0 };
        DWORD dwSerial = 0;
        DWORD dwMaxCompLen = 0;
        DWORD dwFlags = 0;

        // Retrieve live attributes from the specific OS filesystem volume
        if (::GetVolumeInformation(szRootPath, szVolumeName, MAX_PATH, &dwSerial, &dwMaxCompLen, &dwFlags, szFsName, MAX_PATH))
        {
            CString strLine;
            // Write formatted drive details mapping exact requested patterns
            strLine.Format(_T("%c:  [%s]\t\t[%s] -- Serial is %04X-%04X\n"),
                cDrive,
                szVolumeName,
                szFsName,
                HIWORD(dwSerial),
                LOWORD(dwSerial));

            file.WriteString(strLine);
        }
        else
        {
            // Fallback line output for unmounted, locked, or empty drives
            CString strLine;
            strLine.Format(_T("%c:  [Unavailable]\n"), cDrive);
            file.WriteString(strLine);
        }
    }

    file.Close();

    // Notify the user about the successfully generated file path location
    CString strSuccess;
    strSuccess.Format(_T("Disk parameters saved successfully to:\n%s"), strFilePath.GetString());
    MessageBox(strSuccess, _T("Success"), MB_ICONINFORMATION);
}

void CVolumeSerialDlg::PopulateDriveListBox()
{
    // Clear old list content
    m_lbDrives.ResetContent();

    // Add your special flag at index 0 that allows the list to be refreshed
    m_lbDrives.AddString(_T("-- refresh list --"));
    m_lbDrives.SetItemData(0, 0);

    TCHAR szDriveBuffer[MAX_PATH] = { 0 };

    // Securely fetch logical drive strings (fully compatible with x86/x64)
    DWORD dwBufferLen = ::GetLogicalDriveStrings(MAX_PATH, szDriveBuffer);

    if (dwBufferLen == 0 || dwBufferLen > MAX_PATH)
    {
        MessageBox(_T("Failed to retrieve logical drives!"), _T("Error"), MB_ICONERROR);
        return;
    }

    TCHAR* pDrive = szDriveBuffer;
    while (*pDrive)
    {
        // Filter only hard drives and removable flash drives
        UINT uDriveType = ::GetDriveType(pDrive);

        if (uDriveType == DRIVE_REMOVABLE || uDriveType == DRIVE_FIXED)
        {
            TCHAR cDriveLetter = pDrive[0]; // Extract letter (e.g., 'C')

            CString strDisplay;
            strDisplay.Format(_T("Drive %c:"), cDriveLetter);

            int nIndex = m_lbDrives.AddString(strDisplay);
            if (nIndex != CB_ERR)
            {
                // CRITICAL FOR x64: Store character data directly as DWORD_PTR 
                // to match the architectural register width safely.
                m_lbDrives.SetItemData(nIndex, static_cast<DWORD_PTR>(cDriveLetter));
            }
        }

        // Advance to the next null-terminated drive path string inside the buffer
        pDrive += _tcslen(pDrive) + 1;
    }

    // Automatically select the first actual drive (index 1, right after refresh option)
    if (m_lbDrives.GetCount() > 1)
    {
        m_lbDrives.SetCurSel(1);
        OnSelchangeLbDrives(); // Force update information block for the selected drive
    }
}

void CVolumeSerialDlg::OnSelchangeLbDrives()
{
    int curSel = m_lbDrives.GetCurSel();
    if (curSel == 0)
    {
        PopulateDriveListBox();
        return;
    }
    if (curSel == CB_ERR) return;

    // Retrieve the stored drive letter character packed into DWORD_PTR
    DWORD_PTR driveData = m_lbDrives.GetItemData(curSel);
    if (driveData == 0) return;

    TCHAR cDrive = (TCHAR)(driveData & 0xFF);

    TCHAR szRootPath[5];
    _stprintf_s(szRootPath, _countof(szRootPath), _T("%c:\\"), cDrive);

    TCHAR szVolumeName[MAX_PATH] = { 0 }; // Reallocated buffer to store the volume label
    TCHAR szFsName[MAX_PATH] = { 0 };
    DWORD dwSerial = 0;
    DWORD dwMaxCompLen = 0;
    DWORD dwFlags = 0;

    // Fetch live volume information from the OS
    if (::GetVolumeInformation(szRootPath, szVolumeName, MAX_PATH, &dwSerial, &dwMaxCompLen, &dwFlags, szFsName, MAX_PATH))
    {
        CString strInfo;

        // Format and append information strings using standard safe carriage returns
         // Line 1: Drive Letter (e.g., C:)
        strInfo.AppendFormat(_T("Drive letter: %c:\r\n"), cDrive);

        // Line 3: File System type (e.g., NTFS, FAT32)
        strInfo.AppendFormat(_T("File system: %s\r\n"), szFsName);

        // Line 2: Volume Label (e.g., Windows, Local Disk or [No Label] if empty)
        strInfo.AppendFormat(_T("Volume label: %s\r\n"), szVolumeName[0] != _T('\0') ? szVolumeName : _T("[No Label]"));

        // Line 4: Current Serial Number formatted as XXXX-XXXX
        strInfo.AppendFormat(_T("Current Serial Number: %04X-%04X\r\n"), HIWORD(dwSerial), LOWORD(dwSerial));

        m_edtInformation.SetWindowText(strInfo);

        // Pre-populate the new serial input field with the current value as a convenient hint
        CString strHexSerial;
        strHexSerial.Format(_T("%04X-%04X"), HIWORD(dwSerial), LOWORD(dwSerial));
        m_edtNewSerial.SetWindowText(strHexSerial);
    }
    else
    {
        // Display fallback string if the disk is locked, unformatted, or unreadable
        m_edtInformation.SetWindowText(_T("Failed to retrieve volume parameters.\r\nDrive might be unformatted or restricted."));
        m_edtNewSerial.SetWindowText(_T(""));
    }
}

void CVolumeSerialDlg::ChangeSerialNumber(DWORD_PTR Drive, const DWORD newSerial)
{
    const int max_pbsi = 3;

    // Struct definitions must remain explicit char* (ANSI) 
    // because boot sector headers are always raw byte strings.
    struct partial_boot_sector_info
    {
        const char* Fs;     // ANSI signature inside the boot sector
        DWORD FsOffs;       // Offset of the signature
        DWORD SerialOffs;   // Offset of the Serial Number
    };

    partial_boot_sector_info pbsi[max_pbsi] =
    {
        {"FAT32", 0x52, 0x43},
        {"FAT",   0x36, 0x27},
        {"NTFS",  0x03, 0x48} // Note: NTFS serials are 64-bit, writing 32-bit changes lower DWORD
    };

    char Sector[512] = { 0 };
    TCHAR cDrive = (TCHAR)(Drive & 0xFF);

    // Try to open the disk with administrative privileges
    if (!disk.Open(cDrive))
    {
        MessageBox(_T("Failed to gain exclusive access to the drive! Run the app as Administrator."), _T("Error"), MB_ICONERROR);
        return;
    }

    // Read Sector 0 (Boot Sector)
    if (!disk.ReadSector(0, Sector))
    {
        MessageBox(_T("Error reading the initial boot sector!"), _T("Error"), MB_ICONERROR);
        disk.Close();
        return;
    }

    // Scan for filesystem signature match within the sector buffer
    int i = 0;
    for (i = 0; i < max_pbsi; i++)
    {
        if (strncmp(pbsi[i].Fs, Sector + pbsi[i].FsOffs, strlen(pbsi[i].Fs)) == 0)
        {
            break;
        }
    }

    if (i >= max_pbsi)
    {
        MessageBox(_T("Unsupported file system or corrupt boot sector!"), _T("Error"), MB_ICONERROR);
        disk.Close();
        return;
    }

    // Patch the serial number bytes via safe type casting compatible with x86 and x64 execution
    DWORD* pSerial = reinterpret_cast<DWORD*>(Sector + pbsi[i].SerialOffs);
    *pSerial = newSerial;

    // Write the patched sector back to disk
    if (!disk.WriteSector(0, Sector))
    {
        MessageBox(_T("Failed to write to sector! Ensure no background handles are blocking the drive."), _T("Error"), MB_ICONERROR);
        disk.Close();
        return;
    }

    disk.Close();

    MessageBox(_T("Volume serial number changed successfully!\nRe-plug the device or restart for changes to take effect."),
        _T("Success"), MB_ICONINFORMATION);
}

void CVolumeSerialDlg::OnBtnAbout() 
{
  MessageBox(_T("Change Volume Serial Number v2.0\n")
             _T("(c) smiley.mails.2025@gmail.com\n"), _T("About"), MB_ICONINFORMATION);	
}


void CVolumeSerialDlg::OnBtnChangeserial()
{
    if (MessageBox(_T("Are you sure you want to change the serial number?"), _T("Warning"), MB_ICONQUESTION | MB_ICONWARNING | MB_YESNO) == IDNO)
        return;

    int curSel = m_lbDrives.GetCurSel();
    if (curSel == CB_ERR) return;

    // Use DWORD_PTR instead of int/DWORD to keep standard pointer width on x64
    DWORD_PTR drive = m_lbDrives.GetItemData(curSel);

    if (drive != 0)
    {
        CString strInput;
        m_edtNewSerial.GetWindowText(strInput);

        // Strip out the dash formatting character to validate raw hex input
        CString strCleanHex = _T("");
        for (int i = 0; i < strInput.GetLength(); i++)
        {
            if (strInput[i] != _T('-'))
            {
                strCleanHex += strInput[i];
            }
        }

        // Validate that we have exactly 8 characters (32-bit integer in HEX format)
        if (strCleanHex.GetLength() != 8)
        {
            MessageBox(_T("Please enter a valid 8-digit hexadecimal number (XXXX-XXXX)!"), _T("Validation Error"), MB_ICONEXCLAMATION);
            return;
        }

        TCHAR* pEnd = nullptr;
        // Parse the clean hex string properly supporting Unicode compilation targets
        DWORD newSerial = _tcstoul(strCleanHex, &pEnd, 16);

        ChangeSerialNumber(drive, newSerial);
    }
}

void CVolumeSerialDlg::OnEnChangeEdtNewSerial()
{
    // Static flag to prevent infinite recursion loop during SetWindowText
    static bool bInChange = false;
    if (bInChange) return;

    bInChange = true;

    CString strText;
    m_edtNewSerial.GetWindowText(strText);

    // Save the current cursor selection point
    int nStartChar, nEndChar;
    m_edtNewSerial.GetSel(nStartChar, nEndChar);

    // Filter out existing dashes to work with raw hex characters
    CString strClean = _T("");
    for (int i = 0; i < strText.GetLength(); i++)
    {
        if (strText[i] != _T('-'))
        {
            strClean += strText[i];
        }
    }

    // Limit maximum length to 8 hex characters (e.g., 4 bytes DWORD)
    if (strClean.GetLength() > 8)
    {
        strClean = strClean.Left(8);
    }

    // Format the text into XXXX-XXXX mask pattern dynamically
    CString strFormatted = _T("");
    for (int i = 0; i < strClean.GetLength(); i++)
    {
        if (i == 4)
        {
            strFormatted += _T('-'); // Inject dash after the 4th hex character
        }
        strFormatted += strClean[i];
    }

    // Apply the formatted string back to the edit control if changed
    if (strText != strFormatted)
    {
        m_edtNewSerial.SetWindowText(strFormatted);

        // Adjust caret position based on dash addition or deletion mechanics
        if (nStartChar == 5 && strFormatted.GetLength() >= 5)
        {
            // Caret was right after the added dash, push it forward
            m_edtNewSerial.SetSel(nStartChar + 1, nEndChar + 1);
        }
        else if (nStartChar > strFormatted.GetLength())
        {
            // Handle edge adjustments for selection boundaries
            m_edtNewSerial.SetSel(strFormatted.GetLength(), strFormatted.GetLength());
        }
        else
        {
            // Restore normal relative cursor position
            m_edtNewSerial.SetSel(nStartChar, nEndChar);
        }
    }

    bInChange = false;
}

void CVolumeSerialDlg::ShowErrorString(const char *msg)
{
  MessageBox(msg, "Info", MB_ICONINFORMATION);
}