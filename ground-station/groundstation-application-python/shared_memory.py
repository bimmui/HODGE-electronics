from __future__ import annotations
from typing import TypeVar, Optional, Generic

T = TypeVar("T")

#The Node class represents a node in the linked the linked list
class Node (Generic[T]):

	#Constructor: initializes a node in the linked list
	#next (Node): the next node in the linked list
	#previous (Node): the previous node in the linked list
	#data (array: any): the data stored by the node
	def __init__(self, next: Optional[Node[T]], previous: Optional[Node[T]], data: list[T]):
		self.next: Optional[Node[T]] = next
		self.prev: Optional[Node[T]] = previous
		self.data: list[T] = data

	#Gets data in a node 
	def get_data(self) -> list[T]:
		return self.data
	
	#Gets previous node 
	def get_prev(self) -> Optional[Node[T]]:
		return self.prev
	
	#Gets next node
	def get_next (self) -> Optional[Node[T]]:
		return self.next
#The SharedMemory class is a linked list with that keeps track of data transferred from the arduino
#SharedMemory has a fixed length that is specified to prevent memory overflow
#SharedMemory opens a connection to an InfluxDB to write the data for long term use
class SharedMemory:
	#Constructor: initializes a linked list with a specified maximum length
	#max_length (int): the maximum length of the linked list
	#column_names (array: string): the names of the fields in the InfluxDB measurement
	def __init__(self, max_length: int, column_names: list[str]):
		self._max_length: int = max_length
		self._length: int = 1
		self._column_names: list[str] = column_names
		self._first: Optional[Node[float]] = None #CONSIDER MAKING COLUMN NAMES NODE ALWAYS LAST
		self._last: Optional[Node[float]] = None

	#Gets first node in linkedList	
	def get_first(self) -> Optional[Node[float]]:
		return self._first
	
	#Gets last node in linkedList
	def get_last(self) -> Optional[Node[float]]:
		return self._last
	
	#Gets the length of the linkedList
	def length(self) -> int:
		return self._length
	
	#Gets the maximum length of the linkedList
	def max_length(self) -> int:
		return self._max_length
	
	#Gets the category names corresponding to the array entries in the linkedList
	def get_column_names(self) -> list[str]:
		return self._column_names

	#Adds a new node to the shared memory linked list, if the linked list length exceeds max_length, then remove the last node
	#data (array: any): an array with a row to be added to a CSV sheet
	def write(self, data: list[float]):
		
		if len(data) > len(self._column_names):
			print("There are more data entries than column names.  Data not added!")

		node_to_add: Node[float] = Node(None, self._first, data)

		if self._first != None:
			node_to_add.prev = self._first
			self._first.next = node_to_add
		else:
			self._last = node_to_add
		
		self._first = node_to_add
		self._length += 1

		#The error raised by the linter should not be a issue, for we check to make sure the last node is not null
		if (self._length > self._max_length) and (self._last != None): #Remove last node if list is too long
			self._last = self._last.next
			self._last.prev = None
			self._length -= 1
	
	#Converts one categoryName in the linkedList to an array
	#category_number (int): the index corresponding to the categoryName. 
	#return: (array)
	def convert_to_array(self, category_number: int) -> list[float]:
		returnArray: list[float] = []

		currentNode: Optional[Node[float]] = self._last
		while currentNode != None:
			returnArray.append(currentNode.get_data()[category_number])
			currentNode = currentNode.get_next()
		
		return returnArray
	
	#IMPLEMENT WITH METHOD OVERLOADING
	# Overload: Converts one column in the linkedList to an array
	# catergory_name (str): the name of the target category
	#def convert_to_array(self, category_name: str) -> list[T]:
		#for i in range(len(self._column_names)):
			#if (self._column_names[i] == category_name):
				#return self.convert_to_array(i)