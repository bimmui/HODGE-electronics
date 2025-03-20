
using DiagnosticApp;
public class DashboardUpdaterTester
{
	public static void Main(String[] args)
	{
		Console.WriteLine("Starting Program...");
		String[] column_names = {"velocity", "altitude", "acceleration"};
		SharedMemory sm = new SharedMemory(1000, column_names);

		DashboardUpdaterTask my_task = new DashboardUpdaterTask(sm);
		SerialReaderTask reader = new SerialReaderTask(sm, SerialReaderTask.DEFAULT_CONNECTION_WIN_3, 88600);
		reader.start_task();
		my_task.start_task();
		my_task.join_task();
	}
}