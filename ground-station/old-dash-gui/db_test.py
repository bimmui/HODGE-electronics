import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

# its a locally hosted db idgaf putting the api key in the code
token = "AymmjKZLk5MdjFG5K___pvNY20FhDwoVvpFOg4wXEQQYMiMRJNeKpOf_Nfb0o49zpdEIyhgwzk7UxlLKx_7Eog=="
org = "TuftsRocketry"
url = "http://localhost:8086"

client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)

bucket = "bucket2"

write_api = client.write_api(write_options=SYNCHRONOUS)

# for value in range(5):
#     point = Point("measurement1").tag("tagname1", "tagvalue1").field("field1", value)
#     write_api.write(bucket=bucket, org="TuftsSEDSRocketry", record=point)
#     time.sleep(2)  # separate points by 1 second


query_api = client.query_api()

# query = """from(bucket: "bucket2")
#  |> range(start: -10m)
#  |> filter(fn: (r) => r._measurement == "measurement1")"""
# tables = query_api.query(query, org="TuftsSEDSRocketry")


query = 'from(bucket:"bucket3") |> range(start: 0)'
tables = client.query_api().query(query, org=org)

for table in tables:
    for record in table.records:
        print(record)