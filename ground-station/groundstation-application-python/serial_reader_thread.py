from __future__ import annotations
from serial import Serial
from serial import SerialException

from thread_handler import ThreadHandler
from shared_memory import SharedMemory

class SerialReaderThread (ThreadHandler):

	DEFAULT_CONNECTION_LINUX_0: str = "/dev/ttyACM0" #Represents the default connection path for Linux if no USB devices are plugged in
	DEFAULT_CONNECTION_LINUX_1: str = "/dev/ttyACM1"; #Represents the default connection path for Linux if one USB device is plugged in
	DEFAULT_CONNECTION_WIN_3: str = "COM3"; #Represents the default connection path for Windows
	
	# Constructor: opens a new serial connection
	# memory_manager (SharedMemory): a reference to the shared memory where data can be pulled from
	# serial_connection_path (String): the path that represents the USB connection.  The static constants defined in this class are the common paths
	# baud_rate (int): the baud rate of the serial connection.  It should match the baud rate specified in the arduino code
	def __init__(self, memory_manager: SharedMemory, serial_connection_path: str, baud_rate: int):
		super().__init__(memory_manager, True)

		self._serial_controller: Serial = Serial(serial_connection_path, baud_rate)
		self._serial_connection_path: str = serial_connection_path
		self._baud_rate: int = baud_rate

		self._serial_controller.flush()

	# Continually logs data to the memory_manager
	# Notes on formatting data:
	# - Each list should be a number of entries equal to the number of column_names in the memory manager.  
	# - The order of the values corresponds to the order of each column_names entry
	# - The individual entries should be floating point numbers
	# - Each entry is separated by a comma (,)
	# - A newline denotes a new list
	def thread_function(self):
		add_input_list_str: list[str] = []
		add_input_list_float: list[float] = []
		try:
			add_input: str = self._serial_controller.readline().decode().strip()
			add_input_list_str = add_input.split(",")

		except SerialException:
			print("Serial reader did not read a value!")
			print(f"strList: {add_input_list_str}")


		try:
			for i in range(len(add_input_list_str)):
				add_input_list_float.append(float(add_input_list_str[i]))

			self.memory_manager.write(add_input_list_float)
			#print(self.memory_manager.get_first().get_data())

		except ValueError:
			print("Serial reader read an improper value")
			print(f"floatList: {add_input_list_float}")