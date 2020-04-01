extern "C" {

	#include <efi.h>
	#include <efilib.h>

}

#include <cstdint>
#include <cstring>

EFI_STATUS cxx_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);

extern "C" {

	EFI_STATUS
	EFIAPI
	efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	   InitializeLib(ImageHandle, SystemTable);
	   SystemTable->BootServices->SetWatchdogTimer(0, 0, 0, nullptr); // Disable UEFI's timer which after ~5min would reset the computer.
	   return cxx_main(ImageHandle, SystemTable);
	}

}

uint16_t * screen = reinterpret_cast<uint16_t*>(0xb8000);

enum class Color {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    Pink = 13,
    Yellow = 14,
    White = 15,
};

EFI_SYSTEM_TABLE* systab;

void putstr(wchar_t * ptr) {
    uefi_call_wrapper((void*)systab->ConOut->OutputString, 2, systab->ConOut, ptr);
}

void setcolor(Color foreground = Color::White, Color background = Color::Black, std::size_t blink = 0) {
    std::size_t attr = blink << 7 | ((std::size_t)background & 0x7) << 4 | (std::size_t)foreground;
    uefi_call_wrapper((void*)systab->ConOut->SetAttribute, 2, systab->ConOut, attr);
}

void setpos(std::size_t x, std::size_t y) {
    uefi_call_wrapper((void*)systab->ConOut->SetCursorPosition, 3, systab->ConOut, x, y);
}

void sleep(std::size_t us) {
    uefi_call_wrapper((void*)BS->Stall, 1, us);
}

void setTextMode() {
    uefi_call_wrapper((void*)systab->ConOut->SetMode, 2, systab->ConOut, 0);
}

std::size_t rdtsc() {
    std::size_t high = 0;
    std::size_t low = 0;
    __asm__ ("rdtsc" : "=a"(low), "=d"(high));
    return high << 32 | low;
}

EFI_STATUS cxx_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    // Insert your C++ code here
    systab = SystemTable;
    
    setTextMode();

    while(1) {
        setcolor((Color)(rdtsc()&0x7));
        putstr(L"A");
        sleep(300'000);
    }

    return EFI_SUCCESS;
}
