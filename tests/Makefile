#!/usr/bin/make -f
# Makefile for DGL #
# ---------------- #
# Created by falkTX
#

include ../Makefile.base.mk

# ---------------------------------------------------------------------------------------------------------------------

BUILD_C_FLAGS   += $(DGL_FLAGS) -I..
BUILD_CXX_FLAGS += $(DGL_FLAGS) -I.. -I../dgl/src/pugl-upstream/include -DDONT_SET_USING_DGL_NAMESPACE
LINK_FLAGS      += -lpthread

# TODO fix within pugl
BUILD_CXX_FLAGS += -Wno-extra -Wno-missing-field-initializers

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -ObjC++ -DGL_SILENCE_DEPRECATION -Wno-deprecated-declarations
endif

# ---------------------------------------------------------------------------------------------------------------------

MANUAL_TESTS  =
UNIT_TESTS    = Color Point TestTelemetry TestBlockSlicing TestCLAPExtensions TestVST3Units TestPresetManager TestDoublePrecision TestCLAPPresetDiscovery TestNoteExpressions TestCLAPNoteNames TestMIDI2AndAudioPortsConfig TestVST3ContextMenuAndPrograms

ifeq ($(HAVE_CAIRO),true)
MANUAL_TESTS += Demo.cairo
endif

ifeq ($(HAVE_OPENGL),true)
MANUAL_TESTS += Demo.opengl
MANUAL_TESTS += FileBrowserDialog
MANUAL_TESTS += NanoImage
MANUAL_TESTS += NanoSubWidgets
endif

ifneq ($(WASM),true)
UNIT_TESTS   += Application
ifeq ($(HAVE_CAIRO),true)
UNIT_TESTS   += Window.cairo
endif
ifeq ($(HAVE_OPENGL),true)
UNIT_TESTS   += Window.opengl
UNIT_TESTS   += TestVectorWidgets
UNIT_TESTS   += TestVectorScopeAndTabs
UNIT_TESTS   += TestVectorSpectrum
UNIT_TESTS   += TestThreadPoolAndA11y
UNIT_TESTS   += TestXYPadAndToggleSwitch
UNIT_TESTS   += TestEnvelopeAndContextMenu
UNIT_TESTS   += TestVectorKeyboard
ifeq ($(MACOS),true)
UNIT_TESTS   += TestNativeAccessibility
UNIT_TESTS   += TestAUv3
endif
endif
ifeq ($(HAVE_STUB),true)
UNIT_TESTS   += Window.stub
endif
ifeq ($(HAVE_VULKAN),true)
UNIT_TESTS   += Window.vulkan
endif
ifeq ($(HAVE_METAL),true)
UNIT_TESTS   += Window.metal
endif
endif

MANUAL_TARGETS = $(MANUAL_TESTS:%=../build/tests/%$(APP_EXT))
UNIT_TARGET    = $(UNIT_TESTS:%=../build/tests/%$(APP_EXT))

ALL_OBJS  = $(MANUAL_TESTS:%=../build/tests/%.cpp.o)
ALL_OBJS += $(UNIT_TESTS:%=../build/tests/%.cpp.o)

# ---------------------------------------------------------------------------------------------------------------------

all: $(MANUAL_TARGETS) $(UNIT_TARGET)

# ---------------------------------------------------------------------------------------------------------------------

Demo.opengl: ../build/tests/Demo.opengl$(APP_EXT)
FileBrowserDialog: ../build/tests/FileBrowserDialog$(APP_EXT)
NanoImage: ../build/tests/NanoImage$(APP_EXT)
NanoSubWidgets: ../build/tests/NanoSubWidgets$(APP_EXT)

# ---------------------------------------------------------------------------------------------------------------------

define RUN_TEST

	${1}
endef

# 	valgrind --leak-check=full $@

run: $(UNIT_TARGET)
	$(foreach TEST,$^,$(call RUN_TEST,$(TEST)))

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -rf ../build/tests

# ---------------------------------------------------------------------------------------------------------------------
# building steps

../build/tests/%.c.o: %.c
	-@mkdir -p ../build/tests
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

../build/tests/%.cpp.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../build/tests/%.cpp.cairo.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Cairo)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(CAIRO_FLAGS) -DDGL_CAIRO -c -o $@

../build/tests/%.cpp.opengl.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (OpenGL)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

../build/tests/%.cpp.stub.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Stub)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

../build/tests/%.cpp.vulkan.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Vulkan)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_VULKAN -c -o $@

