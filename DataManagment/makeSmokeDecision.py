"""Various experiments with diverse ML models
SVM saves the final mode to svm.mod file"""
import sqlite3
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.inspection import DecisionBoundaryDisplay
from sklearn.svm import SVC
from sklearn.cluster import KMeans
import pickle

conn = sqlite3.connect('C://Users/winbu/Desktop/IoT/DataManagment/sensor_data.db')
cursor = conn.cursor()
#input("Let's start collecting data!")
dataClean = pd.read_sql('SELECT CO2, tVOC FROM sensor_data WHERE id BETWEEN 8130 AND 8209', conn)
dataPolluted = pd.read_sql('SELECT CO2, tVOC FROM sensor_data WHERE id BETWEEN 8210 AND 8248', conn)
dataPolluted = dataPolluted.drop(index=[12,13,14,33,34])
y_c = [0 for i in range(len(dataClean))]
y_p = [1 for i in range(len(dataPolluted))]
Y = y_c + y_p
X = dataClean._append(dataPolluted, ignore_index=True)
X = X.values


#Build the model
svm = SVC(kernel="rbf", gamma='scale', C=0.4)
# Trained the model
svm.fit(X, Y)
# Plot Decision Boundary
DecisionBoundaryDisplay.from_estimator(
		svm,
		X,
		response_method="predict",
		cmap=plt.cm.Spectral,
		alpha=0.8,
		xlabel="CO2",
		ylabel="tVOC",
	)

# Scatter plot
plt.scatter(X[:, 0], X[:, 1], 
			c=Y, 
			s=20, edgecolors="k")
#plt.show()
#filename = 'svm.mod'
#pickle.dump(svm, open(filename, 'wb'))
 
# some time later...
 
# load the model from disk
#loaded_model = pickle.load(open(filename, 'rb'))
#result = loaded_model.predict(X[0:2])
#print(result)
kmeans = KMeans(n_clusters=2,random_state=0,n_init='auto').fit(X)
print(kmeans.cluster_centers_)
#[[  700.2254902   1102.49019608]
#[21882.58333333 51192.75      ]]
X = pd.read_sql('SELECT CO2, tVOC FROM sensor_data', conn)
X = X.values
kmeans = KMeans(n_clusters=2,random_state=0,n_init='auto').fit(X)
print(kmeans.cluster_centers_)

X = pd.read_sql('SELECT temperature, humidity FROM sensor_data', conn)
X = X.values
kmeans = KMeans(n_clusters=1,random_state=0,n_init='auto').fit(X)
print(kmeans.cluster_centers_)