#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <set>

// Helper to get current system date (YYYY-MM-DD)
std::string getCurrentDateString() {
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    char buffer[11];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
    return std::string(buffer);
}

// Struct to represent a date and provide comparison/validation helpers
struct Date {
    int year;
    int month;
    int day;

    bool isValid() const {
        if (month < 1 || month > 12 || day < 1 || day > 31) return false;
        if (month == 2) {
            bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return day <= (isLeap ? 29 : 28);
        }
        if (month == 4 || month == 6 || month == 9 || month == 11) {
            return day <= 30;
        }
        return true;
    }

    // Returns true if this date is strictly after `other`
    bool isAfter(const Date& other) const {
        if (year != other.year) return year > other.year;
        if (month != other.month) return month > other.month;
        return day > other.day;
    }

    std::string toString() const {
        std::ostringstream ss;
        ss << year << "-" 
           << std::setw(2) << std::setfill('0') << month << "-" 
           << std::setw(2) << std::setfill('0') << day;
        return ss.str();
    }

    static Date parse(const std::string& str) {
        int y = 0, m = 0, d = 0;
        char dash1, dash2;
        std::istringstream ss(str);
        if (ss >> y >> dash1 >> m >> dash2 >> d && dash1 == '-' && dash2 == '-') {
            return {y, m, d};
        }
        return {0, 0, 0}; // Invalid marker
    }

    static Date today() {
        std::string cur = getCurrentDateString();
        return parse(cur);
    }

    // Add days to a date (useful for recurring tasks and due dates)
    Date addDays(int daysToAdd) const {
        std::tm t = {};
        t.tm_year = year - 1900;
        t.tm_mon = month - 1;
        t.tm_mday = day + daysToAdd;
        std::time_t timeEpoch = std::mktime(&t);
        std::tm* updated = std::localtime(&timeEpoch);
        return {updated->tm_year + 1900, updated->tm_mon + 1, updated->tm_mday};
    }
    
    // Difference in days between *this* date and `earlier` date
    int daysDifference(const Date& earlier) const {
        std::tm t1 = {};
        t1.tm_year = year - 1900;
        t1.tm_mon = month - 1;
        t1.tm_mday = day;
        
        std::tm t2 = {};
        t2.tm_year = earlier.year - 1900;
        t2.tm_mon = earlier.month - 1;
        t2.tm_mday = earlier.day;

        std::time_t epoch1 = std::mktime(&t1);
        std::time_t epoch2 = std::mktime(&t2);
        
        double diffSeconds = std::difftime(epoch1, epoch2);
        return static_cast<int>(diffSeconds / (60 * 60 * 24));
    }
};

// Base Task Class
class Task {
protected:
    int tid;
    std::string title;
    std::string description;
    Date dueDate;
    bool completed;

public:
    Task(int tid, std::string title, std::string description, Date dueDate, bool completed = false)
        : tid(tid), title(title), description(description), dueDate(dueDate), completed(completed) {}

    virtual ~Task() = default;

    int getTid() const { return tid; }
    std::string getTitle() const { return title; }
    std::string getDescription() const { return description; }
    Date getDueDate() const { return dueDate; }
    bool isCompleted() const { return completed; }

    void setTitle(const std::string& t) { if (!t.empty()) title = t; }
    void setDescription(const std::string& d) { if (!d.empty()) description = d; }
    void setDueDate(Date d) { dueDate = d; }
    void setCompleted(bool status) { completed = status; }

    virtual std::string getTypeString() const { return "Normal"; }
    virtual int getRecurDays() const { return 0; }

    virtual void checkRecurringReset() {
        // Normal tasks do not auto-reset
    }

    virtual void display() const {
        std::cout << "----------------------------------------\n";
        std::cout << "Task ID     : " << tid << "\n";
        std::cout << "Type        : " << getTypeString() << "\n";
        std::cout << "Title       : " << title << "\n";
        std::cout << "Description : " << description << "\n";
        std::cout << "Due Date    : " << dueDate.toString() << "\n";
        std::cout << "Status      : " << (completed ? "Completed" : "Pending") << "\n";
    }

    virtual std::string serialize() const {
        // Format: TYPE|TID|TITLE|DESC|DUEDATE|COMPLETED|RECURDAYS
        return "NORMAL|" + std::to_string(tid) + "|" + title + "|" + description + "|" + dueDate.toString() + "|" + (completed ? "1" : "0");
    }
};

// Derived Recurring Task Class
class RecurringTask : public Task {
private:
    int recurDays; // Between 1 and 30 days

public:
    RecurringTask(int tid, std::string title, std::string description, Date dueDate, int recurDays, bool completed = false)
        : Task(tid, title, description, dueDate, completed), recurDays(recurDays) {}

