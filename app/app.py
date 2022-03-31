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
    "/dev1/temp" : [],
    "/dev1/light" : [],
    "/dev2/temp" : [],
    "/dev2/light" : []
}
TIME_WINDOW_S=2
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

def publish_job(client: mqtt.Client):
    print("[THINGSBOARD] Sending data")
    with BUFFER_LOCK:
        print("[THINGSBOARD] Publishing telemetry")
        try:
            raw_output = subprocess.check_output(
                ["/opt/vc/bin/vcgencmd", "measure_temp"]
            ).decode()
            output_matching = re.search("temp=([\d\.]+)'C", raw_output)
            return float(output_matching.group(1))
        except:
            pi_temp = 50 + 10 * random.random()

        keys={
            "gateway/temp" : pi_temp
        }

        for k in TOPIC_BUFFER:
            if len(TOPIC_BUFFER[k]) == 0:
                continue
            arr = np.array(TOPIC_BUFFER[k])
            keys[k+"/median"]=np.median(arr)
            keys[k+"/average"]=np.average(arr)
            keys[k+"/std"]=np.std(arr)
            keys[k+"/min"]=arr.min()
            keys[k+"/max"]=arr.max()
            TOPIC_BUFFER[k].clear()
        
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