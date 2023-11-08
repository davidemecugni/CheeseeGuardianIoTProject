import random, time
from paho.mqtt import client as mqtt_client
import sqlite3
broker = 'broker.hivemq.com'
port = 1883
topic = "Wio-CheeseGuardian"
# Generate a Client ID with the subscribe prefix.
client_id = f'subscribe-{random.randint(0, 100)}'
# username = 'emqx'
# password = 'public'
conn = sqlite3.connect('sensor_data.db')
cursor = conn.cursor()
cursor.execute('''
    CREATE TABLE IF NOT EXISTS sensor_data (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        temperature FLOAT,
        humidity FLOAT,
        abs_humidity FLOAT,
        tVOC INT,
        CO2 INT,
        flood BOOL,
        earthquake BOOL,
        timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    )
''')


def connect_mqtt() -> mqtt_client:
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n", rc)

    client = mqtt_client.Client(client_id)
    # client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client


def subscribe(client: mqtt_client):
    def on_message(client, userdata, msg):
        data = msg.payload.decode()
        #print(f"Received {data}")
        data = data.replace("\n","")
        data = data.split(",")
        data_dict = {}
        for item in data:
            key, value = item.split(':')
            data_dict[key] = value
        data = data_dict
        cursor.execute('INSERT INTO sensor_data(temperature,humidity,abs_humidity,tVOC,CO2,flood,earthquake) VALUES (?,?,?,?,?,?,?)', \
                       (data['T'],data['H'],data['AH'],data['tVOC'],data['CO2'],data['Flood'],data['Earthquake']))
        #print(data_dict)
        conn.commit()

    client.subscribe(topic)
    client.on_message = on_message


def run():
    client = connect_mqtt()
    subscribe(client)
    client.loop_forever()


if __name__ == '__main__':
    run()
