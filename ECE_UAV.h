//Declaration of class ECE_UAV, to store information about a uAV

#pragma once
#include<thread>
#include<string>
#include<atomic>
#include<mutex>

class ECE_UAV
{
public:
	void start();
	void stop();
	void position(double* inPos) 
	{
		m_mutex.lock();
		memcpy(m_Position, inPos, 3 * sizeof(double));
		m_mutex.unlock();
	}//make it thread safe
	double* getPosition(void);
	friend void threadFunction(ECE_UAV* pUAV);

private:
	std::atomic<bool> m_bStop = false;
	double m_mass = 1.0;
	double m_Position[3] = {};
	double m_Velocity[3] = {};
	double m_Acceleration[3] = {};

	std::thread m_KinematicsThread;
	std::mutex m_mutex;
};