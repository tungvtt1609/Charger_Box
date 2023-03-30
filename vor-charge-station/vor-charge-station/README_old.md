# vor-charge-station

ver 1: This code subscribe data on the topic `charge_request` to check the request from robot while robot moving to charge station.

If the data = 1, vor charge station turn on the relay, switch power to charge robot. And vor charge station publish the topic `status` to advertise status of its (is charging or not).

ver 2: Using service server. Send a request from computer or robot to control ACS.

# Build and Flash

Generate the ROS libraries prior to building this example as instructed in the [README](../../../README.md) of root directory

Default mode of rosserial communication is over UART.

To use WiFi:

Go to vor_charge_station folder:

## 1. Enable rosserial over WiFi

`idf.py menuconfig` -> `Component config` -> `rosserial` ->`rosserial over WiFi using TCP`

Config SSID and Password of wifi.

Config IP of roscore.

## 2. Enter WiFi and server details

```
$ export ESPPORT=/dev/ttyUSB0
$ idf.py build flash
$ While terminal display: 
Serial port /dev/ttyUSB0
Connecting...................
Hold button FW update, after that press & release reset button to upload firmware
```

## 3. Config static IP for ESP32 (to use local network in VTCC):
Open file: ~/esp-idf/components/rosserial_esp32/esp_ros_wifi.c

## Method 1:
In function esp_ros_wifi_init(), add:
```
// ip config
    tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    tcpip_adapter_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 172, 16, 30, 191);   
   	IP4_ADDR(&ip_info.gw, 172, 16, 30, 1);
   	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);
    tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
// end ip config
```

## Method 2:
In file vor_charge_station/esp/sdkconfig, search rosserial and modify:
```
#
# rosserial
#
CONFIG_ROSSERIAL_OVER_WIFI=y
CONFIG_ROSSERVER_AP="Robotics"
CONFIG_ROSSERVER_PASS="Robotics@VTCC-2021"
CONFIG_ROSSERVER_IP="172.16.30.198"
CONFIG_ROSSERVER_PORT=11411
CONFIG_ROS_STATIC_IP_CONFIG=y
CONFIG_ROS_STATIC_IP_ESP="172.16.30.191"
CONFIG_ROS_GATE_WAY_ESP="172.16.30.1"
CONFIG_ROS_NETMASK_ESP="255.255.255.0"
# end of rosserial
```

# On a new terminal

```
$ roscore
```

# On another new terminal

## Connect with esp32 via TCP
Run this command:
```
$ rosrun rosserial_python serial_node.py tcp
```
Or create and using launch file (Recommended), respawn node if ESP reset:
```
<?xml version="1.0"?>
<launch>
    <node name="rosserial_tcp_esp32" pkg="rosserial_python" type="serial_node.py" 
            output="screen" args="tcp 11411" respawn="true"></node>
    <param name="port" value="tcp" />
</launch>
```

## Using topic to control ACS

```
On Charge:

$ rostopic pub -1 /charge_request std_msgs/Bool 1

Off Charge:

$ rostopic pub -1 /charge_request std_msgs/Bool 0

```
On another new terminal view status of charge station

```
$ rostopic echo /status
```

## Using rqt or cmd service call:
```
rqt:
Service /charge_server/on turn on the ACS
Service /charge_server/off turn off the ACS

cmd: 
rosservice call /charge_server/on 
rosservice call /charge_server/off 
```





## On off relay using function

```
relay_on(RELAY_1);  or relay_on(RELAY_3);
relay_off(RELAY_1); or relay_off(RELAY_3);
```
## Note:
Relay 1 & 2 in pin 34 and 35 of ESP32 not working because that pins are used input only.

But relay 2 had been fixed, connected with GPIO4. Using normaly. 

## Error:
1. Lost sync with device, restarting...

    Method 1:

    Fix: change vTaskDelay(xxx) in app_main() with 'xxx' small. (Ex: vTaskDelay(5)). [Link](https://answers.ros.org/question/11237/rosserial-lost-sync-with-device/)

    Or Change wifi connection (Because of internet in VTCC has proxy, sometime lost sync).

## Document:
1. [Rosserial ESP32](https://github.com/sachin0x18/rosserial_esp32)
2. [Config Static IP](https://www.esp32.com/viewtopic.php?t=14689)
3. [Service Server](http://wiki.ros.org/ROS/Tutorials/WritingServiceClient%28c%2B%2B%29)




