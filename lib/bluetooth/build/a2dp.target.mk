# This file is generated by gyp; do not edit.

TOOLSET := target
TARGET := a2dp
DEFS_Debug := \
	'-DNODE_GYP_MODULE_NAME=a2dp' \
	'-DUSING_UV_SHARED=1' \
	'-DUSING_V8_SHARED=1' \
	'-DV8_DEPRECATION_WARNINGS=1' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DLIB_EXPAT=expat' \
	'-DBUILDING_NODE_EXTENSION' \
	'-DDEBUG' \
	'-D_DEBUG'

# Flags passed to all source files.
CFLAGS_Debug := \
	-fPIC \
	-pthread \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-std=gnu++0x \
	-I/usr/include/alsa \
	-g \
	-O0

# Flags passed to only C files.
CFLAGS_C_Debug :=

# Flags passed to only C++ files.
CFLAGS_CC_Debug := \
	-fno-rtti \
	-fno-exceptions \
	-std=gnu++0x

INCS_Debug := \
	-I/home/pi/.node-gyp/7.1.0/include/node \
	-I/home/pi/.node-gyp/7.1.0/src \
	-I/home/pi/.node-gyp/7.1.0/deps/uv/include \
	-I/home/pi/.node-gyp/7.1.0/deps/v8/include \
	-I$(srcdir)/node_modules/nan

DEFS_Release := \
	'-DNODE_GYP_MODULE_NAME=a2dp' \
	'-DUSING_UV_SHARED=1' \
	'-DUSING_V8_SHARED=1' \
	'-DV8_DEPRECATION_WARNINGS=1' \
	'-D_LARGEFILE_SOURCE' \
	'-D_FILE_OFFSET_BITS=64' \
	'-DLIB_EXPAT=expat' \
	'-DBUILDING_NODE_EXTENSION'

# Flags passed to all source files.
CFLAGS_Release := \
	-fPIC \
	-pthread \
	-Wall \
	-Wextra \
	-Wno-unused-parameter \
	-std=gnu++0x \
	-I/usr/include/alsa \
	-O3 \
	-fno-omit-frame-pointer

# Flags passed to only C files.
CFLAGS_C_Release :=

# Flags passed to only C++ files.
CFLAGS_CC_Release := \
	-fno-rtti \
	-fno-exceptions \
	-std=gnu++0x

INCS_Release := \
	-I/home/pi/.node-gyp/7.1.0/include/node \
	-I/home/pi/.node-gyp/7.1.0/src \
	-I/home/pi/.node-gyp/7.1.0/deps/uv/include \
	-I/home/pi/.node-gyp/7.1.0/deps/v8/include \
	-I$(srcdir)/node_modules/nan

OBJS := \
	$(obj).target/$(TARGET)/src/a2dp.o

# Add to the list of files we specially track dependencies for.
all_deps += $(OBJS)

# CFLAGS et al overrides must be target-local.
# See "Target-specific Variable Values" in the GNU Make manual.
$(OBJS): TOOLSET := $(TOOLSET)
$(OBJS): GYP_CFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_C_$(BUILDTYPE))
$(OBJS): GYP_CXXFLAGS := $(DEFS_$(BUILDTYPE)) $(INCS_$(BUILDTYPE))  $(CFLAGS_$(BUILDTYPE)) $(CFLAGS_CC_$(BUILDTYPE))

# Suffix rules, putting all outputs into $(obj).

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(srcdir)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# Try building from generated source, too.

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj).$(TOOLSET)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

$(obj).$(TOOLSET)/$(TARGET)/%.o: $(obj)/%.cc FORCE_DO_CMD
	@$(call do_cmd,cxx,1)

# End of this set of suffix rules
### Rules for final target.
LDFLAGS_Debug := \
	-pthread \
	-rdynamic

LDFLAGS_Release := \
	-pthread \
	-rdynamic

LIBS := \
	-lsbc

$(obj).target/a2dp.node: GYP_LDFLAGS := $(LDFLAGS_$(BUILDTYPE))
$(obj).target/a2dp.node: LIBS := $(LIBS)
$(obj).target/a2dp.node: TOOLSET := $(TOOLSET)
$(obj).target/a2dp.node: $(OBJS) FORCE_DO_CMD
	$(call do_cmd,solink_module)

all_deps += $(obj).target/a2dp.node
# Add target alias
.PHONY: a2dp
a2dp: $(builddir)/a2dp.node

# Copy this to the executable output path.
$(builddir)/a2dp.node: TOOLSET := $(TOOLSET)
$(builddir)/a2dp.node: $(obj).target/a2dp.node FORCE_DO_CMD
	$(call do_cmd,copy)

all_deps += $(builddir)/a2dp.node
# Short alias for building this executable.
.PHONY: a2dp.node
a2dp.node: $(obj).target/a2dp.node $(builddir)/a2dp.node

# Add executable to "all" target.
.PHONY: all
all: $(builddir)/a2dp.node

