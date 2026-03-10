IOT_Modem_HAL
=============

📡 A tiny Hardware Abstraction Layer for IoT cellular modems on Arduino.

This library removes the pain of handling modem hardware control such as:

⚡ Power ON sequence  
🔄 Reset sequence  
⏻ Power OFF  
📍 Status pin management  

Just configure the pins and call begin().  
No more copy-pasting modem power sequences across projects.

The goal is simple:

👉 Zero hassle modem startup.


Features
--------

✔ Simple modem hardware control  
✔ Clean C++ interface  
✔ Works with Arduino HardwareSerial  
✔ Easily extendable to new modem models  
✔ Designed to integrate with libraries like TinyGSM  


Supported Modems
----------------

Currently supported:

📶 SIM7600GH

The architecture makes it easy to add other modems.


Basic Idea
----------

Instead of writing modem power sequences every time:

    digitalWrite(PWRKEY, LOW);
    delay(500);
    digitalWrite(PWRKEY, HIGH);
    delay(12000);

You simply do:

    modem.begin(config);


Perfect for projects where you just want the modem to:

✔ turn on  
✔ reset  
✔ power off  
✔ report status  

without dealing with hardware timing every time.


Author
------

Francesco Simonetti


License
-------

MIT
