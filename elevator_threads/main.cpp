#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <algorithm>
#include <limits>
#include <mutex>
#include <condition_variable>


std::atomic<int> currentFloor(0);
std::atomic<char> direction('U');

std::vector<int> upRequests;
std::vector<int> downRequests;

std::mutex mutex;
std::condition_variable messenger;

//securing user input
std::string safecin(const std::string& prompt = "") {
    std::string input;
    while (true) {
        std::cout << prompt;
        std::cin >> input;
        
        if (input == "-1"){
            exit(0);
        }
        
        if (input.back() == 'u'){
            input.back() = 'U';
        } else if (input.back() == 'd'){
            input.back() = 'D';
        }

        if (input.length() < 2 || (input.back() != 'U' && input.back() != 'D') || !isdigit(input[0])) {
            std::cout << "Invalid input" << std::endl;
        } else {
            return input;
        }
    }
}

// Function to simulate the elevator movement
void elevatorSimulation() {
    int target = -1;
    while (true) {
        
        std::unique_lock<std::mutex> lock(mutex);
        messenger.wait(lock, [] { return !upRequests.empty() || !downRequests.empty(); });
        
        //if direction is U check if there are any floors called above current floor
        if (direction == 'U'){
            bool flag = true;
            //stop at floors who want to go up
            for (int i = 0; i < upRequests.size(); i++){
                if (upRequests[i] > currentFloor){
                    target = upRequests[i];
                    upRequests.erase(upRequests.begin() + i);
                    flag = false;
                    break;
                }
            }
            //if floor at top wants to go down
            if (flag){
                for (int i = 0; i < downRequests.size(); i++){
                    if (downRequests[i] > currentFloor){
                        target = downRequests[i];
                        downRequests.erase(downRequests.begin() + i);
                        break;
                    }
                    direction = 'D';
                }
            }
        }
        
        //if direction is D check if there are any floors called below current floor
        if (direction == 'D'){
            bool flag = true;
            //stop at floors who want to go down
            for (int i = (int)downRequests.size() - 1; i > -1; i--){
                if (downRequests[i] < currentFloor){
                    target = downRequests[i];
                    downRequests.erase(downRequests.begin() + i);
                    flag = false;
                    break;
                }
            }
            //if floor at bottom wants to go up
            if (flag){
                for (int i = (int)upRequests.size() - 1; i > -1; i--){
                    if (upRequests[i] < currentFloor){
                        target = upRequests[i];
                        upRequests.erase(upRequests.begin() + i);
                        break;
                    }
                    direction = 'U';
                }
            }
        }
        
        //go to target floor
        while (currentFloor != target){
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (direction == 'U'){
                currentFloor++;
                std::cout << "↑ ";
            } else {
                currentFloor--;
                std::cout << "↓ ";
            }
            std::cout << currentFloor << std::endl;
            lock.lock();
        }
        if (currentFloor == target){
            std::cout << "Stopped at : " << target << std::endl;
        }
    }
}

// Function to handle user input
void handleInput() {
    while (true) {
        std::string input = safecin();

        // Parse the floor and direction from input
        int floor = std::stoi(input.substr(0, input.length() - 1));
        char dir = input.back();
        bool flag = true;

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (dir == 'U') {
                for (int i = (int)upRequests.size() - 1; i > -1; i--){
                    if (upRequests[i] == floor){
                        flag = false;
                    }
                }
                if (flag) upRequests.push_back(floor);
                std::sort(upRequests.begin(), upRequests.end());
            } else if (dir == 'D') {
                for (int i = (int)downRequests.size() - 1; i > -1; i--){
                    if (downRequests[i] == floor){
                        flag = false;
                    }
                }
                if (flag) downRequests.push_back(floor);
                std::sort(downRequests.rbegin(), downRequests.rend());
            }
        }

        // Notify the elevator simulation thread that there is a new request
        messenger.notify_one();
    }
}

int main() {
    
    std::cout <<"\n~~~~~~~~~~~~~~~~ Elevator ~~~~~~~~~~~~~~~~\n";
    std::cout <<"~~~ Enter the floor number + direction ~~~\n";
    std::cout <<"~ int + \'u\'/\'d\' (up or down) eg:7u or 5D ~\n";
    std::cout <<"~~~~~~~~~~~~~~~ -1 to exit ~~~~~~~~~~~~~~~\n";

    std::thread elevatorThread(elevatorSimulation);

    handleInput();

    messenger.notify_one();

    elevatorThread.join();

    return 0;
}