    std::string getTypeString() const override { return "Recurring"; }
    int getRecurDays() const override { return recurDays; }

    // Automatically check application start / system date and reset to pending if recurrence period elapsed
    void checkRecurringReset() override {
        if (completed) {
            Date today = Date::today();
            // If today is past or equal to the due date, check if recurrence cycle passed
            int daysSinceDue = today.daysDifference(dueDate);
            if (daysSinceDue >= 0 && (daysSinceDue % recurDays == 0 || daysSinceDue >= recurDays)) {
                completed = false;
                // Shift due date forward by recurrence intervals until it's in the future or matches today
                while (!dueDate.isAfter(today)) {
                    dueDate = dueDate.addDays(recurDays);
                }
            }
        }
    }

    void display() const override {
        Task::display();
        std::cout << "Recur Days  : Every " << recurDays << " days\n";
    }

    std::string serialize() const override {
        return "RECURRING|" + std::to_string(tid) + "|" + title + "|" + description + "|" + dueDate.toString() + "|" + (completed ? "1" : "0") + "|" + std::to_string(recurDays);
    }
};

// TodoManager handles collection logic and file persistence
class TodoManager {
private:
    std::vector<std::unique_ptr<Task>> tasks;
    const std::string filename = "tasks.txt";
    int nextId = 1;

    void updateNextId() {
        int maxId = 0;
        for (const auto& task : tasks) {
            if (task->getTid() > maxId) {
                maxId = task->getTid();
            }
        }
        nextId = maxId + 1;
    }

public:
    TodoManager() {
        loadFromFile();
        checkAllRecurringTasks();
    }

    void checkAllRecurringTasks() {
        for (auto& task : tasks) {
            task->checkRecurringReset();
        }
    }

