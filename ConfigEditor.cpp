#include <fstream>
#include "include/menu.hpp"
#include "include/configor/json.hpp"
#include "include/path.h"
#include "include/inicpp.hpp"

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

	json::value toJson();
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
inline bool editConfig(string path, json::value data) {
	ofstream file(path.c_str());
	if (!file.is_open()) return true;
	try {
		file << json::wrap(data);
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

json::value Config::toJson() {
	json::value ret;
	ret["interval_time"] = to_string(interval_time);
	ret["interval_eps"] = to_string(interval_eps);
	for (Task subtask : task) {
		ret["task"].push_back(json::object( {
			{"path", subtask.path},
			{"trigger_count", to_string(subtask.trigger_count)},
			{"trigger_interval", to_string(subtask.trigger_interval)},
			{"auto_trigger", subtask.auto_trigger}
		} ));
	}
	return ret;
}

void paintMenu(Menu &menu, const Task &subtask, const short &delt, const short &i) {
	menu.clear();
	menu.push(Button({0, (short)(delt + 0)}, "[-]", "4" + to_string(i), ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({4, (short)(delt + 0)}, "文件路径：" + subtask.path, "0" + to_string(i), ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({4, (short)(delt + 1)}, "触发次数：" + to_string(subtask.trigger_count), "1" + to_string(i), ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({4, (short)(delt + 2)}, "触发间隔：" + to_string(subtask.trigger_interval) + "ms", "2" + to_string(i), ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({4, (short)(delt + 3)}, "唤醒触发：" + (subtask.auto_trigger ? (string)"允许" : (string)"禁止"), "3" + to_string(i), ButtonColor(consoleColor.brightCyan), true, true));
}
void paintMenu(Menu &menu, const Config &config, const string &config_path) {
	menu.clear();

	menu.push(Button({0, 0}, "配置文件修改器 - " + config_path, "title", ButtonColor(consoleColor.White), true, true));
	menu.push(Button({0, 1}, "撤销上次修改", "back", ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({0, 2}, "回退至启动时", "backtobeg", ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({0, 3}, "保存并退出", "exit", ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({0, 5}, "轮询时间间隔：" + to_string(config.interval_time) + "ms", "interval_time", ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({0, 6}, "系统唤醒误差：" + to_string(config.interval_eps) + "ms", "interval_eps", ButtonColor(consoleColor.brightCyan), true, true));
	menu.push(Button({4, 7}, "任务列表 [i]", "tips", ButtonColor(consoleColor.brightBlue), true, true));
	menu.push(Button({0, 7}, "[+]", "newtask", ButtonColor(consoleColor.brightCyan), true, true));

	short heit = 0;
	for (Task subtask : config.task) {
		Menu submenu;
		paintMenu(submenu, subtask, heit * 4 + 8, heit);
		menu.push(submenu);
		heit++;
	}
}

bool change(Task &task, const string &event) {
	bool ischanged = false;
	if (event.front() == '0') {
		cout << "请输入任务文件路径：\n";
		string path; getline(cin, path);
		if (path != "") {
			pathDelete(path);
			task.path = path;
			ischanged = true;
		}
	} else if (event.front() == '1') {
		cout << "请输入触发次数：\n";
		string tc; getline(cin, tc);
		if (tc != "") {
			task.trigger_count = stol(tc);
			ischanged = true;
		}
	} else if (event.front() == '2') {
		cout << "请输入触发间隔：\n";
		string ti; getline(cin, ti);
		if (ti != "") {
			task.trigger_interval = stol(ti);
			ischanged = true;
		}
	} else if (event.front() == '3') {
		Menu chos;
		chos.push(Button({0, 0}, "是否在系统唤醒后，自动触发此任务：", "", ButtonColor(consoleColor.brightCyan), false, true));
		chos.push(Button({0, 1}, "允许", "true", ButtonColor(consoleColor.brightCyan), true, true));
		chos.push(Button({0, 2}, "禁止", "false", ButtonColor(consoleColor.brightCyan), true, true));
		chos.push(Button({0, 3}, "返回", "exit", ButtonColor(consoleColor.brightCyan), true, true));
		chos.start();
		while (true) {
			string evchos;
			if (runMenu(chos, evchos) || evchos == "exit") break;
			if (evchos == "true") task.auto_trigger = true, ischanged = true;
			else if (evchos == "false") task.auto_trigger = false, ischanged = true;
			if (evchos != "") break;
			Sleep(50);
		}
		chos.stop();
	}
	return ischanged;
}

int main(int n_, char** config_path_) {
	pathSplit(_pgmptr, self_path, self_name);
	string ini_config_path = self_path + "\\config.ini";
	pathDelete(ini_config_path);
	IniManager ini_config(ini_config_path.c_str());

	string config_path;
	if (n_ == 1) config_path = ini_config["Autorun"]["default_profile"];
	else config_path = config_path_[1];

	Config config;
	config.init(config_path);

	vector<Config> back_up;
	back_up.push_back(config);

	Menu menu;
	paintMenu(menu, config, config_path);
	menu.start();
	while (true) {
		string event;
		if (runMenu(menu, event) || event == "exit") break;
		if (event == "title") {
			menu.stop();
			int ret = MessageBox(nullptr, "你关注我了吗？", "lateworker", MB_YESNOCANCEL);
			if (ret == IDYES) MessageBoxW(nullptr, L"非常感谢！欢迎来实验室玩！（QQ，935718273）", L"(´ ∀ ` *)", MB_OK);
			else if (ret == IDNO) MessageBoxW(nullptr, L"再给你一次机会，还不赶紧关注我！？", L"o(￣ヘ￣o＃)", MB_OK);
			else if (ret == IDCANCEL) {
				MessageBoxW(nullptr, L"你什么意思？", L"(っ*´Д`)っ", MB_OK);
				system("start https://space.bilibili.com/454920362");
			}
			menu.start();
			continue;
		}
		if (event != "") {
			menu.stop();
			system("cls");
			bool ischanged = false;
			if (event == "backtobeg") {
				config = back_up.front();
				ischanged = true;
			} else if (event == "back") {
				if (back_up.size() > 1) {
					back_up.pop_back();
					config = back_up.back();
				}
			} else if (event == "interval_time") {
				cout << "请输入轮询时间间隔（ms）：\n";
				string it; getline(cin, it);
				if (it != "") {
					config.interval_time = stol(it);
					ischanged = true;
				}
			} else if (event == "interval_eps") {
				cout << "请输入系统唤醒误差（ms）：\n";
				string ie; getline(cin, ie);
				if (ie != "") {
					config.interval_eps = stol(ie);
					ischanged = true;
				}
			} else if (event == "newtask") {
				Task task ("/* 请自行修改任务文件路径 */", 1, 0, true);
				Menu newt;
				auto _paint = [&]() {
					paintMenu(newt, task, 1, 0);
					newt.push(Button({0, 0}, "创建新任务 - 编辑任务属性", "", ButtonColor(consoleColor.White), false, true));
					newt.push(Button({0, 5}, "保存并返回", "save", ButtonColor(consoleColor.brightCyan), true, true));
					newt.push(Button({0, 6}, "放弃并返回", "exit", ButtonColor(consoleColor.brightCyan), true, true));
				};
				_paint();
				newt.start();
				while (true) {
					string newtvt;
					if (runMenu(newt, newtvt) || newtvt == "exit") break;
					if (newtvt != "") {
						newt.stop();
						system("cls");
						if (newtvt == "save") {
							config.task.push_back(task);
							ischanged = true;
							break;
						} else {
							if (newtvt.front() == '4') {
								task = Task("/* 请自行修改任务文件路径 */", 1, 0, true);
							} else {
								change(task, newtvt);
							}
						}
						_paint();
						system("cls");
						newt.start();
					}
					Sleep(50);
				}
				newt.stop();
			} else if (event == "tips") {
				cout << "这里是操作指南，将介绍符号按钮的功能：\n\n";
				cout << "在主界面下：\n";
				cout << "“[+]”按钮：新建配置文件\n";
				cout << "“[-]”按钮：删除相应的配置文件\n\n";
				cout << "在新建配置文件界面下：\n";
				cout << "“[-]”按钮：重置新建的配置\n\n";
				cout << "希望此说明对你有所帮助";
				system("pause");
			} else {
				if (event.front() == '4') {
					config.task.erase(config.task.begin() + stol(event.substr(1)));
					ischanged = true;
				} else {
					ischanged = ischanged || change(config.task[stol(event.substr(1))], event);
				}
			}
			if (ischanged)
				back_up.push_back(config);
			editConfig(config_path, config.toJson());
			paintMenu(menu, config, config_path);
			system("cls");
			menu.start();
		}
		Sleep(50);
	}
	menu.stop();
	return 0;
}
