#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(TOPDIR)/switch32_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET		:=	subsdk
BUILD		:=	build
SOURCES 	:= 	source \
				source/forge \
				source/forge/graphics \
				libs/cimgui \
				libs/cimgui/imgui \
				libs/libstdcxx-pic

DATA		:=	data
INCLUDES	:=	include \
				libs/nnsdk/include \
				libs/iniparser/src \
				libs/libnx/nx/external/bsd/include \
				libs/nnheaders/include \
				libs/nnheaders/src/NintendoSDK \
				libs/cimgui/imgui

INIPARSER_DIR	:=	$(TOPDIR)/libs/iniparser/src
INIPARSER_BUILD	:=	$(TOPDIR)/$(BUILD)/iniparser
INIPARSER_LIB	:=	$(INIPARSER_BUILD)/libiniparser.a

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -mfloat-abi=softfp -mfpu=crypto-neon-fp-armv8 -fPIC -ftls-model=initial-exec -fno-short-enums

CFLAGS	:=	-g -Wall -Werror \
			-ffunction-sections \
			-fdata-sections \
			$(ARCH) \
			$(BUILD_CFLAGS)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DLIBNX_NO_DEPRECATION -DFORGE_INJECTED_NSO \
		-DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS -DIMGUI_DISABLE_TTY_FUNCTIONS

CXXFLAGS	:= $(CFLAGS) -std=c++23 -fno-rtti -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -enable-libstdcxx-allocator=new -fpermissive -fno-threadsafe-statics \
			-include $(TOPDIR)/include/compat.h \
			-Wno-deprecated-declarations \
			-fno-builtin-ceil -fno-builtin-ceilf -fno-builtin-floor -fno-builtin-floorf

ASFLAGS	:=	-g $(ARCH)

SDK_MATH := sinf logf powf cosf fmodf acosf floorf sqrtf ceilf log pow sqrt atan2f
LDFLAGS  =  -specs=$(TOPDIR)/switch32.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) -Wl,--version-script=$(TOPDIR)/exported.txt -Wl,-init=forge_init -Wl,-fini=forge_fini -Wl,--export-dynamic -nodefaultlibs -nostartfiles -Wl,-z,text $(foreach name,$(SDK_MATH),-Wl,--wrap=$(name))

LIBS	:= -liniparser -lgcc -u malloc -lnx32_min

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(TOPDIR)/libs/libnx/nx
LIBNX_MIN_LIB := $(TOPDIR)/libs/libnx/nx/lib/libnx32_min.a

SWITCH_TOOLS_SRC	:=	$(TOPDIR)/libs/switch-tools/src
PYTHON ?= python3
HOST_CC ?= gcc
ELF2NSO32			:=	$(TOPDIR)/tools/elf2nso32.py
NPDMTOOL			:=	$(TOPDIR)/tools/npdmtool

