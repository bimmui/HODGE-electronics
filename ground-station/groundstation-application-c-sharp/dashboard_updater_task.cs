using Python.Runtime;

namespace DiagnosticApp;
public class DashboardUpdaterTask : TaskHandler
{
	public const String DLL_PATH = "/Program Files/Python39/python39.dll";
	public DashboardUpdaterTask(SharedMemory memory_manager) : base(memory_manager)
	{

	}

	protected override void task_function()
	{
		Runtime.PythonDLL = DLL_PATH;
		PythonEngine.PythonPath = PythonEngine.PythonPath + ";" + ".";
		PythonEngine.Initialize();
		PythonEngine.BeginAllowThreads();

		using (Py.GIL())
		{
			PyObject python_script = Py.Import("dashboard");

			while (true)
			{
				PyList data_array = new PyList();
				for (int i = 0; i < memory_manager.column_names.Length; i++)
				{
					data_array.Append(memory_manager.convert_to_array_python(i));
					Console.WriteLine(memory_manager.convert_to_array(i)[0]);
				}


				python_script.InvokeMethod("update_graph_data", new PyObject[]{data_array});
			}
		}
	}
}