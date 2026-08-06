#---------------------------------------------------------------------------------
# CLEAR RULES
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set the DEVKITARM environment variable in your environment.")
endif

include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET CONFIGURATION
#---------------------------------------------------------------------------------
TARGET        := PokemonMega
BUILD         := build
SOURCES       := source
DATA          := data
INCLUDES      := include build
ROMFS         := romfs

APP_TITLE       := Pokémon Mega Regions
APP_DESCRIPTION := Automated Asset Build
APP_AUTHOR      := Zayl1n168

# FIXED: Linked -lcitro2d before -lcitro3d since Citro2D relies on it
LIBS    := -lcitro2d -lcitro3d -lctru -lm

ifneq ($(ROMFS),)
    export APP_ROMFS := $(CURDIR)/$(ROMFS)
endif

export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                   -I$(DEVKITPRO)/libctru/include

# FIXED: Added -L search paths for libcitro2d library objects explicitly
export LIBDIRS  := -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/libcitro2d/lib -L$(DEVKITPRO)/libcitro3d/lib

#---------------------------------------------------------------------------------
# COMPILER PATH AND FLAG SETTING MATRICES
#---------------------------------------------------------------------------------
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp
CFLAGS  := -g -Wall -O2 -mword-relocations -fomit-frame-pointer -ffunction-sections $(ARCH)
export CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11
export ASFLAGS  := -g $(ARCH)
export LDFLAGS  := -g $(ARCH) -Wl,-Map,$(BUILD)/$(TARGET).map -specs=3dsx.specs

#---------------------------------------------------------------------------------
# TARGET EXECUTION COMPILATION ROUTINES
#---------------------------------------------------------------------------------
.PHONY: all clean

all: $(BUILD) $(BUILD)/vshader_shbin.h $(TARGET).3dsx

$(BUILD):
	@mkdir -p $(BUILD)

# Safe wrapper to prevent "Cannot open SMDH file!" errors
$(TARGET).3dsx: $(BUILD)/$(TARGET).elf
	@echo "Generating 3DSX executable binary payload..."
	@if [ -f "icon.png" ]; then \
		smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" icon.png $(BUILD)/$(TARGET).smdh 2>/dev/null; \
		3dsxtool $(BUILD)/$(TARGET).elf $(TARGET).3dsx --smdh=$(BUILD)/$(TARGET).smdh $(if $(APP_ROMFS),--romfs=$(APP_ROMFS)); \
	else \
		3dsxtool $(BUILD)/$(TARGET).elf $(TARGET).3dsx $(if $(APP_ROMFS),--romfs=$(APP_ROMFS)); \
	fi

# Explicitly link your C++ objects AND the compiled shader object file
$(BUILD)/$(TARGET).elf: $(BUILD)/intro.o $(BUILD)/main.o $(BUILD)/overworld.o $(BUILD)/vshader_shbin.o
	@echo "Linking PokemonMega.elf..."
	@arm-none-eabi-g++ $(LDFLAGS) -o $@ $^ $(LIBDIRS) $(LIBS)

# Compile the .vsh file into a .shbin file, and generate the C++ header
$(BUILD)/vshader_shbin.h: vshader.vsh | $(BUILD)
	@echo "Compiling system shader assets..."
	@picasso -o $(BUILD)/vshader.shbin $<
	@echo "extern const u8 vshader_shbin[];" > $(BUILD)/vshader_shbin.h
	@echo "extern const u32 vshader_shbin_size;" >> $(BUILD)/vshader_shbin.h

# FIXED: Passing raw vshader.shbin to bin2s to produce matching vshader_shbin text definitions
$(BUILD)/vshader_shbin.o: $(BUILD)/vshader_shbin.h
	@echo "Assembling shader binary into object format..."
	@cd $(BUILD) && bin2s -a 4 vshader.shbin | arm-none-eabi-as -o vshader_shbin.o

# Compilation rules for C++ source items
$(BUILD)/intro.o: $(SOURCES)/intro.cpp $(BUILD)/vshader_shbin.h | $(BUILD)
	@echo "Compiling source/intro.cpp..."
	@arm-none-eabi-g++ $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(BUILD)/main.o: $(SOURCES)/main.cpp $(BUILD)/vshader_shbin.h | $(BUILD)
	@echo "Compiling source/main.cpp..."
	@arm-none-eabi-g++ $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(BUILD)/overworld.o: $(SOURCES)/overworld.cpp $(BUILD)/vshader_shbin.h | $(BUILD)
	@echo "Compiling source/overworld.cpp..."
	@arm-none-eabi-g++ $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	@echo "Cleaning workspace modules..."
	@rm -rf $(BUILD) $(TARGET).3dsx $(TARGET).elf
