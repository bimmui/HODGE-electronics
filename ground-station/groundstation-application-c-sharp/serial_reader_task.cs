using System.IO.Ports;

/*
* SerialReaderTask - This class represents a task that continually reads data incoming from a serial port and writes it to the shared memory. 
*/
public class SerialReaderTask : TaskHandler
{
	public const String DEFAULT_CONNECTION_LINUX_0 = "/dev/ttyACM0"; //Represents the default connection path for Linux if no USB devices are plugged in
	public const String DEFAULT_CONNECTION_LINUX_1 = "/dev/ttyACM1"; //Represents the default connection path for Linux if one USB device is plugged in
	public const String DEFAULT_CONNECTION_WIN_3 = "COM3"; //Represents the default connection path for Windows
	public int baud_rate {get; private set;} //The baud rate of the serial connection
	private string serial_connection_path {get; set;} //the access point of he serial connection.



	/*
	* Constructor: creates a new SerialReaderTask with a specified connection path to the
	* memory_manager (SharedMemory): a reference to the shared memory where data can be pulled from
	* serial_connection_path (String): the path that represents the USB connection
	* baud_rate (int): the baud rate of the serial connection.  I should match the boud rate specified in the arduino code
	*/
	public SerialReaderTask(SharedMemory memory_manager, String serial_connection_path, int baud_rate) : base(memory_manager)
	{
		this.baud_rate = baud_rate;
		this.serial_connection_path = serial_connection_path;
	}

	/*
	* Reads the data from the serial input and logs it to the memory_manager
	* Notes on formatting of data:
	* - A "list" represents a sequence of entries that should be logged at the same time stampt
	* - Each list should be a number of entries equal to the number of column_names in the memory manager.  
	* - The order of the values corresponds to the order of each column_names entry
	* - The individual entries should be double numbers
	* - Each entry is separated by a comma (,)
	* - A newline denotes a new leist
	*/
	protected override void task_function()
	{
		// Open a serial port
		SerialPort accessport = new SerialPort(serial_connection_path, baud_rate);
		accessport.Open();

		//Continually get info from serial connection and add it to the database.  
		while (true)
		{
			// NOTE: I tried doing an implementation that does not use split, but it was not more efficient, for the list containing doubles needed to be copied to an array. 
			String serial_data = accessport.ReadLine();
			String[] serial_data_array = serial_data.Split(",");

			double[] node_data_array = new double[serial_data_array.Length]; // This is the array that will be uploaded to the shared memory

			for (int i = 0; i < serial_data_array.Length; i++) // Cast each entry in the serial_data_array from a string to a double (stored in node_data_array)
			{
				node_data_array[i] = Convert.ToDouble(serial_data_array[i]);
			}

			memory_manager.write(node_data_array);
			//Console.WriteLine($"First entry of first node: {memory_manager.first.data[0]}  Length: {memory_manager.length}");

		}
	}
}