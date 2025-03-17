
public class DatabaseHandlerTask : TaskHandler
{
	private String org {get; set;}
	private String bucket {get; set;}
	private String table_name {get; set;}
	public String[] field_names {get; private set;}
	public DatabaseHandlerTask(SharedMemory memory_manager, String token, String org, String url, String bucket, String table_name) : base(memory_manager)
	{
		this.org = org;
		this.bucket = bucket;
		this.table_name = table_name;
		this.field_names = memory_manager.column_names;
	}

	protected override void task_function()
	{

	}
}