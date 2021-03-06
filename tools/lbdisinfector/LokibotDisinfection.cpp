// LokibotDisinfection.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "Shlobj.h"
#include "Shlwapi.h"
#include "Strsafe.h"

#define MD5LEN          16

#define LOKIBOTFILES    0x10
#define LOKIBOTREGK     0x20

struct LOKIBOTPERSISTENCE {
    char szMachineGuid[50];
    char szMachineGuidMD5[MD5LEN * 2 + 1];
    char szPersistenceFolder[MAX_PATH];
    char szFolder[MAX_PATH];
    char szFilename[MAX_PATH];
    char szExe[MAX_PATH];
    char szHdb[MAX_PATH];
    char szLck[MAX_PATH];
    char szKdb[MAX_PATH];
    char szRegKey[MAX_PATH];
};


char* GetMachineGuid() {
    HKEY hkey;
    static char szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hkey) != ERROR_SUCCESS) {
        return 0;
    }

    if (RegQueryValueExA(hkey, "MachineGuid", 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) != ERROR_SUCCESS) {
        return 0;
    }
    return szBuffer;
}


char* md5(char* szMachineGuid, size_t MachineGuidSize) {
    // MD5 MachineGuid
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTHASH hHash = NULL;
    HCRYPTHASH hHexHash = NULL;
    DWORD cbHash = MD5LEN;
    BYTE rgbHash[MD5LEN];
    static CHAR hashString[MD5LEN * 2 + 1];

    if (!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        printf("ERROR_3\n");
        getchar();
        exit(1);
    }

    // Acquire a hash object handle.
    if (!CryptCreateHash(hCryptProv, CALG_MD5, 0, 0, &hHash))
    {
        printf("ERROR_4\n");
        getchar();
        exit(1);
    }

    CryptHashData(hHash, (const BYTE *)szMachineGuid, MachineGuidSize, 0);

    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        for (DWORD i = 0; i < cbHash; i++)
        {
            sprintf(&hashString[i * 2], "%02X", rgbHash[i] & 0xFF);
        }
        hashString[MD5LEN * 2] = '\x00';

        CryptDestroyHash(hHash);
        return hashString;
    }
    else
    {
        return 0;
    }
}


int removeDir(char* dir) {
    CHAR szDir[MAX_PATH];
    CHAR aux[MAX_PATH];
    memcpy(szDir, dir, strlen(dir));

    szDir[strlen(dir)] = '\\';

    memcpy(aux, szDir, strlen(dir) + 1);
    szDir[strlen(dir) + 1] = '*';
    szDir[strlen(dir) + 2] = 0;
    aux[strlen(dir) + 1] = 0;
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(szDir, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        return 0;
    }

    do
    {
        aux[strlen(dir) + 1] = 0;
        StringCchCatA(aux, MAX_PATH, ffd.cFileName);
        if (DeleteFileA(aux) == 0) {
            //printf("\t\t[!] Error %d deleting %s.", GetLastError(), aux);
        }
        printf("\t\t+ Deleted: %s\n", aux);
    } while (FindNextFileA(hFind, &ffd) != 0);

    RemoveDirectoryA(dir);

    return 1;

}


int checkLokiBotRegistry(LOKIBOTPERSISTENCE* LokiBot) {
    int loki = 0;
    HKEY hkey;

    HKEY regKeys[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };

    static char szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    char strToComp[MAX_PATH];

    strcpy(strToComp, LokiBot->szFolder);
    StringCchCatA(strToComp, MAX_PATH, "\\");
    StringCchCatA(strToComp, MAX_PATH, LokiBot->szFilename);
    StringCchCatA(strToComp, MAX_PATH, ".exe");

    for (int i = 0; i < 2; i++) {
        if (RegOpenKeyExA(regKeys[i], LokiBot->szRegKey, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hkey, LokiBot->szFolder, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS) {

                if (strcmp(&szBuffer[strlen(szBuffer) - strlen(LokiBot->szFolder) - strlen(LokiBot->szFilename) - 5], strToComp) == 0) {
                    loki = LOKIBOTREGK;
                }
            }
            RegCloseKey(hkey);
        }
    }
    return loki;
}


int removeRegKeys(LOKIBOTPERSISTENCE* LokiBot) {
    int error = 0;
    HKEY hkey;

    HKEY regKeys[2] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };

    static char szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    char strToComp[MAX_PATH];

    strcpy(strToComp, LokiBot->szFolder);
    StringCchCatA(strToComp, MAX_PATH, "\\");
    StringCchCatA(strToComp, MAX_PATH, LokiBot->szFilename);
    StringCchCatA(strToComp, MAX_PATH, ".exe");

    for (int i = 0; i < 2; i++) {
        if (RegOpenKeyExA(regKeys[i], LokiBot->szRegKey, 0, KEY_ALL_ACCESS, &hkey) == ERROR_SUCCESS) {
            if (RegQueryValueExA(hkey, LokiBot->szFolder, 0, NULL, (LPBYTE)szBuffer, &dwBufferSize) == ERROR_SUCCESS) {

                if (strcmp(&szBuffer[strlen(szBuffer) - strlen(LokiBot->szFolder) - strlen(LokiBot->szFilename) - 5], strToComp) == 0) {
                    if (RegDeleteValueA(hkey, LokiBot->szFolder) != ERROR_SUCCESS) {
                        error = 1;
                    }
                }
            }
            RegCloseKey(hkey);
        }
    }
    return error;
}


