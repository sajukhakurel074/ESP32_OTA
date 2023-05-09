# ESP32_OTA

This is a complete ESP32 program, in esp32idf for OTA updates. There are seperate files for the source of the new firmware to be installed.

ESP32 provides multiple memory partition for the installation of new firmware. By default the single partition configurations is set. 
For the programs less than 1MB which is the default size of each partition,
“Factory app, two OTA definitions” has to be selected.
->mkkjjenucofig->Partition Table->Factory app, two OTA definitions->save
If your program size exceeds 1MB then the custom parition tables can be made. More dehjjkktails can be found at https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html