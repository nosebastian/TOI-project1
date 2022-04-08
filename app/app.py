import re
from pip import main
import paho.mqtt.client as mqtt 
from datetime import datetime
from threading import Lock
import time
import schedule
import numpy as np
import json
import collections
import gpiozero
import subprocess
import random

ACCESS_TOKEN="22SXsHHTcI9JhN4MUWvU"
MQTT_SERVER="147.229.12.176"
TELEMETRY_TOPIC="v1/devices/me/telemetry"
TOPIC_BUFFER={
    "/esp1/temp" : [10,10,3,5,8],
    "/esp1/light" : [3,5,6,8,9],
    "/esp2/temp" : [2,1,3,5,6],
    "/esp2/light" : [9,8,2,3],
    "/cpu/temp" : [10,56,8,96],
}
TIME_WINDOW_S=10
PUBLISH_LOCK=True
BUFFER_LOCK=Lock()

# The callback for when the client receives a CONNACK response from the server.
def on_connect_local(client, userdata, flags, rc):
    print("[LOCAL] Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    for t in TOPIC_BUFFER:
        client.subscribe(t)

# The callback for when a PUBLISH message is received from the server.
def on_message_local(client, userdata, msg):
    with BUFFER_LOCK:
        TOPIC_BUFFER[msg.topic].append(msg.payload)
    print("[LOCAL] "+msg.topic+" "+str(msg.payload))

def on_connect_thingsboard(client, userdata, flags, rc):
    print("[THINGSBOARD] Connected with result code "+str(rc))
def on_publish_thingsboard(client,userdata,result):
    print("[THINGSBOARD] Data published with result code "+str(result))

def temp_job():
    try:
        with open('/sys/class/thermal/thermal_zone0/temp') as f:
            raw_output = f.read()
        return float(raw_output)/1000
    except:
        pi_temp = 50 + 10 * random.random()

    with BUFFER_LOCK:
        TOPIC_BUFFER["/cpu/temp" ].append(pi_temp)

def publish_job(client: mqtt.Client):
    print("[THINGSBOARD] Sending data")

    keys={}
    with BUFFER_LOCK:
        for k in TOPIC_BUFFER:
            if len(TOPIC_BUFFER[k]) == 0:
                continue
            arr = np.array(TOPIC_BUFFER[k])
            keys[k+"/median"]=np.median(arr)
            keys[k+"/average"]=np.average(arr)
            keys[k+"/min"]=arr.min()
            keys[k+"/max"]=arr.max()
            #TOPIC_BUFFER[k].clear()
    
    print("[THINGSBOARD] Publishing telemetry")
    msg_payload = json.dumps(keys, indent=4)
    client.publish(TELEMETRY_TOPIC, msg_payload)

if __name__ == '__main__':
    client_local = mqtt.Client("local")
    client_local.on_connect = on_connect_local
    client_local.on_message = on_message_local
    #client_local.connect("localhost", 1883, 60)

    client_thingsboard = mqtt.Client("rpiGateway")
    client_thingsboard.on_connect = on_connect_thingsboard
    client_thingsboard.on_publish = on_publish_thingsboard
    client_thingsboard.username_pw_set(ACCESS_TOKEN)
    client_thingsboard.connect(MQTT_SERVER, 1883, 60)

    schedule.every(TIME_WINDOW_S).seconds.do(lambda: publish_job(client_thingsboard))
    while True:
        schedule.run_pending()
        time.sleep(1)