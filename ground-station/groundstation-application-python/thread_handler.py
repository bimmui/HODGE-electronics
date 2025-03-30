from __future__ import annotations
import threading
from abc import ABC, abstractmethod

from shared_memory import SharedMemory

# The ThreadHandler class controls a single thread.  The class controls stopping and starting the thread

class ThreadHandler (ABC, threading.Thread):
	
	# Constructor: Initializes the task and assigns a memory_manager to it.
	# memory_manager (SharedMemory): the shared memory object this thread has access to
	def __init__(self, memory_manager: SharedMemory, is_infinite_loop: bool):

		self._kill: bool = False
		self._is_infinite_loop: bool = is_infinite_loop

		self._is_alive: bool = False
		self.memory_manager: SharedMemory = memory_manager


	# This function is what the _wrapped_task runs
	def run(self):
		if not self._is_infinite_loop:
			self.thread_function()
			return
		
		while self._kill == False:
			self.thread_function()
		
		self._kill = False
		self._is_alive = False




	# This function represents the body of the _thread_runner method
	# It is specified whether this method will run on loop or not in the constructor
	@abstractmethod
	def thread_function(self):
		pass

	# Starts the thread associated with the derived class
	def start_thread(self):
		if not self._is_alive:
			self.run()
			self._is_alive = True

	# Stops the thread from running by breaking it out of an infinite loop
	def stop_thread(self):
		self._kill = True
	
	# Wait for the thread to complete execution
	def join_thread(self):
		if self._is_alive:
			self.join()
	
	# check if the task is alive
	def is_alive(self) -> bool:
		return self._is_alive