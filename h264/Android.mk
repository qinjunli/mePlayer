LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := H264Android
LOCAL_SRC_FILES := dsputil.c error_resilience.c golomb.c h264.c h264_cabac.c h264_cavlc.c h264_direct.c h264_loopfilter.c h264_ps.c h264_refs.c h264dsp.c h264idct.c h264pred.c imgutils.c log.c mem.c mpegvideo.c utils.c bBuffer.c
LOCAL_LDLIBS := -lc
include $(BUILD_SHARED_LIBRARY)