// NOTE: I want to make the next and previous nodes only set by nested classes.
// See: https://stackoverflow.com/questions/42961744/how-to-restrict-access-to-a-nested-class-to-its-container-in-c

using Python.Runtime;

namespace DiagnosticApp;

	/*
* SharedMemory - Specifies an object that multiple threads can access simeltaneously.  Serves as a "shared memory" between them.
* The SharedMemory class is formatted as a linked list, which can be written and read from
*/
public class SharedMemory
{
	/*
	* Node - represents one node in the linked list
	*/
	public class Node<list_data_type>
	{
		public Node<list_data_type>? next {get; set;} //The refernece to the next node on the linked list
		public Node<list_data_type>? prev {get; set;} //The reference to the previous node on the linked list
		public list_data_type[] data {get; private set;} //The list storing the node data
		
		/* 
		* Constructor: intializes a node in the linked list
		* next (Node<T>): the next node in the linked list
		* previous (Node<T>): the previous node in the linked list
		* data (List<T>) : the data stored by the node.  The Generic constraint of the list MUST match that of the node
		* column_names_length (int): the size of the column_names array
		*/
		public Node(Node<list_data_type>? next, Node<list_data_type>? previous, list_data_type[] data)
		{
			this.next = next;
			this.prev = previous;
			this.data = data;
		}
	}

	public int max_length {get; private set;} //the maximum number of nodes on the linked list
	public int length {get; private set;} //the current number of nodes on the linked list
	public Node<double>? first {get; private set;} //the first node on the linked list
	public Node<double>? last {get; private set;} //the last node on the linked list
	public String[] column_names {get; private set;} //the names of the fields in the InfluxDB measurment

	/*
	* Constructor: Initializes the SharedMemory class, which contains a linked list of specified maximum length
	* max_length (int): the maximum length of the linked list.  Precondition: max_length > 0
	* column_names (List<String>): the names of the fields in the InfluxDB measurement
	*/
	public SharedMemory(int max_length, String[] column_names)
	{
		this.max_length = max_length;

		// If max length is less than 1, set it to 1
		if (max_length < 1)
		{
			this.max_length = 1;
		}
		
		this.length = 0;
		this.column_names = column_names;
		this.first = null;
		this.last = null;
	}

	/*
	* Adds a new node to the shared memory linked list.  If the linked list exceeds max_length, then remove the last node
	* data (node_data_type): an array with a row to be added to the database
	*/
	public void write(double[] data)
	{
		if(data.Length != column_names.Length)
		{
			Console.WriteLine("There are more data entries than column names.  Data not added!");
			return;
		}
		Node<double> node_to_add = new Node<double>(null, first, data);

		// Ensure that the first node is NOT null before changing its reference.
		// If the first node is null, then make this node the first and last node.  
		if(first != null)
		{
			node_to_add.prev = first;
			first.next = node_to_add;
			
		}
		else
		{
			last = node_to_add;
		}

		first = node_to_add;
		length += 1;

		// If the list is at its max length, then we want to drop the last node.
		// Currently, we check to make sure the last node is not null, but if max_length > 0, then that should not happen
		if(length > max_length)
		{
			last = last.next;
			last.prev = null;
			length -= 1;
		}

	}

	/*
	* Converts one columtn in the linkedList to an array
	* category_number (int): the index corresponding to the category name (first column is zero)
	*/
	public List<double>? convert_to_array(int category_number)
	{
		if(category_number > column_names.Length)
		{
			return null;
		}

		List<double> return_list = new List<double>();

		Node<double>? current_node = last;

		while(current_node != null)
		{
			return_list.Add(current_node.data[category_number]);
			current_node = current_node.next;
		}

		return return_list;
	}

	/*
	* Overload: Converts one columtn in the linkedList to an array
	* category_name (String): the name of the target category
	*/
	public List<double>? convert_to_array(String category_name)
	{
		for (int i = 0; i < column_names.Length; i++)
		{
			if (column_names[i].Equals(category_name))
			{
				return convert_to_array(i);
			}
		}

		return null;
	}

	/*
	* Converts one columtn in the linkedList to an Python list
	* category_number (int): the index corresponding to the category name (first column is zero)
	*/
	public PyList? convert_to_array_python(int category_number)
	{
		if(category_number > column_names.Length)
		{
			return null;
		}

		PyList return_list = new PyList();

		Node<double>? current_node = last;

		while (current_node != null)
		{
			return_list.Append(new PyFloat(current_node.data[category_number]));
			current_node = current_node.next;
		}

		return return_list;
	}

	/*
	* Overload: Converts one columtn in the linkedList to a Python list
	* category_name (String): the name of the target category
	* return: (PyList)
	*/
	public PyList? convert_to_array_python(String category_name)
	{
		for (int i = 0; i < column_names.Length; i++)
		{
			if (column_names[i].Equals(category_name))
			{
				return convert_to_array_python(i);
			}
		}

		return null;
	}
}

