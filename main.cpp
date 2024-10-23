#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>
#include <crow.h>
#include <boost/program_options.hpp>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include <iostream>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <csignal>

enum ProgramLocation {
    NOT_FOUND = 0,
    ENV_ONLY = 1,
    LOCAL_ONLY = 2,
    BOTH = 3
};

struct Job {
    int id;
    std::string command;
    std::string result;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    bool result_ready = false;
    bool running = true;
};

std::vector<std::shared_ptr<Job>> jobs;
std::mutex jobs_mutex;
std::condition_variable jobs_cv;
std::atomic<bool> shutting_down(false);
std::vector<std::future<void>> futures;
int max_concurrent_jobs;
std::atomic<int> running_jobs(0);
std::string response_old = "";

int program_exists(const std::string& program) {
    bool local_exists = std::filesystem::exists("./" + program);
    std::string which_cmd = "which " + program + " > /dev/null 2>&1";
    bool env_exists = (system(which_cmd.c_str()) == 0);

    if (local_exists && env_exists) {
        return BOTH;
    } else if (local_exists) {
        return LOCAL_ONLY;
    } else if (env_exists) {
        return ENV_ONLY;
    } else {
        return NOT_FOUND;
    }
}

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::string full_cmd = std::string(cmd) + " 2>&1";
    std::shared_ptr<FILE> pipe(popen(full_cmd.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}


void run_command(std::shared_ptr<Job> job) {
    job->start_time = std::chrono::system_clock::now();
    std::time_t start_time_t = std::chrono::system_clock::to_time_t(job->start_time);
    std::string start_time_str = std::ctime(&start_time_t);
    std::cout << "\tJob started: " << job->command << " at " << start_time_str << std::endl;

    running_jobs++;

    std::string output = exec(job->command.c_str());
    job->end_time = std::chrono::system_clock::now();

    {
        std::lock_guard<std::mutex> lock(jobs_mutex);
        job->result = output;
        job->result_ready = true;
        job->running = false;
        std::time_t end_time_t = std::chrono::system_clock::to_time_t(job->end_time);
        std::string end_time_str = std::ctime(&end_time_t);
        std::cout << "\tJob finished: " << job->command << " at " << end_time_str << std::endl;
    }

    running_jobs--;
    jobs_cv.notify_all();
}

void signal_handler(int signal) {
    shutting_down.store(true);
    jobs_cv.notify_all();

    for (auto& future : futures) {
        future.wait();
    }

    exit(signal);
}

int main(int argc, char* argv[]) {
    // Register signal handler for program termination
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("port,P", po::value<int>()->default_value(8080), "set port number")
        ("threads,T", po::value<int>()->default_value(10), "set number of threads")
        ("ip,I", po::value<std::string>()->default_value("0.0.0.0"), "set IP address or DNS name")
        ("job,J", po::value<std::string>()->default_value("stress-ng"), "set program to execute")
        ("check-program,C", po::value<bool>()->default_value(true), "check if program exists before starting")
        ("prefer-local,L", po::value<bool>()->default_value(true), "prefer local program over system-wide")
        ("max-concurrent-jobs,M", po::value<int>()->default_value(5), "set maximum number of concurrent jobs");

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << desc << "\n";
        return 1;
    }

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    int port = vm["port"].as<int>();
    int threads_count = vm["threads"].as<int>();
    std::string ip = vm["ip"].as<std::string>();
    std::string program = vm["job"].as<std::string>();
    bool check_program = vm["check-program"].as<bool>();
    bool prefer_local = vm["prefer-local"].as<bool>();
    max_concurrent_jobs = vm["max-concurrent-jobs"].as<int>();

    if (threads_count < 0) {
        std::cerr << "Error: Number of threads cannot be negative.\n";
        return 1;
    } else if (threads_count == 1) {
        std::cerr << "Warning: Number of threads cannot be less than 2. Setting to 2.\n";
        threads_count = 2;
    }

    int program_location = program_exists(program);
    if (check_program && program_location == NOT_FOUND) {
        std::cerr << "Error: Program " << program << " does not exist.\n";
        return 1;
    }

    if (program_location == BOTH && prefer_local) {
        program = "./" + program;
    }

    crow::SimpleApp app;

    CROW_ROUTE(app, "/run").methods("POST"_method)
    ([&program](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON\n");
        }

        std::string flags = body["flags"].s();
        std::string command = program + " " + flags;

        std::shared_ptr<Job> job = std::make_shared<Job>();
        job->command = command;

        {
            std::lock_guard<std::mutex> lock(jobs_mutex);
            if (running_jobs >= max_concurrent_jobs) {
                return crow::response(429, "Too many concurrent jobs\n");
            }

            job->id = jobs.size() + 1;
            jobs.push_back(job);

            futures.push_back(std::async(std::launch::async, run_command, job));
        }

        return crow::response(202, "Command is running\n");
    });

    CROW_ROUTE(app, "/results").methods("GET"_method)
    ([]() {
        std::unique_lock<std::mutex> lock(jobs_mutex);
        if (jobs.empty()) {
            if (!response_old.empty()) {
                return crow::response(200, response_old);
            }
            return crow::response(200, "No job has been started. Please submit a POST request to /run.\n");
        }

        std::string response = "";
        int i = 1;
        bool allfinished = true;
        for (const auto& job : jobs) {
            auto start_time_t = std::chrono::system_clock::to_time_t(job->start_time);
            std::string start_time_str = std::ctime(&start_time_t);
            start_time_str.pop_back(); // Remove newline character

            if (job->running) {
                response += "[Job " + std::to_string(i) + "] Results not ready for process: " + job->command + " that started at " + start_time_str + "\n";
            } else {
                auto end_time_t = std::chrono::system_clock::to_time_t(job->end_time);
                std::string end_time_str = std::ctime(&end_time_t);
                end_time_str.pop_back(); // Remove newline character
                response += "[Job " + std::to_string(i) + "] Command: " + job->command + "\nStart time: " + start_time_str + "\nEnd time: " + end_time_str + "\nResult:\n" + job->result + "\n";
            }
            allfinished = allfinished && !job->running;
            i++;
        }
        if (allfinished) {
            response_old = response;
            jobs.clear();
        }
        return crow::response(200, response);
    });

    auto server_thread = std::thread([&app, ip, port, threads_count]() {
        if (threads_count == 0) {
            app.bindaddr(ip).port(port).run();
        } else {
            app.bindaddr(ip).port(port).multithreaded().concurrency(threads_count).run();
        }
    });

    server_thread.join();

    shutting_down.store(true);
    jobs_cv.notify_all();

    for (auto& future : futures) {
        future.wait();
    }

    return 0;
}