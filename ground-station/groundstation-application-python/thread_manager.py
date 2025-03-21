from __future__ import annotations

from thread_handler import ThreadHandler

#The ThreadManager class manages a group of threads
class ThreadManager:
	
	# Constructor: assigns references to a number of threads
	def __init__(self, threads: list[ThreadHandler]):
		self._threads: list[ThreadHandler] = threads

	# Start all the threads contained in this manager
	def start(self):
		for thread in self._threads:
			thread.start_thread()
	
	# Join all the threads (i.e. pause execution for completion of all the threads)
	def join(self):
		for thread in self._threads:
			thread.join_thread()
		
	# Stop all the threads from running
	def stop(self):
		for thread in self._threads:
			thread.stop_thread()
	
	