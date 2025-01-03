class SharedMemory:

	class node:

		def __init__(self, next, previous, data):
			self.next = next
			self.prev = previous
			self.data = data

	def __init__(self, maxLength, columnNames):
		self.maxLength = maxLength
		self.length = 1
		self.first = self.node(None, None, columnNames)
		self.last = self.first
		print(self.last.data)
		

	# Adds a new node to the shared memory linkedList, if the linked list length exceeds maxLength, then remove the last node
	# "data" is an array with a row to be added to a CSV sheet
	def write(self, data):
		nodeToAdd = self.node(None, self.first, data)
		self.first.next = nodeToAdd
		self.first = nodeToAdd
		self.length += 1

		if self.length > self.maxLength: #Remove last node if list is too long
			self.last = self.last.next
			self.length -= 1

SM = SharedMemory(3, ["favorite", "leastFavorite"])
SM.write(["apple", "banana"])
SM.write(["pineapple", "turnip"])
SM.write(["carrot", "mango"])
print(SM.last.data)
print(SM.first.data)

