idf_component_register(SRCS "connect_wifi.h" "main.cpp"
                            "app.cpp" "app.h"
                            "../components/LoRa/lora.c" "../components/LoRa/lora.h" "../components/LoRa/loraRegs.h"
                    INCLUDE_DIRS "."
                                "../components/LoRa/"
                                "../components/arduino/")
