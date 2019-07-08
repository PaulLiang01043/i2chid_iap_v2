# i2chid_iap_v2
Elan Touchscreen Firmware Update Tool (I2C-HID Interface)
---
    Get ELAN Touchscreen Firmware Information & Update Firmware.

Compilation
--- 
    make: to build the exectue project "i2chid_iap_v2".
    $ make
   
Run
---
Get Firmware Information :

    ./i2chid_iap_v2 -P {hid_pid} -i
ex:

    ./i2chid_iap_v2 -P 2a03 -i

Update Firmware :

    ./i2chid_iap_v2 -P {hid_pid} -f {firmware_file}

ex:

    ./i2chid_iap_v2 -P 2a03 -f /tmp/elants_i2c_2a03.bin

Calibrate Touchscreen :

    ./i2chid_iap_v2 -P {hid_pid} -k

ex: 

    ./i2chid_iap_v2 -P 2a03 -k

Configure Log File Name :

    ./i2chid_iap_v2 -P {hid_pid} -f {firmware_file} -l {result_log_file}

ex: 

    ./i2chid_iap_v2 -P 2a03 -f /tmp/elants_i2c_2a03.bin -l result.txt

Enable Silent Mode :

    ./i2chid_iap_v2 -P {hid_pid} -f {firmware_file} -s

ex: 

    ./i2chid_iap_v2 -P 2a03 -f /tmp/elants_i2c_2a03.bin -s

