using System.IO.Ports;

public class SerialReaderTask : TaskHandler
{
	public const String DEFAULT_CONNECTION_LINUX_0 = "/dev/ttyACM0"; //Represents the default connection path for Linux if no USB devices are plugged in
	public const String DEFAULT_CONNECTION_LINUX_1 = "/dev/ttyACM1"; //Represents the default connection path for Linux if one USB device is plugged in
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
		SerialPort accessport = new SerialPort(serial_connection_path, baud_rate);
		while (true)
		{
			String add_input = accessport.ReadLine();
			Console.WriteLine(add_input);
		}
	}
}