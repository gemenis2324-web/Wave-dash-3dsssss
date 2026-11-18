TARGET		:= WaveDash3DS
BUILD		:= build
SOURCES		:= source
DATA		:= data
INCLUDES	:= include

APP_TITLE	:= Wave Dash 3DS
APP_DESCRIPTION	:= Geometry Dash Wave Clone
APP_AUTHOR	:= HomebrewDev
APP_UNIQUE_ID	:= 0xWD3D

include $(DEVKITPRO)/libctru/base_rules

ARCH		:= -march=armv6k -mtune=mpcore -mfloat-abi=hard
CFLAGS		:= -g -Wall -O3 -mword-relocations $(ARCH) -ffunction-sections $(INCLUDE)
CXXFLAGS	:= $(CFLAGS) -std=gnu++17 -fno-rtti -fno-exceptions
LDFLAGS		:= -specs=3dsx.specs $(ARCH) -Wl,--gc-sections
LIBS		:= -lcitro2d -lcitro3d -lctru -lm

CFILES		:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
OFILES		:= $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)

VPATH		:= $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

.PHONY: all clean cia

all: $(TARGET).3dsx

cia: $(TARGET).cia

$(TARGET).elf: $(OFILES)
	$(CXX) $(LDFLAGS) $(OFILES) $(LIBS) -o $@

$(TARGET).3dsx: $(TARGET).elf
	3dsxtool $< $@

$(TARGET).smdh:
	smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(DEVKITPRO)/libctru/default_icon.png $@

$(TARGET).cia: $(TARGET).elf $(TARGET).smdh
	makerom -f cia -o $@ -elf $(TARGET).elf -rsf $(DEVKITPRO)/libctru/cia.rsf -icon $(TARGET).smdh -DAPP_TITLE="$(APP_TITLE)" -DAPP_PRODUCT_CODE="CTR-P-WAVD" -DAPP_UNIQUE_ID=$(APP_UNIQUE_ID)

clean:
	rm -rf $(OFILES) $(TARGET).elf $(TARGET).3dsx $(TARGET).smdh $(TARGET).cia
