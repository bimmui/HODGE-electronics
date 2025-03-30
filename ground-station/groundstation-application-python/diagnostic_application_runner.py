from __future__ import annotations

import dotenv
import os

from shared_memory import SharedMemory
from thread_handler import ThreadHandler
from serial_reader_thread import SerialReaderThread
from db_handler_thread import DBHandlerThread
from dashboard_thread import DashboardThread
from thread_manager import ThreadManager

dotenv.load_dotenv()
token: str = os.environ["DB_TOKEN"]
org: str = os.environ["DB_ORG"]

url: str = "http://localhost:8086" #uncomment this value for local testing
#url: str = "http://192.168.1.181:8086" #uncomment this value if doing remote testing
bucket: str = "Test With Dash Revamped"
table_name: str = "Dummy alt and velocity integers"
#field_names: list[str,list[int]] = ["Time", "Altitude", "Velocity", "Acceleration", "Positions","Internal Temp", "External Temp"]
field_names: list[str] = ["Time", "Velocity", "Acceleration"]
# "Positions": [[37.7749, -122.4194],], # Check to see if typing has to be different bc its a list
# Old field_names: list[str] = ["Time", "Velocity"]
baud_rate: int = 88600
shared_memory_length: int = 1000

shared_mem: SharedMemory = SharedMemory(1000, field_names)

my_threads: list[ThreadHandler] = [
SerialReaderThread(shared_mem, SerialReaderThread.DEFAULT_CONNECTION_LINUX_0, baud_rate),
DBHandlerThread(shared_mem, url, token, org, bucket, table_name)
]

threadrunner: ThreadManager = ThreadManager(my_threads)
threadrunner.start()
dt = DashboardThread(shared_mem, 8060, "0.0.0.0")
dt.thread_function()