#The SharedMemory class is a linked list with that keeps track of data transferred from the arduino
#SharedMemory has a fixed length that is specified to prevent memory overflow
#SharedMemory opens a connection to an InfluxDB to write the data for long term use
class SharedMemory:

	#The node class represents a node in the linked the linked list
	class Node:

		#Constructor: initializes a node in the linked list
		#next (Node): the next node in the linked list
		#previous (Node): the previous node in the linked list
		#data (array: any): the data stored by the node
		def __init__(self, next, previous, data):
			self.next = next
			self.prev = previous
			self.data = data

		#Gets data in a node 
		#Return: (array: any)
		def get_data(self):
			return self.data
		
		#Gets previous node 
		#Return: (Node)
		def get_prev(self):
			return self.prev
		
		#Gets next node
		#Return: (Node)
		def get_next (self):
			return self.next

	#Constructor: initializes a linked list with a specified maximum length
	#max_length (int): the maximum length of the linked list
	#column_names (array: string): the names of the fields in the InfluxDB measurement
	def __init__(self, max_length=None, column_names=None):
		self.max_length = max_length
		self.length = 1
		self.first = self.Node(None, None, column_names) #CONSIDER MAKING COLUMN NAMES NODE ALWAYS LAST
		self.last = self.first

	#Gets first node in linkedList	
	def get_first(self):
		return self.first
	
	#Gets last node in linkedList
	def get_last(self):
		return self.last

	#Adds a new node to the shared memory linked list, if the linked list length exceeds max_length, then remove the last node
	#data (array: any): an array with a row to be added to a CSV sheet
	def write(self, data):
		node_to_add = self.Node(None, self.first, data)
		self.first.next = node_to_add
		self.first = node_to_add
		self.length += 1

		if self.length > self.max_length: #Remove last node if list is too long
			self.last = self.last.next
			self.length -= 1
	
	#Converts one categoryName in the linkedList to an array
	#category_number (int): the index corresponding to the categoryName. 
	#return: (array)
	def convert_to_array(self, category_number):
		returnArray = []
		#We don't want to include the category header node
		currentNode = self.get_last().get_next()

		while currentNode != None:
			returnArray.append(currentNode.get_data()[category_number])
			currentNode = currentNode.get_next()
		
		return returnArray