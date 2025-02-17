from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
import datetime

# Define your InfluxDB connection details
token = "lrfY12UYoVD0yw4Cz3XTx2lWUuCb1-8CyTuc743pJrFqHiR7yAfH1I-bxAT_85JO2Mz_cQZn2au1oh2TqtDg9Q=="
org = "TuftsSEDSRocketry"
bucket = "bucket1"
url = "http://localhost:8086/"

# Initialize the InfluxDB client
client = InfluxDBClient(url=url, token=token)

# Initialize the Write API
write_api = client.write_api(write_options=SYNCHRONOUS)

# Create sample data
data_points = [
    {
        "measurement": "temperature",
        "tags": {"location": "sensor1"},
        "fields": {"value": 23.5},
        "time": datetime.datetime.utcnow(),
    },
    {
        "measurement": "temperature",
        "tags": {"location": "sensor2"},
        "fields": {"value": 22.3},
        "time": datetime.datetime.utcnow(),
    },
]

# Write data points to InfluxDB
for data_point in data_points:
    point = (
        Point(data_point["measurement"])
        .tag("location", data_point["tags"]["location"])
        .field("value", data_point["fields"]["value"])
        .time(data_point["time"], WritePrecision.NS)
    )

    write_api.write(bucket=bucket, org=org, record=point)

# Close the client
client.close()

print("Data successfully written to InfluxDB!")