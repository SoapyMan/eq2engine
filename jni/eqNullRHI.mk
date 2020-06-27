##
## libeqNullRHI shared library
##
include $(CLEAR_VARS)

LOCAL_PATH				:= $(SRC_PATH)

LOCAL_MODULE    		:= eqNullRHI
LOCAL_MODULE_FILENAME	:= libeqNullRHI

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/public\
	$(LOCAL_PATH)/public/core \
	$(LOCAL_PATH)/public/platform\
	$(LOCAL_PATH)/public/materialsystem\
	$(LOCAL_PATH)/public/materialsystem/renderers\

LOCAL_SRC_FILES := \
	MaterialSystem/Renderers/Empty/emptylibrary.cpp	\
	MaterialSystem/Renderers/Shared/ShaderAPI_Base.cpp		\
	MaterialSystem/Renderers/Shared/CTexture.cpp

LOCAL_STATIC_LIBRARIES := coreLib prevLib jpeg eqCore

include $(BUILD_SHARED_LIBRARY)
