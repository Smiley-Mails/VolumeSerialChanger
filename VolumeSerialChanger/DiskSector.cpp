#include "DiskSector.h"
#include <stdio.h>
#include <winioctl.h>

DiskSector::DiskSector() : m_hDisk(INVALID_HANDLE_VALUE)
{}

DiskSector::~DiskSector()
{
    Close();
}

bool DiskSector::Open(TCHAR cDriveLetter)
{
    Close();

    TCHAR szDrivePath[10];
    _stprintf_s(szDrivePath, _countof(szDrivePath), _T("\\\\.\\%c:"), cDriveLetter);

    // Open the logical drive handle with direct access rights
    m_hDisk = ::CreateFile(
        szDrivePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (m_hDisk == INVALID_HANDLE_VALUE)
        return false;

    // Check if the target drive is the active system drive (usually C:)
    TCHAR szWinDir[MAX_PATH];
    ::GetWindowsDirectory(szWinDir, MAX_PATH);
    TCHAR cSystemDrive = szWinDir[0]; // Extract the system drive letter

    if (_totupper(cDriveLetter) == _totupper(cSystemDrive))
    {
        // For the system drive, we KNOW beforehand that FSCTL_LOCK_VOLUME will fail with Error 5.
        // Therefore, we bypass the locking phase and rely entirely on direct write privileges.
        return true;
    }

    // For all non-system drives (e.g., D:, E:), locking and dismounting is MANDATORY
    if (!LockAndDismountVolume())
    {
        Close();
        return false;
    }

    return true;
}

/*bool DiskSector::Open(TCHAR cDriveLetter)
{
    Close();

    TCHAR szDrivePath[10];
    _stprintf_s(szDrivePath, _countof(szDrivePath), _T("\\\\.\\%c:"), cDriveLetter);

    m_hDisk = ::CreateFile(
        szDrivePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (m_hDisk == INVALID_HANDLE_VALUE)
    {
        DWORD dwErr = ::GetLastError(); // Если 5 (ERROR_ACCESS_DENIED) — точно нет админ-прав
        TCHAR szBuf[128];
        _stprintf_s(szBuf, _countof(szBuf), _T("CreateFile failed. Error code: %d"), dwErr);
        ::MessageBox(NULL, szBuf, _T("Debug Error"), MB_ICONERROR);
        return false;
    }

    if (!LockAndDismountVolume())
    {
        DWORD dwErr = ::GetLastError(); // Если диск C:, тут упадет с ошибкой блокировки
        TCHAR szBuf[128];
        _stprintf_s(szBuf, _countof(szBuf), _T("Lock/Dismount failed. Error code: %d"), dwErr);
        ::MessageBox(NULL, szBuf, _T("Debug Error"), MB_ICONERROR);
        Close();
        return false;
    }

    return true;
}*/

//bool DiskSector::Open(TCHAR cDriveLetter)
//{
//    Close();
//
//    TCHAR szDrivePath[10];
//    // Format device path properly supporting both ANSI and Unicode targets
//    _stprintf_s(szDrivePath, _countof(szDrivePath), _T("\\\\.\\%c:"), cDriveLetter);
//
//    // Open the logical drive with direct access rights
//    m_hDisk = ::CreateFile(
//        szDrivePath,
//        GENERIC_READ | GENERIC_WRITE,
//        FILE_SHARE_READ | FILE_SHARE_WRITE,
//        NULL,
//        OPEN_EXISTING,
//        FILE_ATTRIBUTE_NORMAL,
//        NULL
//    );
//
//    if (m_hDisk == INVALID_HANDLE_VALUE)
//        return false;
//
//    // Modern Windows OS requires locking and dismounting the volume, 
//    // otherwise WriteFile to Sector 0 will fail with ACCESS_DENIED.
//    if (!LockAndDismountVolume())
//    {
//        Close();
//        return false;
//    }
//
//    return true;
//}

bool DiskSector::LockAndDismountVolume()
{
    DWORD bytesReturned;

    // Step 1: Request exclusive access by locking the volume
    if (!::DeviceIoControl(m_hDisk, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL))
        return false;

    // Step 2: Force dismount to flush any cached OS file system structures
    if (!::DeviceIoControl(m_hDisk, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL))
        return false;

    return true;
}

void DiskSector::Close()
{
    if (m_hDisk != INVALID_HANDLE_VALUE)
    {
        DWORD bytesReturned;
        // Unlock the volume so the OS can safely remount it
        ::DeviceIoControl(m_hDisk, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
        ::CloseHandle(m_hDisk);
        m_hDisk = INVALID_HANDLE_VALUE;
    }
}

bool DiskSector::ReadSector(ULONGLONG sector, char* buffer, DWORD sectorSize)
{
    if (m_hDisk == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER liOffset;
    liOffset.QuadPart = sector * sectorSize;

    // SetFilePointerEx is safe for 64-bit architectures (unlike SetFilePointer)
    if (!::SetFilePointerEx(m_hDisk, liOffset, NULL, FILE_BEGIN))
        return false;

    DWORD bytesRead = 0;
    return ::ReadFile(m_hDisk, buffer, sectorSize, &bytesRead, NULL) && (bytesRead == sectorSize);
}

bool DiskSector::WriteSector(ULONGLONG sector, const char* buffer, DWORD sectorSize)
{
    if (m_hDisk == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER liOffset;
    liOffset.QuadPart = sector * sectorSize;

    // Reposition the file pointer safely for x64/x86 large offsets
    if (!::SetFilePointerEx(m_hDisk, liOffset, NULL, FILE_BEGIN))
        return false;

    DWORD bytesWritten = 0;
    return ::WriteFile(m_hDisk, buffer, sectorSize, &bytesWritten, NULL) && (bytesWritten == sectorSize);
}