#---------------------------------------------------------------------------------
# version derived from latest git tag + short commit hash when not at tag
#---------------------------------------------------------------------------------
# Source archives and forks without release tags use this tree's upstream base.
GIT_TAG			:=	$(shell git describe --tags --abbrev=0 2>/dev/null || echo "v1.5.1")
GIT_COMMIT		:=	$(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_EXACT		:=	$(shell git describe --exact-match --tags HEAD >/dev/null 2>&1 && echo 1 || echo 0)

VERSION_TAG		:=	$(patsubst v%,%,$(GIT_TAG))
VERSION_MAJOR	:=	$(word 1,$(subst ., ,$(VERSION_TAG)))
VERSION_MINOR	:=	$(word 2,$(subst ., ,$(VERSION_TAG)))
VERSION_PATCH	:=	$(word 3,$(subst ., ,$(VERSION_TAG)))

ifeq ($(GIT_EXACT),1)
FORGE_VERSION	:=	$(VERSION_TAG)
else
FORGE_VERSION	:=	$(VERSION_TAG)-g$(GIT_COMMIT)
endif

VERSION_HEADER	:=	$(BUILD)/forge/version.h

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	?=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

# Remove cimgui backend wrappers
CPPFILES	:=	$(filter-out cimgui_impl.cpp,$(CPPFILES))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
			-L$(INIPARSER_BUILD)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

.PHONY: $(BUILD) clean all prepare-dependencies

#---------------------------------------------------------------------------------

all: $(BUILD) $(TOPDIR)/main.npdm

# This checked-in input must never be rebuilt through the generic %.npdm rule.
$(TOPDIR)/metadata/main-hardware.npdm: ;

prepare-dependencies:
	@$(PYTHON) $(TOPDIR)/tools/apply_hardware_patches.py

# Preserve the tested title metadata instead of regenerating unrelated fields.
$(TOPDIR)/main.npdm: $(TOPDIR)/metadata/main-hardware.npdm $(TOPDIR)/tools/prepare_npdm.py
	@$(PYTHON) $(TOPDIR)/tools/prepare_npdm.py

$(NPDMTOOL): $(SWITCH_TOOLS_SRC)/npdmtool.c $(SWITCH_TOOLS_SRC)/cJSON.c $(SWITCH_TOOLS_SRC)/cJSON.h
	@mkdir -p $(dir $@)
	@$(HOST_CC) -O2 -I$(SWITCH_TOOLS_SRC) -o $@ $(filter %.c,$^)
	@echo built ... $(notdir $@)

$(VERSION_HEADER): $(TOPDIR)/data/version.h.in
	@mkdir -p $(dir $@)
	@sed \
		-e 's|@FORGE_VERSION@|$(FORGE_VERSION)|g' \
		-e 's|@FORGE_VERSION_MAJOR@|$(VERSION_MAJOR)|g' \
		-e 's|@FORGE_VERSION_MINOR@|$(VERSION_MINOR)|g' \
		-e 's|@FORGE_VERSION_PATCH@|$(VERSION_PATCH)|g' \
		-e 's|@FORGE_VERSION_COMMIT@|$(GIT_COMMIT)|g' \
		-e 's|@FORGE_VERSION_IS_RELEASE@|$(GIT_EXACT)|g' \
		$< > $@
	@echo generated ... $(notdir $@)

$(BUILD): prepare-dependencies $(ELF2NSO32) $(VERSION_HEADER)
	@$(MAKE) -C $(CURDIR)/libs/libnx/nx -f $(CURDIR)/libs/libnx/nx/Makefile.32_min.mk
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@$(MAKE) -C $(CURDIR)/libs/libnx/nx -f $(CURDIR)/libs/libnx/nx/Makefile.32_min.mk clean
	@echo clean ...
	@rm -fr $(BUILD) *.nso *.elf *.npdm tools/npdmtool tools/npdmtool.exe

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)
INIPARSER_OBJS	:=	$(INIPARSER_BUILD)/iniparser.o $(INIPARSER_BUILD)/dictionary.o

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all	: $(OUTPUT).nso

$(OUTPUT).nso	:	$(OUTPUT).elf $(ELF2NSO32) $(TOPDIR)/tools/check_relocations.py

$(OUTPUT).elf	:	$(OFILES) $(INIPARSER_LIB) $(LIBNX_MIN_LIB) $(TOPDIR)/Makefile $(TOPDIR)/switch32.specs $(TOPDIR)/switch32.ld

# Built by the outer libnx make; do not recreate it with the generic %.a rule.
$(LIBNX_MIN_LIB): ;

$(OFILES_SRC)	: $(HFILES_BIN) $(TOPDIR)/$(BUILD)/forge/version.h

$(INIPARSER_LIB): $(INIPARSER_OBJS)
	@echo archiving $(notdir $@)
	@$(AR) rcs $@ $^

$(INIPARSER_BUILD)/%.o: $(INIPARSER_DIR)/%.c
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@echo $(notdir $<)
	@$(CC) $(filter-out -Werror,$(CFLAGS)) -I$(INIPARSER_DIR) -c $< -o $@

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
