#include <iostream>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>


class Application
{
	std::mutex 				m_mutex;
	std::condition_variable m_condVar;
	bool 					workdone;
	bool 					is_recording;
	std::thread 			t_record;

public:
	Application();
	~Application();
	
	void launchThread();
	void executeActuator();
	void record_from_microphone();

	bool signal4recording();
	void start_recording();
	void stop_recording();
	void stop_experiment();
	
	bool isrecording();
	bool isworkdone();
	bool isrecordingorworkdone();
};