../build/tests/%.cpp.metal.o: %.cpp
	-@mkdir -p ../build/tests
	@echo "Compiling $< (Metal)"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(METAL_FLAGS) -DDGL_METAL -c -o $@

../build/tests/%.mm.o: %.mm
	-@mkdir -p ../build/tests
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) $(OPENGL_FLAGS) -DDGL_OPENGL -c -o $@

# ---------------------------------------------------------------------------------------------------------------------
# linking steps

../build/tests/%$(APP_EXT): ../build/tests/%.cpp.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) -o $@

../build/tests/%.cairo$(APP_EXT): ../build/tests/%.cpp.cairo.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(CAIRO_LIBS) -o $@

../build/tests/%.opengl$(APP_EXT): ../build/tests/%.cpp.opengl.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/%.stub$(APP_EXT): ../build/tests/%.cpp.stub.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) -o $@

../build/tests/%.vulkan$(APP_EXT): ../build/tests/%.cpp.vulkan.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(VULKAN_LIBS) -o $@

../build/tests/%.metal$(APP_EXT): ../build/tests/%.cpp.metal.o
	@echo "Linking $*"
	$(SILENT)$(CXX) $< $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(METAL_LIBS) -o $@

# ---------------------------------------------------------------------------------------------------------------------
# linking steps (special, links against DGL static lib)

../build/tests/Demo.cairo$(APP_EXT): ../build/tests/Demo.cpp.cairo.o ../build/libdgl-cairo.a
	@echo "Linking Demo (Cairo)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(CAIRO_LIBS) -o $@

../build/tests/Demo.opengl$(APP_EXT): ../build/tests/Demo.cpp.opengl.o ../build/libdgl-opengl.a
	@echo "Linking Demo (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/Demo.vulkan$(APP_EXT): ../build/tests/Demo.cpp.vulkan.o ../build/libdgl-vulkan.a
	@echo "Linking Demo (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(VULKAN_LIBS) -o $@

../build/tests/FileBrowserDialog$(APP_EXT): ../build/tests/FileBrowserDialog.cpp.o ../build/libdgl-opengl.a
	@echo "Linking FileBrowserDialog (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/NanoImage$(APP_EXT): ../build/tests/NanoImage.cpp.o ../build/libdgl-opengl.a
	@echo "Linking NanoImage (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/NanoSubWidgets$(APP_EXT): ../build/tests/NanoSubWidgets.cpp.o ../build/libdgl-opengl.a
	@echo "Linking NanoSubWidgets (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestVectorWidgets$(APP_EXT): ../build/tests/TestVectorWidgets.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestVectorWidgets (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestVectorScopeAndTabs$(APP_EXT): ../build/tests/TestVectorScopeAndTabs.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestVectorScopeAndTabs (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestVectorSpectrum$(APP_EXT): ../build/tests/TestVectorSpectrum.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestVectorSpectrum (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestThreadPoolAndA11y$(APP_EXT): ../build/tests/TestThreadPoolAndA11y.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestThreadPoolAndA11y (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestXYPadAndToggleSwitch$(APP_EXT): ../build/tests/TestXYPadAndToggleSwitch.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestXYPadAndToggleSwitch (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestEnvelopeAndContextMenu$(APP_EXT): ../build/tests/TestEnvelopeAndContextMenu.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestEnvelopeAndContextMenu (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestVectorKeyboard$(APP_EXT): ../build/tests/TestVectorKeyboard.cpp.o ../build/libdgl-opengl.a
	@echo "Linking TestVectorKeyboard (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -o $@

../build/tests/TestNativeAccessibility$(APP_EXT): ../build/tests/TestNativeAccessibility.mm.o ../build/libdgl-opengl.a
	@echo "Linking TestNativeAccessibility (OpenGL)"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) $(DGL_SYSTEM_LIBS) $(OPENGL_LIBS) -framework Cocoa -o $@

../build/tests/TestAUv3$(APP_EXT): ../build/tests/TestAUv3.mm.o
	@echo "Linking TestAUv3"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) -framework AudioToolbox -framework CoreAudioKit -framework AVFoundation -framework Cocoa -o $@

# ---------------------------------------------------------------------------------------------------------------------

.PHONY: Demo.opengl FileBrowserDialog NanoImage NanoSubWidgets TestVectorWidgets TestVectorScopeAndTabs TestVectorSpectrum TestThreadPoolAndA11y TestXYPadAndToggleSwitch TestEnvelopeAndContextMenu TestVectorKeyboard TestNativeAccessibility TestAUv3

-include $(ALL_OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
