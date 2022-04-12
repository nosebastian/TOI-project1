import os
import re
import threading
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
    "/esp1/temp" : [],
    "/esp1/light" : [],
    "/esp2/temp" : [],
    "/esp2/light" : [],
    "/cpu/temp" : [],
}
TIME_WINDOW_S=10
TEMP_MEASURE_S=2
BUFFER_LOCK=Lock()

# The callback for when the client receives a CONNACK response from the server.
def on_connect_local(client, userdata, flags, rc):
    print("[LOCAL] Connected with result code "+str(rc))
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    for t in TOPIC_BUFFER:
        print("[LOCAL] Subscribe to "+t)
        client.subscribe(t)

# The callback for when a PUBLISH message is received from the server.
def on_message_local(client, userdata, msg):
    dat=float(msg.payload.decode('utf-8'))
    with BUFFER_LOCK:
        TOPIC_BUFFER[msg.topic].append(dat)
    print(f"[LOCAL] {msg.topic} {dat}")

def on_publish_local(client,userdata,result):
    print("[LOCAL] Data published with result code "+str(result))

def on_connect_thingsboard(client, userdata, flags, rc):
    print("[THINGSBOARD] Connected with result code "+str(rc))
def on_publish_thingsboard(client,userdata,result):
    print("[THINGSBOARD] Data published with result code "+str(result))

def temp_job():
    if os.path.isfile('/sys/class/thermal/thermal_zone0/temp'):
        with open('/sys/class/thermal/thermal_zone0/temp') as f:
            raw_output = f.read()
        pi_temp = float(raw_output)/1000
        print(f"[RASPBERRY] Measuring true temp {pi_temp}")
    else:
        pi_temp = 50 + 10 * random.random()
        print(f"[RASPBERRY] Measuring rand temp {pi_temp}")
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
            keys[k+"/median"]=float(np.median(arr))
            keys[k+"/average"]=float(np.average(arr))
            keys[k+"/min"]=float(arr.min())
            keys[k+"/max"]=float(arr.max())
            TOPIC_BUFFER[k].clear()
    
    print("[THINGSBOARD] Publishing telemetry")
    msg_payload = json.dumps(keys, indent=4)
    #print(msg_payload)
    client.publish(TELEMETRY_TOPIC, msg_payload)

def run_threaded(job_func, *args, **kwargs):
    job_thread = threading.Thread(target=job_func, args=args, kwargs=kwargs)
    job_thread.start()


if __name__ == '__main__':
    client_local = mqtt.Client("localClient")
    client_local.on_connect = on_connect_local
    client_local.on_message = on_message_local
    client_local.on_publish = on_publish_local
    client_local.connect("mqtt_server")
    client_local.loop_start()

    client_thingsboard = mqtt.Client("rpiGateway")
    client_thingsboard.on_connect = on_connect_thingsboard
    client_thingsboard.on_publish = on_publish_thingsboard
    client_thingsboard.username_pw_set(ACCESS_TOKEN)
    client_thingsboard.connect(MQTT_SERVER, 1883, 60)
    client_thingsboard.loop_start()

    schedule.every(TEMP_MEASURE_S).seconds.do(run_threaded, temp_job)
    schedule.every(TIME_WINDOW_S).seconds.do(run_threaded, publish_job, client_thingsboard)
    while True:
        schedule.run_pending()
        time.sleep(1)