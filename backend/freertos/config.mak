
INCLUDES += -I$(CARBON_PATH)/backend/freertos/Source/include \
    -I$(CARBON_PATH)/backend/freertos/Source \
    -I$(CARBON_PATH)/backend/freertos/Source/portable/GCC/ARM_CM0

CFLAGS += -D__freertos__=1
CFLAGS_embed += -mcpu=cortex-m3 -mthumb 

#LIBS += -lfreertos
LDFLAGS += -L$(CARBON_PATH)/backend/freertos/Source
