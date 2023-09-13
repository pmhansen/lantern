# Setup of Raspberry Pi with Raspbian, configuring Zigbee2MQTT, wol etc.

# Run this playbook
_`--vault-password-file` can be replaced with `--ask-vault-pass`. This will prompt for the password, instead of using a file_
```
# Run the playbook
ansible-playbook playbook.yml -i inventory --vault-password-file=../.vault-pw --ask-become-pass
```

# lantern

#### Objective is controlling driveway light based on daylight (lux) and occupancy sensor.

Data in and out are coming through Mosquitto[1] MQTT, and the hard work are done by Zigbee2MQTT[2]

A Zigbee SONOFF ZBDongle-E[3] is connected to an USB port on the Raspberry Pi.

Light is controlled by a Envilar[4] relay.

Input illuminance and occupancy values are coming from the Philips Hue[5] sensor.

[1] - https://mosquitto.org/  
[2] - https://www.zigbee2mqtt.io/  
[3] - https://www.zigbee2mqtt.io/devices/ZBDongle-E.html  
[4] - https://www.zigbee2mqtt.io/devices/2CH-ZG-BOX-RELAY.html  
[5] - https://www.zigbee2mqtt.io/devices/9290030674.html