int checkLokiBotFiles(LOKIBOTPERSISTENCE* LokiBot) {
    int loki = 0;
    if (GetFileAttributesA(LokiBot->szPersistenceFolder) != INVALID_FILE_ATTRIBUTES && GetFileAttributesA(LokiBot->szPersistenceFolder) & FILE_ATTRIBUTE_DIRECTORY) {
        if (GetFileAttributesA(LokiBot->szExe) != INVALID_FILE_ATTRIBUTES)
            loki = LOKIBOTFILES;
        if (GetFileAttributesA(LokiBot->szHdb) != INVALID_FILE_ATTRIBUTES)
            loki = LOKIBOTFILES;
        if (GetFileAttributesA(LokiBot->szLck) != INVALID_FILE_ATTRIBUTES)
            loki = LOKIBOTFILES;
        if (GetFileAttributesA(LokiBot->szKdb) != INVALID_FILE_ATTRIBUTES)
            loki = LOKIBOTFILES;
    }
    return loki;
}


int isThereLokibot(LOKIBOTPERSISTENCE* LokiBot) {
    return checkLokiBotFiles(LokiBot) | checkLokiBotRegistry(LokiBot);
}

LOKIBOTPERSISTENCE* loadLokiBot() {
    static LOKIBOTPERSISTENCE LokiBot;

    strcpy(LokiBot.szMachineGuid, GetMachineGuid());
    strcpy(LokiBot.szMachineGuidMD5, md5(LokiBot.szMachineGuid, strlen(LokiBot.szMachineGuid)));

    char szFolder[7] = {};
    char szFilename[7] = {};

    for (int i = 0; i < 6; i++) {
        memcpy(szFolder + i, &LokiBot.szMachineGuidMD5[7 + i], 1);
    }
    szFolder[6] = 0;

    for (int i = 0; i < 6; i++) {
        memcpy(szFilename + i, &LokiBot.szMachineGuidMD5[12 + i], 1);
    }
    szFilename[6] = 0;

    StringCchCatA(LokiBot.szFolder, MAX_PATH, szFolder);
    StringCchCatA(LokiBot.szFilename, MAX_PATH, szFilename);

    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, LokiBot.szPersistenceFolder);
    StringCchCatA(LokiBot.szPersistenceFolder, MAX_PATH, "\\");
    StringCchCatA(LokiBot.szPersistenceFolder, MAX_PATH, szFolder);

    strcpy(LokiBot.szExe, LokiBot.szPersistenceFolder);
    StringCchCatA(LokiBot.szExe, MAX_PATH, "\\");
    StringCchCatA(LokiBot.szExe, MAX_PATH, szFilename);
    StringCchCatA(LokiBot.szExe, MAX_PATH, ".exe");

    strcpy(LokiBot.szHdb, LokiBot.szPersistenceFolder);
    StringCchCatA(LokiBot.szHdb, MAX_PATH, "\\");
    StringCchCatA(LokiBot.szHdb, MAX_PATH, szFilename);
    StringCchCatA(LokiBot.szHdb, MAX_PATH, ".hdb");

    strcpy(LokiBot.szLck, LokiBot.szPersistenceFolder);
    StringCchCatA(LokiBot.szLck, MAX_PATH, "\\");
    StringCchCatA(LokiBot.szLck, MAX_PATH, szFilename);
    StringCchCatA(LokiBot.szLck, MAX_PATH, ".lck");

    strcpy(LokiBot.szKdb, LokiBot.szPersistenceFolder);
    StringCchCatA(LokiBot.szKdb, MAX_PATH, "\\");
    StringCchCatA(LokiBot.szKdb, MAX_PATH, szFilename);
    StringCchCatA(LokiBot.szKdb, MAX_PATH, ".kdb");

    strcpy(LokiBot.szRegKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    
    return &LokiBot;
}

int main()
{
    int isLokiBot = 0;
    LOKIBOTPERSISTENCE* LokiBot = loadLokiBot();

    printf("[INFO] MachineGuid:\n\t+ %s\n", LokiBot->szMachineGuid);
    printf("[INFO] MD5 hash of MachineGuid:\n\t+ %s\n", LokiBot->szMachineGuidMD5);
    printf("[INFO] Lokibot folder for this machine: \n");
    printf("\t+ %s\n", LokiBot->szPersistenceFolder);

    isLokiBot = isThereLokibot(LokiBot);

    if (isLokiBot & LOKIBOTFILES) {
        printf("[WARNING] Lokibot files found.\n");
        removeDir(LokiBot->szPersistenceFolder);
    }
    if (isLokiBot & LOKIBOTREGK) {
        printf("[WARNING] Lokibot registry found.\n");
        removeRegKeys(LokiBot);
    }
    if (isLokiBot == 0) {
        printf("[INFO] Lokibot IOC's not found.\n");
    }

    getchar();
    return 0;
}
