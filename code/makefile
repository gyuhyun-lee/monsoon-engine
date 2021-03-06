COMPILER = clang 
ARCHITECTURE = -arch arm64
MAKEFLAGS += --silent

# -O0 = unoptimized, -O2 = compiler optimized
COMPILER_FLAGS = -g -Wall -O0 -std=c++11 -D MONSOON_DEBUG=1 -D COMPILER_LLVM=1 -D ARCH_ARM=1 -D VK_LAYER_PATH=/Volumes/vulkan/monsoon_renderer/external_libraries/VulkanSDK/macOS/share/vulkan/explicit_layer.d -D VK_ICD_FILENAMES=/Volumes/vulkan/monsoon_renderer/external_libraries/VulkanSDK/macOS/share/vulkan/icd.d/MoltenVK_icd.json
# to disable warning, prefix the name of the warning with no-
COMPILER_IGNORE_WARNINGS = -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-writable-strings -Wno-switch -Wno-objc-missing-super-calls -Wno-missing-braces
UNIVERSAL_CODE_PATH = ../code

MACOS_BUILD_PATH = ../../build
MACOS_APP_NAME = fox.app
MACOS_EXE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS
MACOS_RESOURCE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
MACOS_FRAMEWORK = -framework Cocoa -framework CoreFoundation -framework CoreAudio -framework AudioUnit -framework CoreVideo -framework IOKit -framework AppKit -framework OpenGL -framework GameController

EXTERNAL_LIBRARIES_PATH = ../external_libraries/VulkanSDK/macOS/lib
EXTERNAL_LIBRARIES = -lvulkan.1.2.182 -lvulkan.1

all : make_directory make_app create_lock monsoon.dylib delete_lock fox.app clean
#all : make_directory make_app fox.dylib fox.app clean

make_directory : 
	mkdir -p $(MACOS_BUILD_PATH)

make_app : 
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS

create_lock :
	touch $(MACOS_RESOURCE_PATH)/monsoon.lock
monsoon.dylib : $(UNIVERSAL_CODE_PATH)/monsoon.cpp
	$(COMPILER) $(ARCHITECTURE) $(COMPILER_FLAGS) -dynamiclib $(COMPILER_IGNORE_WARNINGS) -o $(MACOS_RESOURCE_PATH)/monsoon.dylib $(UNIVERSAL_CODE_PATH)/monsoon.cpp
delete_lock :
	rm $(MACOS_RESOURCE_PATH)/monsoon.lock

# target:  dependency1 dependency2 ...
#       <tab> command
fox.app : $(UNIVERSAL_CODE_PATH)/macos_main.mm
	$(COMPILER) $(ARCHITECTURE) $(COMPILER_FLAGS) $(COMPILER_IGNORE_WARNINGS) -L$(EXTERNAL_LIBRARIES_PATH) -lc++ $(MACOS_FRAMEWORK) -o $(MACOS_EXE_PATH)/fox $(UNIVERSAL_CODE_PATH)/macos_main.mm $(EXTERNAL_LIBRARIES)

#clean all the object files.
clean : 
	rm -rf *.o 
