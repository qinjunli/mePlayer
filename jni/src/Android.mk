LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

FFMPEG_PATH= ../ffmpeg

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

LOCAL_C_INCLUDES +=$(LOCAL_PATH)/$(FFMPEG_PATH)
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/$(FFMPEG_PATH)/libavcodec
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/$(FFMPEG_PATH)/libavutil
LOCAL_C_INCLUDES +=$(LOCAL_PATH)/$(FFMPEG_PATH)/libavformat



#LOCAL_CFLAGS+=-D_FILE_OFFSET_BITS=64

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.cpp \
main.c

LOCAL_SHARED_LIBRARIES += SDL2 ffmpeg

LOCAL_LDLIBS += -lGLESv1_CM -llog

include $(BUILD_SHARED_LIBRARY)
