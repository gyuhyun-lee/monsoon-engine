COMPILER = clang
MAKEFLAGS += --silent

COMPILER_FLAGS = -g -Wall -O0 -std=c++11
# to disable warning, prefix the name of the warning with no-
COMPILER_IGNORE_WARNINGS = -Wno-unused-variable -Wno-unused-function -Wno-deprecated-declarations -Wno-writable-strings -Wno-switch -Wno-objc-missing-super-calls
UNIVERSAL_CODE_PATH = ../code

MACOS_CODE_PATH = ../platform_macos
MACOS_BUILD_PATH = ../build
MACOS_APP_NAME = fox.app
MACOS_EXE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS
MACOS_RESOURCE_PATH = $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
MACOS_FRAMEWORK = -framework Cocoa -framework CoreFoundation -framework CoreAudio -framework AudioUnit -framework CoreVideo -framework IOKit -framework AppKit -framework OpenGL

all : make_directory make_app fox.app clean
#all : make_directory make_app fox.dylib fox.app clean

make_directory : 
	mkdir -p $(MACOS_BUILD_PATH)

make_app : 
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/Resources
	mkdir -p $(MACOS_BUILD_PATH)/$(MACOS_APP_NAME)/Contents/MacOS

# fox.dylib : $(UNIVERSAL_CODE_PATH)/fox.cpp
#	$(COMPILER) $(COMPILER_FLAGS) -dynamiclib $(COMPILER_IGNORE_WARNINGS) -o $(MACOS_RESOURCE_PATH)/fox.dylib $(UNIVERSAL_CODE_PATH)/fox.cpp

# target:  dependency1 dependency2 ...
#       <tab> command
fox.app : $(MACOS_CODE_PATH)/macos_main.mm
	$(COMPILER) $(COMPILER_FLAGS) $(COMPILER_IGNORE_WARNINGS) $(MACOS_FRAMEWORK) -o $(MACOS_EXE_PATH)/fox $(MACOS_CODE_PATH)/macos_main.mm

#clean all the object files.
clean : 
	rm -rf *.o 
