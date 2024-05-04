#include <bits/stdc++.h>
#include <synchapi.h>
#include <windef.h>
#include <winuser.h>
#include "include/configor/json.hpp"
#include "include/inicpp.hpp"
#include "include/path.h"
#include "include/system.h"

using namespace std;
using namespace path;
using namespace configor;
using namespace inicpp;

string self_path, self_name;

class Task {
public:
	string path;
	size_t trigger_count;
	clock_t trigger_interval;
	bool auto_trigger;

	inline void init();
	inline void init(const json::value &task);
	inline void init(const string &p, const size_t &tc, const clock_t &ti, const bool &at);

	Task() { init(); }
	Task(const json::value &task) { init(task); }
	Task(const string &p, const size_t &tc, const clock_t &ti, const bool &at) { init(p, tc, ti, at); }

	void run();
};
class Config {
public:
	clock_t interval_time, interval_eps;
	vector<Task> task;

	inline void init();
	inline void init(const json::value &config);
	inline bool init(const string &config_path);
	inline void init(const clock_t &it, const clock_t &ie, const vector<Task> &t);

	Config() { init(); }
	Config(const json::value &config) { init(config); }
	Config(const string config_path) { init(config_path); }
	Config(const clock_t &it, const clock_t &ie, const vector<Task> &t) { init(it, ie, t); }
};

inline bool loadConfig(string path, json::value &data) {
	data.clear();
	ifstream file(path.c_str());
	if (!file.is_open()) return true;
	try {
		file >> json::wrap(data);
	} catch (exception &_ERROR_) { return true; }
	file.close();
	return false;
}

inline void Task::init() {
	path = "";
	trigger_count = 1;
	trigger_interval = 0;
	auto_trigger = true;
}
inline void Task::init(const json::value &task) {
	init();
	path = task["path"];
	pathDelete(path);
	if (task.count("trigger_count") > 0)
		trigger_count = stol(task["trigger_count"]);
	if (task.count("trigger_interval") > 0)
		trigger_interval = stol(task["trigger_interval"]);
	if (task.count("auto_trigger") > 0)
		auto_trigger = task["auto_trigger"];
}
inline void Task::init(const string &p, const size_t &tc, const clock_t &ti, const bool &at) {
	init();
	path = p;
	trigger_count = tc;
	trigger_interval = ti;
	auto_trigger = at;
}

inline void Config::init() {
	interval_time = 100;
	interval_eps = 1000;
	task.clear();
}
inline void Config::init(const json::value &config) {
	init();
	interval_time = stol(config["interval_time"]);
	if (config.count("interval_eps"))
		interval_eps = stol(config["interval_eps"]);
	if (config.count("task") > 0) {
		for (json::value subtask : config["task"])
			task.push_back(subtask);
	}
}
inline bool Config::init(const string &config_path) {
	json::value config;
	bool is_failed = loadConfig(config_path, config);
	if (is_failed) return true;

	init(config);

	return false;
}
inline void Config::init(const clock_t &it, const clock_t &ie, const vector<Task> &t) {
	init();
	interval_time = it;
	interval_eps = ie;
	task = t;
}

inline void debug(const Config &config) {
	cout << config.interval_time << "\n";
	for (Task subtask : config.task) {
		cout << subtask.path << " " << subtask.trigger_count << " " << subtask.trigger_interval << " " << subtask.auto_trigger << "\n";
	}
}

void Task::run() {
	string target, trash;
	pathSplit(path, target, trash);
	chdir(target.c_str());
	vector< future<void> > pool;
	for (size_t i = 1; i <= trigger_count; i++) { // PTSD
		pool.push_back(async(launch::async, executefile, path));
		if (i < trigger_count)
			Sleep(trigger_interval);
	}
	for (future<void> &fut : pool)
		fut.wait();
	chdir(self_path.c_str());
}
void process(const Config &config) {
	clock_t time_rec, time_now;
	time_rec = time_now = 0;
	map<string, bool> exist_rec;
	for (Task subtask : config.task)
		exist_rec[subtask.path] = false;
	while (true) {
		time_rec = time_now;
		time_now = clock();
		for (Task subtask : config.task) {
			const string &path = subtask.path; // PTSD
			bool exist_now = pathExist(path);
			if (exist_now && (!exist_rec[path] || (subtask.auto_trigger && time_now - time_rec > config.interval_time + config.interval_eps))) {
				future<void> trash = async(launch::async, Task::run, subtask); // 有风险，目前最优
			}
			exist_rec[path] = exist_now;
		}
		Sleep(config.interval_time);
	}
}

int main(int n_, char** config_path_) {
	pathSplit(_pgmptr, self_path, self_name);
	HWND console = FindWindowA(nullptr, _pgmptr);
	IniManager ini_config("config.ini");

	if (ini_config["Autorun"]["hide_console_window"] == "1")
		ShowWindow(console, SW_HIDE);

	vector<string> config_path;
	if (n_ == 1)
		config_path.push_back(ini_config["Autorun"]["default_profile"]);
	else
		for (int i = 1; i < n_; i++)
			config_path.push_back(config_path_[i]);

	for (string path : config_path) {
		Config config;
		bool is_failed = config.init(path);
		if (is_failed) {
			//
			string alert = "配置文件 \"" + path + "\" 解析失败！";
			MessageBox(nullptr, alert.c_str(), "Error - Autorun", MB_OK);
			continue;
		}
		future<void> trash = async(process, config);
	}
	return 0;
}
