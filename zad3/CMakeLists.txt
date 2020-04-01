cmake_minimum_required(VERSION 3.0)

project(efi-template)

include_directories("inc")
include_directories("inc/gnu-efi")
include_directories("inc/gnu-efi/x86_64")


add_library(app SHARED
    # add new sources files here
    "src/main.cc"
    )


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-masm=intel -fno-stack-protector -static -D_GLIBCXX_FULLY_DYNAMIC_STRING -fpic -fshort-wchar -Wall -Wextra -mno-red-zone -DEFI_FUNCTION_WRAPPER -ggdb -O0")

execute_process(COMMAND which ld OUTPUT_STRIP_TRAILING_WHITESPACE  OUTPUT_VARIABLE LINKER)

set(SKIP_BUILD_RPATH TRUE)
set(STATIC_LIB_LINK "-l:libefi.a -l:libgnuefi.a")
set(STATIC_LIB_PATH "${CMAKE_SOURCE_DIR}/lib")
set(CRT0_PATH "${CMAKE_SOURCE_DIR}/lib/crt0-efi-x86_64.o")
set(LINK_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/elf_x86_64_efi.lds")
set(LDFLAGS "-nostdlib -znocombreloc -Bsymbolic -shared -static")

set(CMAKE_CXX_CREATE_SHARED_LIBRARY "${LINKER} ${CRT0_PATH} <OBJECTS> ${LDFLAGS} -L${STATIC_LIB_PATH} ${STATIC_LIB_LINK} -T${LINK_SCRIPT} -o<TARGET>")

set(OBJCOPY_FLAGS -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64)
set(OBJCOPY_DEBUG_FLAGS -j .debug_info -j .debug_abbrev -j .debug_loc -j .debug_aranges -j .debug_line -j .debug_macinfo -j .debug_str ${OBJCOPY_FLAGS})
execute_process(COMMAND which objcopy OUTPUT_STRIP_TRAILING_WHITESPACE  OUTPUT_VARIABLE OBJCOPY)


set(OUTPUT_FILE_NAME "BOOTX64.efi")

add_custom_command(TARGET app POST_BUILD COMMAND ${OBJCOPY} ${OBJCOPY_FLAGS} $<TARGET_FILE:app> ${CMAKE_SOURCE_DIR}/${OUTPUT_FILE_NAME})
add_custom_command(TARGET app POST_BUILD COMMAND ${OBJCOPY} ${OBJCOPY_DEBUG_FLAGS} $<TARGET_FILE:app> ${CMAKE_SOURCE_DIR}/debug.${OUTPUT_FILE_NAME} )

set(DISK_NAME "disk.img")
set(DISK_IMAGE "${CMAKE_SOURCE_DIR}/${DISK_NAME}")
set(UEFI_PATH "${CMAKE_SOURCE_DIR}/bios/OVMF.fd")

add_custom_target(img DEPENDS app
        COMMAND dd if=/dev/zero of=${DISK_IMAGE} bs=512 count=93750
        COMMAND sudo parted ${DISK_IMAGE} -s -a minimal mklabel gpt
        COMMAND sudo parted ${DISK_IMAGE} -s -a minimal mkpart EFI FAT16 2048s 93716s
        COMMAND sudo parted ${DISK_IMAGE} -s -a minimal toggle 1 boot
        COMMAND dd if=/dev/zero of=part.img bs=512 count=91669
        COMMAND mformat -i part.img -h 32 -t 32 -n 64 -c 1
        COMMAND mcopy -i part.img ${CMAKE_SOURCE_DIR}/${OUTPUT_FILE_NAME} ::
        COMMAND echo ${OUTPUT_FILE_NAME} > startup.nsh
        COMMAND mcopy -i part.img startup.nsh ::
        COMMAND rm startup.nsh
        COMMAND dd if=part.img of=${DISK_IMAGE} bs=512 count=91669 seek=2048 conv=notrunc
        COMMAND rm part.img
    )

execute_process(COMMAND which qemu-system-x86_64 OUTPUT_STRIP_TRAILING_WHITESPACE  OUTPUT_VARIABLE QEMU)

add_custom_target(run DEPENDS img
    COMMAND ${QEMU} -drive file=${DISK_IMAGE} -enable-kvm -cpu host -bios ${UEFI_PATH} -s
    )
