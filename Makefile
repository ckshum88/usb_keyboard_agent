SDK_DEMO_PATH ?= $(abspath .)
BL_SDK_BASE ?= /opt/bouffalo_sdk

export BL_SDK_BASE



include $(BL_SDK_BASE)/project.build
