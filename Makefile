TARGET		:= WaveDash3DS
BUILD		:= build
SOURCES		:= source
DATA		:= data
INCLUDES	:= include
ICON		:= icon.png
BANNER		:= banner.png
TITLE		:= Wave Dash 3DS
AUTHOR		:= Homebrew Dev
VERSION		:= 1.0.0

include $(DEVKITPRO)/libctru/switch_rules

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard
CFLAGS		:= -g -Wall -O3 -mword-relocations $(ARCH) -ffunction-sections $(INCLUDE)
CXXFLAGS	:= $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions
LDFLAGS		:= -specs=3dsx.specs $(ARCH) -Wl,--gc-sections

LIBS		:= -lcitro2d -lcitro3d -lctru -lm

# Reglas automáticas para generar CIA y 3DSX (requiere makerom y bannertool en el contenedor)
