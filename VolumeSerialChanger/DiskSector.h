#ifndef __DISKSECTOR_INC__
#define __DISKSECTOR_INC__

#include <windows.h>
#include <tchar.h>

class DiskSector
{
private:
    HANDLE m_hDisk;

public:
    DiskSector();
    ~DiskSector();

    // Opens the physical logical drive (e.g., 'C')
    bool Open(TCHAR cDriveLetter);

    // Closes the drive handle and unlocks the volume
    void Close();

    // Reads a specific sector using a 64-bit offset
    bool ReadSector(ULONGLONG sector, char* buffer, DWORD sectorSize = 512);

    // Writes a specific sector using a 64-bit offset
    bool WriteSector(ULONGLONG sector, const char* buffer, DWORD sectorSize = 512);

private:
    // Locks and dismounts the volume to prevent Windows filesystem write blocks
    bool LockAndDismountVolume();
};

#endif