
INCLUDES += -I$(CARBON_PATH)/backend/tneo/tneo/src/core -I$(CARBON_PATH)/backend/tneo/tneo/src 
CFLAGS += -D__tneo__=1
CFLAGS_embed += -mcpu=cortex-m3 -mthumb 
