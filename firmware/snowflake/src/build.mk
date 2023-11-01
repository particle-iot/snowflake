# Place this custom makefile here: <project-path>/src/build.mk

INCLUDE_DIRS += $(SOURCE_PATH)/$(USRSRC)  # add user sources to include path

CFLAGS+= -DEIDSP_USE_CMSIS_DSP=1
CFLAGS+= -D__STATIC_FORCEINLINE="__attribute__((always_inline)) static inline"
CFLAGS+= -DEI_PORTING_PARTICLE=1
CFLAGS+= -DEIDSP_LOAD_CMSIS_DSP_SOURCES=1

# add C and CPP files - if USRSRC is not empty, then add a slash
CPPSRC += $(call target_files,$(USRSRC_SLASH),*.cpp)
CSRC += $(call target_files,$(USRSRC_SLASH),*.c)