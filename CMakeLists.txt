cmake_minimum_required(VERSION 3.15)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_add_compile_definitions(-D SHELL_THREAD_PRIO=20)
sdk_add_include_directories(.)
sdk_set_main_file(main.c)

if(CONFIG_CHERRYUSB_HOST)
    target_sources(app PRIVATE ./usb_hid_bridge.c)
endif()

project(usb_keyboard_agent)
