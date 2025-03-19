using InfluxDB.Client.Api.Domain;
using DiagnosticApp;
public class SharedMemoryTester
{
	public static void Main(String[] args)
	{
		Console.WriteLine("Starting Program!");
		String[] column_names = {"velocity", "altitude", "acceleration"};
		SharedMemory sm = new SharedMemory(1000, column_names);
		SerialReaderTask reader = new SerialReaderTask(sm, SerialReaderTask.DEFAULT_CONNECTION_WIN_3, 88600);
		reader.start_task();
		reader.join_task();
	}
}