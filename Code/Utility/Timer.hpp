#pragma once
#include "TimeTool.hpp"
#include <iostream>

class Timer
{
	using CalculatePrecision = std::chrono::nanoseconds;
	using ClockType = std::chrono::steady_clock;

public:
	enum PrintPrecision
	{
		PrintSecond,
		PrintMillisecond,
		PrintMicrosecond,
		PrintNanosecond
	};

	Timer(bool print = false, PrintPrecision printPrecision = PrintMillisecond) : need_print(print)
	{
		switch (printPrecision)
		{
		case PrintSecond:
			unit = "s";
			break;
		case PrintMillisecond:
			unit = "ms";
			break;
		case PrintMicrosecond:
			unit = "us";
			break;
		case PrintNanosecond:
			unit = "ns";
			break;
		default:
			break;
		}

		precisionCastRatio = 1.0 * ClockType::period::num / ClockType::period::den;

		if (typeid(CalculatePrecision).hash_code() == typeid(std::chrono::seconds).hash_code())
		{
			precisionCastRatio *= 1e9;
		}
		else if (typeid(CalculatePrecision).hash_code() == typeid(std::chrono::milliseconds).hash_code())
		{
			precisionCastRatio *= 1e6;
		}
		else if (typeid(CalculatePrecision).hash_code() == typeid(std::chrono::microseconds).hash_code())
		{
			precisionCastRatio *= 1e3;
		}

		switch (printPrecision)
		{
		case PrintMillisecond:
			precisionCastRatio *= 1e3;
			break;
		case PrintMicrosecond:
			precisionCastRatio *= 1e6;
			break;
		case PrintNanosecond:
			precisionCastRatio *= 1e9;
			break;
		default:
			break;
		}
	}

	void StartTimer()
	{
		startTime = TimeTool::GetSteadyTime<CalculatePrecision>();
		lastTime = TimeTool::GetSteadyTime<CalculatePrecision>();
	}

	double ElapsedByStart() const
	{
		const long long& total_time = TimeTool::DurationTime<CalculatePrecision>(lastTime);
		auto res = double(total_time) * precisionCastRatio;
		if (need_print)
		{
			printf("Total took %f%s.\n", res, unit.c_str());
		}
		return res;
	}

	double ElapsedByLast()
	{
		const long long& cur_time = TimeTool::GetSteadyTime<CalculatePrecision>();
		const long long& total_time = cur_time - lastTime;
		lastTime = cur_time;
		auto res = double(total_time) * precisionCastRatio;
		if (need_print)
		{
			printf("It took %f%s.\n", res, unit.c_str());
		}
		return res;
	}

	template <typename FunctionType, typename ...Types>
	double TestFunctionCostTime(const int& runTimes, const FunctionType& func, Types&&... args) const
	{
		const long long& startTime = TimeTool::GetSteadyTime<CalculatePrecision>();

		for (auto i = 0; i < runTimes; ++i)
		{
			func(std::forward<Types>(args)...);
		}

		const long long& costTime = TimeTool::DurationTime<CalculatePrecision>(startTime);
		auto res = double(costTime) * precisionCastRatio;

		printf("Function run %d times, took %f%s.\n", runTimes, res, unit.c_str());
		return res;
	}

private:
	bool need_print;
	long long startTime;
	long long lastTime;
	double precisionCastRatio;
	std::string unit;
};
