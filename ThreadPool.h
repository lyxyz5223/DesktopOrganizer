#pragma once
#include <thread>
#include <functional>
#include <condition_variable>
#include <queue>


class ThreadPool
{
private:
	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;
	std::mutex mutex;
	std::condition_variable condition;
	bool stop = false;
public:
	ThreadPool(int threadCount) {
		for (auto i = 0; i < threadCount; i++)
		{
			threads.emplace_back([this]() {
				while (true)
				{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(mutex);
						condition.wait(lock, [this] { return stop || !tasks.empty(); });
						if (stop && tasks.empty())
							return;
						task = std::move(tasks.front());
						tasks.pop();
					}
					task();
				}
				});
		}
	}
	~ThreadPool() {

	}
	void addTask(std::function<void()> task);
	void waitAll();
};