    void loadFromFile() {
        std::ifstream file(filename);
        if (!file.is_open()) return;

        tasks.clear();
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            std::stringstream ss(line);
            std::string type, title, desc, dateStr;
            int tid, recurDays = 0;
            bool completed;
            char delimiter = '|';

            std::getline(ss, type, delimiter);
            
            std::string tidStr;
            std::getline(ss, tidStr, delimiter);
            if (tidStr.empty()) continue;
            tid = std::stoi(tidStr);

            std::getline(ss, title, delimiter);
            std::getline(ss, desc, delimiter);
            std::getline(ss, dateStr, delimiter);
            
            std::string compStr;
            std::getline(ss, compStr, delimiter);
            completed = (compStr == "1");

            Date d = Date::parse(dateStr);

            if (type == "RECURRING") {
                std::string recurStr;
                std::getline(ss, recurStr, delimiter);
                if (!recurStr.empty()) {
                    recurDays = std::stoi(recurStr);
                }
                tasks.push_back(std::make_unique<RecurringTask>(tid, title, desc, d, recurDays, completed));
            } else {
                tasks.push_back(std::make_unique<Task>(tid, title, desc, d, completed));
            }
        }
        file.close();
        updateNextId();
    }

    void saveToFile() {
        std::ofstream file(filename);
        for (const auto& task : tasks) {
            file << task->serialize() << "\n";
        }
        file.close();
    }

    void addTaskPrompt() {
        int choice;
        std::cout << "\n--- Add New Task ---\n";
        std::cout << "1. Normal Task\n";
        std::cout << "2. Recurring Task\n";
        std::cout << "Select task type (1-2): ";
        std::cin >> choice;
        std::cin.ignore();

        std::string title, description;
        std::cout << "Enter Title (2-3 words recommended): ";
        std::getline(std::cin, title);

        std::cout << "Enter Description (detailed paragraph): ";
        std::getline(std::cin, description);

        Date dueDate;
        while (true) {
            std::string dateInput;
            std::cout << "Enter Due Date (YYYY-MM-DD): ";
            std::getline(std::cin, dateInput);
            dueDate = Date::parse(dateInput);

            if (!dueDate.isValid()) {
                std::cout << "Invalid format or non-existent date. Please use YYYY-MM-DD.\n";
                continue;
            }

            Date today = Date::today();
            if (!dueDate.isAfter(today)) {
                std::cout << "Due date must exceed today's date (" << today.toString() << "). Try again.\n";
                continue;
            }
            break;
        }

        if (choice == 2) {
            int recurDays = 0;
            while (true) {
                std::cout << "Enter Recurring Duration in days (between 2 and 30): ";
                std::cin >> recurDays;
                std::cin.ignore();
                if (recurDays >= 2 && recurDays <= 30) {
                    break;
                }
                std::cout << "Invalid duration. Must be > 1 and <= 30 days.\n";
            }
            tasks.push_back(std::make_unique<RecurringTask>(nextId++, title, description, dueDate, recurDays, false));
            std::cout << "Recurring Task added successfully with ID: " << (nextId - 1) << "\n";
        } else {
            tasks.push_back(std::make_unique<Task>(nextId++, title, description, dueDate, false));
            std::cout << "Normal Task added successfully with ID: " << (nextId - 1) << "\n";
        }
    }

    void viewTasksPrompt() const {
        if (tasks.empty()) {
            std::cout << "No tasks available.\n";
            return;
        }

        std::cout << "\n--- View Tasks ---\n";
        std::cout << "a. All (sort by ID)\n";
        std::cout << "b. All (sort by due date, earlier first)\n";
        std::cout << "c. Pending tasks only\n";
        std::cout << "d. Completed tasks only\n";
        std::cout << "e. By specific Task ID(s)\n";
        std::cout << "Enter view option (a-e): ";
        char opt;
        std::cin >> opt;
        std::cin.ignore();

        if (opt == 'a') {
            std::vector<Task*> sortedTasks;
            for (const auto& t : tasks) sortedTasks.push_back(t.get());
            std::sort(sortedTasks.begin(), sortedTasks.end(), [](Task* a, Task* b) {
                return a->getTid() < b->getTid();
            });
            for (auto t : sortedTasks) t->display();
        } 
        else if (opt == 'b') {
            std::vector<Task*> sortedTasks;
            for (const auto& t : tasks) sortedTasks.push_back(t.get());
            std::sort(sortedTasks.begin(), sortedTasks.end(), [](Task* a, Task* b) {
                Date da = a->getDueDate();
                Date db = b->getDueDate();
                if (da.year != db.year) return da.year < db.year;
                if (da.month != db.month) return da.month < db.month;
                return da.day < db.day;
            });
            for (auto t : sortedTasks) t->display();
        } 
        else if (opt == 'c') {
            bool found = false;
            for (const auto& t : tasks) {
                if (!t->isCompleted()) {
                    t->display();
                    found = true;
                }
            }
            if (!found) std::cout << "No pending tasks found.\n";
        } 
        else if (opt == 'd') {
            bool found = false;
            for (const auto& t : tasks) {
                if (t->isCompleted()) {
                    t->display();
                    found = true;
                }
            }
            if (!found) std::cout << "No completed tasks found.\n";
        } 
        else if (opt == 'e') {
            std::cout << "Enter Task IDs separated by space: ";
            std::string line;
            std::getline(std::cin, line);
            std::stringstream ss(line);
            int id;
            while (ss >> id) {
                auto it = std::find_if(tasks.begin(), tasks.end(), [id](const auto& t) {
                    return t->getTid() == id;
                });
                if (it != tasks.end()) {
                    (*it)->display();
                } else {
                    std::cout << "Task ID " << id << " not found.\n";
                }
            }
        } else {
            std::cout << "Invalid option.\n";
        }
    }

    void deleteTaskPrompt() {
        if (tasks.empty()) {
            std::cout << "No tasks to delete.\n";
            return;
        }

        std::cout << "\n--- Delete Task ---\n";
        std::cout << "a. By Task ID (can take multiple separated by space)\n";
        std::cout << "b. Delete completed normal tasks (preserves completed recurring tasks)\n";
        std::cout << "Choose deletion option (a-b): ";
        char opt;
        std::cin >> opt;
        std::cin.ignore();

        if (opt == 'a') {
            std::cout << "Enter Task IDs to delete separated by space: ";
            std::string line;
            std::getline(std::cin, line);
            std::stringstream ss(line);
            std::set<int> idsToDelete;
            int id;
            while (ss >> id) {
                idsToDelete.insert(id);
            }

            size_t initialSize = tasks.size();
            tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [&idsToDelete](const auto& t) {
                return idsToDelete.count(t->getTid()) > 0;
            }), tasks.end());

            std::cout << "Deleted " << (initialSize - tasks.size()) << " task(s).\n";
        } 
        else if (opt == 'b') {
            size_t initialSize = tasks.size();
            tasks.erase(std::remove_if(tasks.begin(), tasks.end(), [](const auto& t) {
                // Keep if it's NOT completed OR if it IS a recurring task
                bool isRecurring = (t->getTypeString() == "Recurring");
                bool completed = t->isCompleted();
                return completed && !isRecurring;
            }), tasks.end());

            std::cout << "Cleared completed normal tasks. Remaining tasks: " << tasks.size() << "\n";
        } else {
            std::cout << "Invalid deletion option.\n";
        }
    }

    void searchTaskPrompt() const {
        std::string query;
        std::cout << "\n--- Search Task ---\n";
        std::cout << "Enter a single search keyword: ";
        std::cin >> query;
        std::cin.ignore();

        bool found = false;
        for (const auto& t : tasks) {
            // Search across title, description, or id string representation
            std::string combined = std::to_string(t->getTid()) + " " + t->getTitle() + " " + t->getDescription() + " " + t->getDueDate().toString();
            if (combined.find(query) != std::string::npos) {
                std::cout << "[Match Found] Task ID: " << t->getTid() << " | Details: " << t->getTitle() << " (" << t->getDueDate().toString() << ")\n";
                found = true;
            }
        }
        if (!found) {
            std::cout << "No matches found for keyword: " << query << "\n";
        }
    }

    void changeTaskStatusPrompt() {
        std::cout << "\n--- Change Task Status ---\n";
        std::cout << "Enter Task IDs separated by space to reverse their status: ";
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        int id;
        while (ss >> id) {
            auto it = std::find_if(tasks.begin(), tasks.end(), [id](const auto& t) {
                return t->getTid() == id;
            });
            if (it != tasks.end()) {
                bool currentStatus = (*it)->isCompleted();
                (*it)->setCompleted(!currentStatus);
                std::cout << "Task ID " << id << " status changed to: " << (!currentStatus ? "Completed" : "Pending") << "\n";
            } else {
                std::cout << "Task ID " << id << " not found.\n";
            }
        }
    }

    void updateTaskPrompt() {
        std::cout << "\n--- Update Task ---\n";
        int id;
        std::cout << "Enter Task ID to update: ";
        std::cin >> id;
        std::cin.ignore();

        auto it = std::find_if(tasks.begin(), tasks.end(), [id](const auto& t) {
            return t->getTid() == id;
        });

        if (it == tasks.end()) {
            std::cout << "Task not found.\n";
            return;
        }

        std::cout << "\nCurrent Task Details:\n";
        (*it)->display();

        std::string newTitle, newDesc, newDateStr;

        std::cout << "\nEnter new Title (or press Enter to keep old): ";
        std::getline(std::cin, newTitle);
        if (!newTitle.empty()) {
            (*it)->setTitle(newTitle);
        }

        std::cout << "Enter new Description (or press Enter to keep old): ";
        std::getline(std::cin, newDesc);
        if (!newDesc.empty()) {
            (*it)->setDescription(newDesc);
        }

        std::cout << "Enter new Due Date (YYYY-MM-DD) (or press Enter to keep old): ";
        std::getline(std::cin, newDateStr);
        if (!newDateStr.empty()) {
            Date newDate = Date::parse(newDateStr);
            if (newDate.isValid() && newDate.isAfter(Date::today())) {
                (*it)->setDueDate(newDate);
                std::cout << "Due date updated successfully.\n";
            } else {
                std::cout << "Invalid date or date does not exceed today. Keeping previous due date.\n";
            }
        }

        std::cout << "Task updated successfully!\n";
    }

    void saveAndExit() {
        saveToFile();
        std::cout << "All tasks saved successfully to 'tasks.txt'. Exiting application.\n";
    }
};

int main() {
    TodoManager manager;
    int choice = 0;

    do {
        std::cout << "\n===============================\n";
        std::cout << "      TO-DO COMMAND LINE       \n";
        std::cout << "===============================\n";
        std::cout << "1. Add new task\n";
        std::cout << "2. View task\n";
        std::cout << "3. Delete task\n";
        std::cout << "4. Search task\n";
        std::cout << "5. Change task status\n";
        std::cout << "6. Update task\n";
        std::cout << "7. Save and exit\n";
        std::cout << "Enter your choice (1-7): ";
        
        if (!(std::cin >> choice)) {
            std::cout << "Invalid input. Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            continue;
        }

        switch (choice) {
            case 1:
                manager.addTaskPrompt();
                break;
            case 2:
                manager.viewTasksPrompt();
                break;
            case 3:
                manager.deleteTaskPrompt();
                break;
            case 4:
                manager.searchTaskPrompt();
                break;
            case 5:
                manager.changeTaskStatusPrompt();
                break;
            case 6:
                manager.updateTaskPrompt();
                break;
            case 7:
                manager.saveAndExit();
                break;
            default:
                std::cout << "Invalid choice! Please select between 1 and 7.\n";
        }
    } while (choice != 7);

    return 0;
}
