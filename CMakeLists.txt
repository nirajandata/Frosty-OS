cmake_minimum_required(VERSION 4.0)

project(FrostyOS 
    VERSION 0.1.0 
    LANGUAGES CXX ASM
)

add_library(kernel_bare_metal INTERFACE)

target_compile_features(kernel_bare_metal INTERFACE cxx_std_26)

target_compile_options(kernel_bare_metal INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:
        -ffreestanding
        -fno-stack-protector -fno-stack-check -fno-lto
        -fno-pie -fno-pic
        -fno-rtti -fno-exceptions
        -mno-80387 -mno-mmx 
         -mno-sse -mno-sse2
        -mno-red-zone
        -mcmodel=kernel
        -Wall -Wextra -O2
    >
)

target_link_options(kernel_bare_metal INTERFACE
    -nostdlib
    -static
    -z max-page-size=0x1000
    -Wl,--build-id=none
)

include(FetchContent)

FetchContent_Declare(
    limine_repo
    URL "https://codeberg.org/Limine/limine-protocol/raw/branch/trunk/include/limine.h"
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(limine_repo)

add_library(limine_interface INTERFACE)
target_include_directories(limine_interface INTERFACE ${limine_repo_SOURCE_DIR})

file(GLOB_RECURSE KERNEL_SOURCES CONFIGURE_DEPENDS "kernel/*.cpp" "kernel/*.s")

add_executable(kernel.elf ${KERNEL_SOURCES})

target_link_libraries(kernel.elf 
    PRIVATE 
        kernel_bare_metal 
        limine_interface
)

target_link_options(kernel.elf PRIVATE 
    "LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/kernel/linker.ld"
    "LINKER:--entry=kmain"
)

set(ISO_NAME "frosty_os.iso")
set(ISO_ROOT "${CMAKE_CURRENT_BINARY_DIR}/iso_root")
set(LIMINE_DIR "/usr/share/limine") 

add_custom_command(
    OUTPUT ${ISO_NAME}
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${ISO_ROOT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ISO_ROOT}/boot/limine
    
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel.elf> ${ISO_ROOT}/boot/kernel.elf
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/limine.conf ${ISO_ROOT}/boot/limine/limine.conf
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios.sys ${ISO_ROOT}/boot/limine/
    COMMAND ${CMAKE_COMMAND} -E copy ${LIMINE_DIR}/limine-bios-cd.bin ${ISO_ROOT}/boot/limine/

    COMMAND xorriso -as mkisofs
        -b boot/limine/limine-bios-cd.bin
        -no-emul-boot -boot-load-size 4 -boot-info-table
        -o ${ISO_NAME}
        ${ISO_ROOT}
    
    DEPENDS kernel.elf ${CMAKE_CURRENT_SOURCE_DIR}/limine.conf
    COMMENT "Creating bootable ISO: ${ISO_NAME}"
    VERBATIM
)

add_custom_target(iso ALL DEPENDS ${ISO_NAME})

add_custom_target(run
    COMMAND qemu-system-x86_64 
        -cdrom ${ISO_NAME}
        -m 512M
        -serial stdio
        -boot d
        -device usb-ehci,id=usb 
        -device usb-tablet     
        -machine pcspk-audiodev=audio0
        -audiodev pa,id=audio0
    DEPENDS ${ISO_NAME}
    USES_TERMINAL
)
