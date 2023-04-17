/**
 *  Created by: Mari Peele
 *  Course: COP 4520
 *	Semester: Spring 2023
 */

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

 /**
   * Timer starts a timer, stops the timer, and calculates time tracked.
   */
class Timer
{

public:

	Timer();

	~Timer();

	void startTimer();

	void stopTimer();

	std::chrono::milliseconds getExecutionTime();

private:
	std::chrono::steady_clock::time_point mStartTime;
	std::chrono::steady_clock::time_point mEndTime;

};

#endif // ! TIMER_H