# Riecher
## motivation
While Puster focuses on cleaning indoor air, Riecher focuses on understanding it.
It was developed to better understand and improve indoor air quality. Key indicators such as temperature, humidity, particulate matter (PM2.5, PM10), VOCs, NOx, and CO₂ were identified as essential for both health and comfort.

Poor air quality can trigger allergies, for example from dust mites, and directly impact well-being and productivity. In particular, CO₂ concentration is closely linked to our ability to concentrate, a critical factor in work and learning environments.  

<img width="1000" alt="image" src="https://github.com/user-attachments/assets/455d1424-b163-4f68-aa66-4ee6ab986ae0" />

By monitoring these values in real time, the system provides insights that help create healthier, safer, and more focused living and working spaces.

## mechanicals
<img width="181" height="199" alt="image" src=images/sketch.JPG /> <img width="181" height="199" alt="image" src=images/section_analysis.png />
### parts list
- **1x base**  
  3D printed or machined. Holds the main assembly.  
  <img width="181" height="199" alt="image" src=images/base.png />  
- **1x cage**   
  Provides airflow and mechanical protection for the sensors.   
  <img width="181" height="199" alt="image" src=images/cage.png />  
- **1x lid**  
  screw on lid for easy maintenance.  
  <img width="181" height="199" alt="image" src=images/lid.png />  
- **1x ESP32 based pcb**   
  Integrates the SEN66 air quality sensor.  
  <img width="181" height="199" alt="image" src=images/pcb_physical.png />  
- **SEN66**  
  At the heart of this product sits the SEN66 AirQuality Sensor by Sensirion. This sensor gives us all the relevant metrics in one package.
  <img width="181" height="375" alt="sen6x" src="https://github.com/user-attachments/assets/7189b01e-96ab-4c20-9d19-9cb4cf91f958" />  
- **connection cable**  
  TODO (accurate bezeichnung)
- _x screws
  TODO (heat set inserts)

### manufacturing
The ***base*** was routed from wood using a cnc machine. This was a dual sided operation as it has features on both sides of the base. On the bottom there is a place for the cable to be routed which acts as a strain relief at the same time.

The ***cage*** was printed with Bambu Labs wood filament.'

## electronics
### the pcb 
you can order the pcb through the following link:  
https://aisler.net/p/XLGCRZIT  
i added a stencil to help me with assembly.

the ESP32-C6 was chosen as the core of the Riecher PCB because it offers modern wireless connectivity while staying flexible for different smart home ecosystems. with support for Wi-Fi 6, Bluetooth Low Energy 5.0, Zigbee and Thread (Matter), it can easily integrate into existing setups, whether that’s a Zigbee network, Matter-over-Thread, or simple Wi-Fi control. this makes the device adaptable for the future without locking it to one protocol.

I brought out the full GPIO Header known from the ESP32 Devkits which allows the Puster to be extended to your liking. 
 
<img alt="image" src="images/pcb_routing.png"/>   

