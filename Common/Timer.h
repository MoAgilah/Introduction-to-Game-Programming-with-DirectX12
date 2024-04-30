//***************************************************************************************
// This file is inspired by GameTimer.h by Frank Luna (C) 2011 All Rights Reserved.
// It utilises LARGE_INTEGER directly avoiding the need for conversion.
//***************************************************************************************
#pragma once
#include <Windows.h>

class Timer
{
public:
	Timer();

	float TotalTime() const; // in seconds
	float DeltaTime() const { return (float)m_deltaTime; }	// in seconds

	void Reset();
	void Start();
	void Stop();
	void Tick();
private:
	bool m_stopped;

	double m_secondsPerCount;
	double m_deltaTime;

	LARGE_INTEGER m_baseTime;
	LARGE_INTEGER m_pausedTime;
	LARGE_INTEGER m_stopTime;
	LARGE_INTEGER m_prevTime;
	LARGE_INTEGER m_currTime;
};