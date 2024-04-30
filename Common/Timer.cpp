#include "Timer.h"

Timer::Timer()
	: m_stopped(false),
	m_secondsPerCount(0.0),
	m_deltaTime(-1.0)
{
	LARGE_INTEGER countsPerSec;
	QueryPerformanceFrequency(&countsPerSec);
	m_secondsPerCount = 1.0 / (double)countsPerSec.QuadPart;
}

float Timer::TotalTime() const
{
	// if stopped, we don't count time that has passed
	// since we stopped. If we previousily stopped, the distance
	// m_stopTime - m_baseTime includes paused time. To adjust for
	// this we can subtract the paused time from m_stopTime
	//		previousily paused time
	//			<-------------->
	// ---*-------------*-------------*---------*------------*------> time
	// m_baseTime				m_stopTime	m_currTime

	if (m_stopped)
	{
		return (float)(((m_stopTime.QuadPart - m_pausedTime.QuadPart) - m_baseTime.QuadPart) * m_secondsPerCount);
	}

	// the distance m_currTime - m_baseTime includes paused time, we can
	// subtract the paused time from m_currentTime to adjust for this.
	//		previousily paused time
	//			<-------------->
	// ---*-------------*-------------*---------*------------*------> time
	// m_baseTime	m_stopTime	startTime	m_currTime

	return (float)(((m_currTime.QuadPart - m_pausedTime.QuadPart) - m_baseTime.QuadPart) * m_secondsPerCount);
}

void Timer::Reset()
{
	LARGE_INTEGER  currTime;
	QueryPerformanceCounter(&currTime);

	m_baseTime = currTime;
	m_prevTime = currTime;
	m_stopTime = { 0 };
	m_stopped = false;
}

void Timer::Start()
{
	LARGE_INTEGER startTime;
	QueryPerformanceCounter(&startTime);

	// acculmalate the time elasped between stop and start times
	// ----------------*-----------------*----------------> time
	//		m_stoptime		m_startTime
	if (!m_stopped)
	{
		// then accumulate the paused time
		m_pausedTime.QuadPart += (startTime.QuadPart - m_stopTime.QuadPart);

		// on restart the current previous time is invalid
		// as it occured during a pause
		m_prevTime = startTime;

		m_stopTime = { 0 };
		m_stopped = false;
	}
}

void Timer::Stop()
{
	// if we are already stopped don't do anything
	if (!m_stopped)
	{
		LARGE_INTEGER currTime;
		QueryPerformanceCounter(&currTime);

		// other wise save the time we stopped at, and set
		// the boolean flag indicating the timer is stopped
		m_stopTime = currTime;
		m_stopped = true;
	}
}

void Timer::Tick()
{
	if (m_stopped)
	{
		m_deltaTime = 0.0;
		return;
	}

	// get the time this frame
	LARGE_INTEGER currTime;
	QueryPerformanceCounter(&currTime);
	m_currTime = currTime;

	// time difference between this frame and the previous.
	m_deltaTime = (m_currTime.QuadPart - m_prevTime.QuadPart) * m_secondsPerCount;

	// prepare for the next frame.
	m_prevTime = m_currTime;

	// force non-negative: DXSDK's CDXUTTimer mentions that if
	//  the processor goes into power save mode or we get shuffled
	//  to another processor, then deltatime can be negative.
	if (m_deltaTime < 0.0)
		m_deltaTime = 0.0;
}