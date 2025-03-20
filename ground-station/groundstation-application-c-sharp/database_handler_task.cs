using InfluxDB.Client;
using InfluxDB.Client.Writes;

namespace DiagnosticApp;

/*
* DatabaseHandlerTask - Opens a connection to an influxDB database and contains a thread that can be run to continually log data to the database.
*/
public class DatabaseHandlerTask : TaskHandler
{
	private String org {get; set;} //the organization name of the InfluxDB
	private String bucket {get; set;} //the bucket to log the data to
	private String table_name {get; set;} //the name of the table (measurement) which the data should be recorded to
	public String[] field_names {get; private set;} //the name of the fields which the data should be recorded to
	private WriteApi api_writer {get; set;} //The object that allows data to be written to the API

	/*
	* Constructor: opens a connection to the influxDB database
	* memory_manager (SharedMemory): the reference to the Sharedmemory manager of this class.  
	* token (String): the influxDB access token
	* org (String): the influxDB organization name
	* url (String): the access url of the influx DB
	* bucket (String): the name of the bucket which the data should be recorded to
	* table_name (string): the name of the table (measurement) which the data should be recorded to
	*/
	public DatabaseHandlerTask(SharedMemory memory_manager, String token, String org, String url, String bucket, String table_name) : base(memory_manager)
	{
		this.org = org;
		this.bucket = bucket;
		this.table_name = table_name;
		this.field_names = memory_manager.column_names;

		InfluxDBClient write_client = new InfluxDBClient(url: url, token: token);
		this.api_writer = write_client.GetWriteApi();
	}

	/*
	* Writes data to the influxDB database
	* data (double[]): the data to be added to the database.  This argument should have the same length as the field_names array.  
	*/
	private void write_to_database(double[] data)
	{
		PointData[] points = new PointData[field_names.Length];

		for (int i = 0; i < points.Length; i++)
		{
			points[i] = PointData.Measurement(table_name);
			points[i].Field(field_names[i], data[i]);
		}

		api_writer.WritePoints(points, bucket, org);
	}

	/*
	* This function represents the task that will read data from the memory_manager and log it to the DB
	*/
	protected override void task_function()
	{
		while (true)
		{
			//Ensure that the memory_manager has data available to write to the database
			if (memory_manager.first != null)
			{
				write_to_database(memory_manager.first.data);
			}
		}
	}
}

