import sqlite3
import pandas as pd


conn = sqlite3.connect('C://Users/winbu/Desktop/IoT/DataManagment/sensor_data.db')
cursor = conn.cursor()
input("Let's start collecting data!")
data = pd.read_sql('SELECT CO2, tVOC FROM sensor_data WHERE id = (SELECT max(id) from sensor_data)', conn)

print(data)
