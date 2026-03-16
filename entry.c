#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>
#include <Guid/GlobalVariable.h>


static EFI_GET_VARIABLE       gOrigGetVariable = NULL;
static EFI_HANDLE             gThisImage = NULL;
static EFI_HANDLE             gThisDevice = NULL;

static
EFI_STATUS
EFIAPI
WsGetVariable(
    IN     CHAR16   *Name,
    IN     EFI_GUID *Guid,
    OUT    UINT32   *Attr OPTIONAL,
    IN OUT UINTN    *Size,
    OUT    VOID     *Data OPTIONAL
    )
{
    if (Name && Guid &&
        StrCmp(Name, L"SecureBoot") == 0 &&
        CompareGuid(Guid, &gEfiGlobalVariableGuid))
    {
        if (!Data || *Size < 1) {
            *Size = 1;
            return EFI_BUFFER_TOO_SMALL;
        }
        *(UINT8*)Data = 1;
        *Size = 1;
        if (Attr)
            *Attr = EFI_VARIABLE_BOOTSERVICE_ACCESS |
                    EFI_VARIABLE_RUNTIME_ACCESS;
        return EFI_SUCCESS;
    }
    return gOrigGetVariable(Name, Guid, Attr, Size, Data);
}

EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable
    )
{
    EFI_STATUS st;

    gST->ConOut->ClearScreen(gST->ConOut);

    Print(L"bypass starting\r\n");

    gThisImage = ImageHandle;

    EFI_LOADED_IMAGE_PROTOCOL *li = NULL;
    st = gBS->HandleProtocol(ImageHandle,
        &gEfiLoadedImageProtocolGuid, (VOID**)&li);
    if (EFI_ERROR(st)) {
        Print(L"[-] LoadedImage fail\r\n");
        gBS->Stall(3000000);
        return st;
    }

    gThisDevice = li->DeviceHandle;

    gOrigGetVariable = gRT->GetVariable;
    gRT->GetVariable = WsGetVariable;
    Print(L"[+] SecureBoot=1\r\n");

    EFI_DEVICE_PATH_PROTOCOL *path = FileDevicePath(
        gThisDevice,
        L"\\EFI\\Microsoft\\Boot\\bootmgfw_backup.efi"
    );

    if (!path) {
        Print(L"[-] DevicePath fail\r\n");
        gBS->Stall(3000000);
        return EFI_NOT_FOUND;
    }

    Print(L"[+] loading bootmgfw\r\n");

    EFI_HANDLE bh = NULL;
    st = gBS->LoadImage(TRUE, ImageHandle, path, NULL, 0, &bh);
    FreePool(path);

    if (EFI_ERROR(st)) {
        Print(L"[-] LoadImage: %r\r\n", st);
        gBS->Stall(3000000);
        return st;
    }

    Print(L"[+] starting\r\n");
    gBS->Stall(500000);

    UINTN eds = 0;
    st = gBS->StartImage(bh, &eds, NULL);

    return st;
}
