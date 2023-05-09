idf_component_register(SRCS "main.c"
                            "../OTA/src/ESP_OTA.c"
                            "../Sim7600/src/sim7600.c"
                            "../BLE/src/ble.c"
                            "../Sd_Card/src/Sd_Card.c"
                    
                    INCLUDE_DIRS    "../OTA/inc" 
                                "../BLE/inc" 
                                "../Sim7600/inc" 
                                "../Sd_Card/inc"   
                                 "."                 
                    
)

