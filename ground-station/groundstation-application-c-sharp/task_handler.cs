// Look into using Cancellation tokens, as opposed to aborting the thread.  See: https://medium.com/@mitesh_shah/a-deep-dive-into-c-s-cancellationtoken-44bc7664555f

using System.Threading;

/*
* TaskRunner - A controls a single thread. This includes starting and stopping the thread
*/
public abstract class TaskHandler
{
	public SharedMemory memory_manager {get; private set;} //The memory manager associated with the task
	public bool is_alive {get; private set;} //records the thread status
	private Task wrapped_task {get; set;} //the variable that contains the reference to the task
	private CancellationTokenSource task_cancellation_token {get; set;} //the cancellation token to end the task

	/*
	* Constructor: Initializes the task and assigns a memory_manager to it.
	* Also creates a token to cancel the task later
	*/

	public TaskHandler(SharedMemory memory_manager)
	{
		this.memory_manager = memory_manager;
		this.is_alive = false;

		//Create a token to kill the proces later
		this.task_cancellation_token = new CancellationTokenSource();
		this.wrapped_task = new Task(task_function, task_cancellation_token.Token);

	}

	/*
	* Starts the task associated with the derived class
	*/
	public void start_task()
	{
		if(!is_alive)
		{
			wrapped_task.Start();
		}
	}

	/*
	* Sends a cancellation token to end the task associated with the derived class.  Similar to killing the task
	*/
	public void stop_task()
	{
		if(is_alive)
		{
			task_cancellation_token.Cancel();
		}
	}

	/*
	* Represents the function that will be run by the task
	*/
	protected abstract void task_function();
}