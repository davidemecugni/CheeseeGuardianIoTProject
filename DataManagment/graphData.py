import streamlit as st
from streamlit_autorefresh import st_autorefresh
import re
import pandas as pd
import sqlite3
from datetime import datetime,timedelta

conn = sqlite3.connect('C://Users/winbu/Desktop/IoT/DataManagment/sensor_data.db')
cursor = conn.cursor()

st.set_page_config(
        page_title="CheeseGuardian",
        page_icon="chart_with_upwards_trend",
        layout="wide",
        menu_items={
        'About': "Developed by Davide Mecugni under the supervision of Prof. David Chieng.\n"
        }
    )
streamlit_style = """
			<style>
			font-family: Courier;
			</style>
			"""
st.markdown(streamlit_style, unsafe_allow_html=True)
st.title("CheeseGuardian")
col1, col2, col3 = st.columns([2,2,1])
tdiff = 8
dim = col1.select_slider(
    'Select data window',
    options= range(5,501), value=20)
refresh = col2.select_slider(
    'Select refresh rate(ms)',
    options= [x/10 for x in range(5,105,5)], value= 2.0)
for _ in range(1):
    col3.text("")
col3.text(f"  Total number of measurements: {pd.read_sql('SELECT max(id) as id FROM sensor_data', conn)['id'][0]}")
for _ in range(3):
    col3.text("")

def strToDate(str):
    l = re.split("-|:| ", str)
    l = [int(x) for x in l]
    return datetime(l[0],l[1],l[2],l[3],l[4],l[5])




st_autorefresh(interval= refresh * 1000, key="dataframerefresh")

SQL_Query = pd.read_sql(f'SELECT temperature, humidity, CO2, tVOC, timestamp FROM sensor_data ORDER BY id DESC LIMIT {dim}', conn)
#SQL_Query = SQL_Query.reindex(index=SQL_Query.index[::-1])

revH, revT, revCO2, revtVOC = list(SQL_Query["humidity"]), list(SQL_Query["temperature"]),list(SQL_Query["CO2"]),list(SQL_Query["tVOC"])
revH, revT, revCO2, revtVOC = revH[::-1], revT[::-1], revCO2[::-1], revtVOC[::-1]


col1.metric(label = "Temperature", value = SQL_Query["temperature"][0], delta= round(SQL_Query["temperature"][0] - SQL_Query["temperature"][1],2), delta_color="normal", help=None, label_visibility="visible")
col2.metric(label = "Humidity", value = SQL_Query["humidity"][0], delta= round(SQL_Query["humidity"][0] - SQL_Query["humidity"][1],2), delta_color="normal", help=None, label_visibility="visible")

c1 = col1.line_chart(data = revH, color = ["#0000FF"])
c2 = col2.line_chart(data = revT, color = ["#FF0000"])


col3.metric(label = "CO2", value = int(SQL_Query["CO2"][0]), delta= int(SQL_Query["CO2"][0] - SQL_Query["CO2"][1]), delta_color="normal", help=None, label_visibility="visible")
col3.line_chart(data=revCO2, color= "#000000", height=110)
col3.metric(label = "tVOC", value = int(SQL_Query["tVOC"][0]), delta= int(SQL_Query["tVOC"][0] - SQL_Query["tVOC"][1]), delta_color="normal", help=None, label_visibility="visible")
col3.line_chart(data=revtVOC,color = "#000000",height=110)
#st.info("Graph", icon="ℹ️")
#Function signature[source]


lastFlood = pd.read_sql('SELECT max(timestamp) as tp FROM sensor_data WHERE flood = 1', conn)["tp"][0]
lastFlood = strToDate(lastFlood)
lastEarthquake = pd.read_sql('SELECT max(timestamp) as eq FROM sensor_data WHERE earthquake = 1', conn)["eq"][0]
lastEarthquake = strToDate(lastEarthquake)

diffFlood = datetime.now()-lastFlood-timedelta(0,3600*tdiff)
diffEarth = datetime.now()-lastEarthquake-timedelta(0,3600*tdiff)
st.text(f"Last registered flood: {lastFlood}, elapsed time: {diffFlood}")
st.text(f"Last registered earthquake: {lastEarthquake}, elapsed time: {diffEarth}")
import streamlit as st

