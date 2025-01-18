import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from flightsql import FlightSQLClient

token = "x9XGYWi6Mz5jJBo7LXUqrrhfSDEQokSVE2LdTQQFn-YmAZpDE78eB5LOd64loJ_x92xioNZTrUPQGqoanRH2zA=="
org = "TuftsSEDSRocketry"
url = "http://localhost:8086"

write_client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)

bucket = "bucket1"

write_api = write_client.write_api(write_options=SYNCHRONOUS)

for value in range(5):
    point = Point("measurement1").tag("tagname1", "tagvalue1").field("field1", value)
    write_api.write(bucket=bucket, org="TuftsSEDSRocketry", record=point)
    time.sleep(1)  # separate points by 1 second

query_api = write_client.query_api()

query = """from(bucket: "bucket1")
 |> range(start: -10m)
 |> filter(fn: (r) => r._measurement == "measurement1")"""
tables = query_api.query(query, org="TuftsSEDSRocketry")

# for table in tables:
#     for record in table.records:
#         print(record)


client = FlightSQLClient(
    host="localhost",
    token="x9XGYWi6Mz5jJBo7LXUqrrhfSDEQokSVE2LdTQQFn-YmAZpDE78eB5LOd64loJ_x92xioNZTrUPQGqoanRH2zA==",
    metadata={"bucket-name": "bucket1"},
)

info = client.execute("select * from cpu limit 10")
reader = client.do_get(info.endpoints[0].ticket)

for batch in reader:
    print(batch)