idf_component_register(SRCS "main.cpp"
                            "app.cpp" "app.h"
                            "../components/LoRa/lora.c" "../components/LoRa/lora.h" "../components/LoRa/loraRegs.h"
                            "../components/arduino/libraries/WiFi/src/WiFi.h"
                    INCLUDE_DIRS "."
                                "../components/LoRa/"
                                "../components/arduino/")
