#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

// время
struct Time {
    int hours;
    int minutes;

    Time() : hours(0), minutes(0) {}
    Time(int h, int m) : hours(h), minutes(m) {}

    static Time fromString(const std::string& timeStr) {
        if (timeStr.length() != 5 || timeStr[2] != ':') {
            std::cout <<"Invalid time format";
        }

        int h = std::stoi(timeStr.substr(0, 2));
        int m = std::stoi(timeStr.substr(3, 2));

        if (h < 0 || h > 23 || m < 0 || m > 59) {
            std::cout << "Invalid time value";
        }

        return Time(h, m);
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hours << ":"
            << std::setw(2) << std::setfill('0') << minutes;
        return oss.str();
    }

    bool operator<(const Time& other) const {
        if (hours != other.hours)
            return hours < other.hours;
        return minutes < other.minutes;
    }

    bool operator!=(const Time& other) const {
        return !(*this == other);
    }

    bool operator<=(const Time& other) const {
        return !(*this > other);
    }

    bool operator>(const Time& other) const {
        if (hours != other.hours)
            return hours > other.hours;
        return minutes > other.minutes;
    }

    bool operator>=(const Time& other) const {
        return !(*this < other);
    }

    bool operator==(const Time& other) const {
        return hours == other.hours && minutes == other.minutes;
    }

    // Вычисляет разницу в минутах между двумя временными точками
    int minutesDifference(const Time& other) const {
        int totalMinutesThis = hours * 60 + minutes;
        int totalMinutesOther = other.hours * 60 + other.minutes;
        return totalMinutesThis - totalMinutesOther;
    }
};

// событие
struct Event {
    Time time;
    int id;
    std::vector<std::string> data;
    
    // Для сортировки событиий
    bool operator<(const Event& other) const {
        if (time != other.time)
            return time < other.time;
        if (id != other.id)
            return id < other.id;
        return false; // события с одинаковым временем и ID считаются равными
    }
};

class ClubSystem {
public:
ClubSystem(Time openTime, Time closeTime, int hourlyRate, int tableCount)
    : openTime(openTime), 
      closeTime(closeTime), 
      hourlyRate(hourlyRate), 
      tableCount(tableCount),
      tables(std::vector<TableInfo>(tableCount)) // <-- Вот эта строчка!
{}

    void processEvents(const std::vector<Event>& inputEvents) {
        // Сортируем входящие события по времени
        events = inputEvents;
        std::sort(events.begin(), events.end());

        for (const auto& event : events) {
            // Проверяем, работает ли клуб в это время

            bool isClubOpen = (event.time >= openTime && event.time <= closeTime);
            //std::cout << event.time.toString() << " " << event.id << " " << event.data[0] << std::endl;

            if (event.id == 1) { // Клиент пришел
                handleClientArrival(event, isClubOpen);
                
            } else if (event.id == 2) { // Клиент сел за стол
                handleClientSitDown(event);
            } else if (event.id == 3) { // Клиент ожидает
                handleClientWaiting(event);
            } else if (event.id == 4) { // Клиент ушел
                handleClientLeaving(event);
            }
        }

        // Обрабатываем завершение рабочего дня
        handleEndOfDay();
    }

    void printResults() const {
        // Выводим время начала работы
        std::cout << openTime.toString() << std::endl;

        // Выводим все обработанные события
        for (const auto& event : processedEvents) {
            std::cout << event.time.toString() << " " << event.id;
            for (const auto& item : event.data) {
                std::cout << " " << item;
            }
            std::cout << std::endl;
        }

        // Выводим время окончания работы
        std::cout << closeTime.toString() << std::endl;

        // Выводим информацию по каждому столу
        for (int i = 0; i < tableCount; ++i) {
            const TableInfo& info = tables[i];
            Time usageTime(0, info.totalMinutes);
            usageTime.hours = info.totalMinutes / 60;
            usageTime.minutes = info.totalMinutes % 60;
        
            std::cout << (i + 1) << " " << info.revenue << " " << usageTime.toString() << std::endl;
        }
    }

    void addProcessedEvent(const Event& event) {
        processedEvents.push_back(event);
    }

private:
struct ClientInfo {
    std::string name;
    int currentTable = -1; // клиент не сидит за столом
    Time sessionStart;

    ClientInfo() = default; // Теперь можно создавать без имени
    explicit ClientInfo(const std::string& name) : name(name) {}
};

    struct TableInfo {
        int totalMinutes = 0; // общее время использования в минутах
        int revenue = 0; // доход от стола
    };

    Time openTime;
    Time closeTime;
    int hourlyRate;
    int tableCount;
    std::vector<TableInfo> tables;
    std::map<std::string, ClientInfo> clients;
    std::vector<Event> events;
    std::vector<Event> processedEvents;
    std::queue<std::string> waitingQueue;

    void handleClientArrival(const Event& event, bool isClubOpen) {
        std::string clientName = event.data[0];
        //std::cout << "Hello";
        // Проверяем, существует ли уже такой клиент
        //auto [it, inserted] = clients.insert({clientName, ClientInfo(clientName)});
        auto it = clients.find(clientName);
        if (it != clients.end()) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "YouShallNotPass");
            return;
        }
        

        // Проверяем работает ли клуб
        if (!isClubOpen) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "NotOpenYet");
            return;
        }

        // Добавляем клиента
        clients[clientName] = ClientInfo(clientName);
        addProcessedEvent(event);
    }

    void handleClientSitDown(Event event) {
        std::string clientName = event.data[0];
        int desiredTable = std::stoi(event.data[1]) - 1; // преобразуем в индекс (начиная с 0)

        // Проверяем корректность номера стола
        if (desiredTable < 0 || desiredTable >= tableCount) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "PlaceIsBusy"); // произвольная ошибка
            return;
        }

        // Проверяем, существует ли такой клиент
        auto clientIt = clients.find(clientName);
        if (clientIt == clients.end()) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "ClientUnknown");
            return;
        }

        ClientInfo& client = clientIt->second;

        // Проверяем, занят ли выбранный стол
        if (tables[desiredTable].totalMinutes >= 0 && 
            isClientAtTable(desiredTable) && 
            (client.currentTable != desiredTable)) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "PlaceIsBusy");
            return;
        }

        // Если клиент уже за столом, освобождаем предыдущий стол
        if (client.currentTable != -1) {
            // Обновляем время использования стола
            int minutesUsed = event.time.minutesDifference(client.sessionStart);
            tables[client.currentTable].totalMinutes += minutesUsed;
        }

        // Сажаем клиента за новый стол
        client.currentTable = desiredTable;
        client.sessionStart = event.time;

        addProcessedEvent(event);
    }

    void handleClientWaiting(Event event) {
        std::string clientName = event.data[0];
    
        // Проверяем, существует ли такой клиент
        auto clientIt = clients.find(clientName);
        if (clientIt == clients.end()) {
            addProcessedEvent(event); // <-- Выводим событие
            addErrorEvent(event.time, "ClientUnknown");
            return;
        }
        ClientInfo& client = clientIt->second;
    
        // Проверяем, есть ли свободные места
        bool hasFreeTable = false;
        for (int i = 0; i < tableCount; ++i) {
            if (!isClientAtTable(i)) {
                hasFreeTable = true;
                break;
            }
        }
    
        if (hasFreeTable) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "ICanWaitNoLonger!");
            return;
        }
    
        // Проверяем длину очереди
        if (waitingQueue.size() >= static_cast<size_t>(tableCount)) {
            // Клиент уходит
            Event leaveEvent;
            leaveEvent.time = event.time;
            leaveEvent.id = 11;
            leaveEvent.data = {clientName};
    
            addProcessedEvent(event);
            addProcessedEvent(leaveEvent); 
    
            clients.erase(clientName); // Удаляем клиента из системы
            return;
        }
    
        waitingQueue.push(clientName);
        addProcessedEvent(event);
    }

    void handleClientLeaving(Event event) {
        std::string clientName = event.data[0];

        // Проверяем, существует ли такой клиент
        auto clientIt = clients.find(clientName);
        if (clientIt == clients.end()) {
            addProcessedEvent(event);
            addErrorEvent(event.time, "ClientUnknown");
            return;
        }

        ClientInfo& client = clientIt->second;

        // Если клиент сидел за столом, освобождаем его
        if (client.currentTable != -1) {
            // Обновляем время использования стола
            int minutesUsed = event.time.minutesDifference(client.sessionStart);
            int hoursBilled = (minutesUsed + 59) / 60; // округляем вверх
            tables[client.currentTable].totalMinutes += minutesUsed;
            tables[client.currentTable].revenue += hoursBilled * hourlyRate;
            client.currentTable = -1;
        }

        // Удаляем клиента из системы
        clients.erase(clientName);

        addProcessedEvent(event);

        // Если есть клиенты в очереди, сажаем первого
        if (!waitingQueue.empty()) {
            std::string waitingClient = waitingQueue.front();
            waitingQueue.pop();

            Event sitDownEvent;
            sitDownEvent.time = event.time;
            sitDownEvent.id = 12;
            sitDownEvent.data = {waitingClient, ""}; // Номер стола будет определен позже

            // Находим свободный стол
            int freeTable = -1;
            for (int i = 0; i < tableCount; ++i) {
                if (!isClientAtTable(i)) {
                    freeTable = i;
                    break;
                }
            }

            if (freeTable != -1) {
                sitDownEvent.data.push_back(std::to_string(freeTable + 1)); // используем номер стола (начинается с 1)
                addProcessedEvent(sitDownEvent);

                // Обновляем информацию о клиенте
                clients[waitingClient].currentTable = freeTable;
                clients[waitingClient].sessionStart = event.time;
            }
        }
    }

    void handleEndOfDay() {
        // Собираем имена всех клиентов, чтобы отсортировать их
        std::vector<std::string> clientNames;
        for (const auto& pair : clients) {
            clientNames.push_back(pair.first);
        }
    
        // Сортируем имена в алфавитном порядке
        std::sort(clientNames.begin(), clientNames.end());
    
        // Для каждого клиента генерируем событие ухода
        for (const auto& clientName : clientNames) {
            Event leaveEvent;
            leaveEvent.time = closeTime;
            leaveEvent.id = 11;
            leaveEvent.data = {clientName};
            addProcessedEvent(leaveEvent);
    
            // Если клиент сидел за столом, обновляем время использования
            if (clients[clientName].currentTable != -1) {
                int minutesUsed = closeTime.minutesDifference(clients[clientName].sessionStart);
                int hoursBilled = (minutesUsed + 59) / 60; // округляем вверх
                int tableIndex = clients[clientName].currentTable;
                tables[tableIndex].totalMinutes += minutesUsed;
                tables[tableIndex].revenue += hoursBilled * hourlyRate;
            }
        }
    
        // Очищаем список клиентов
        clients.clear();
    }

    void addErrorEvent(const Time& time, const std::string& errorMessage) {
        Event errorEvent;
        errorEvent.time = time;
        errorEvent.id = 13;
        errorEvent.data = {errorMessage};
        addProcessedEvent(errorEvent);
    }

    bool isClientAtTable(int tableIndex) const {
        for (const auto& pair : clients) {
            if (pair.second.currentTable == tableIndex)
                return true;
        }
        return false;
    }
};

bool isValidName(const std::string& name) {
    for (char c : name) {
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
    }
    return true;
}

void parseInputFile(const std::string& filename, 
                   int& tableCount, 
                   Time& openTime, 
                   Time& closeTime, 
                   int& hourlyRate,
                   std::vector<Event>& events,
                   std::string& errorLine) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std:: cout << "Cannot open file: " << filename;
    }

    std::string line;
    int lineNumber = 0;
    
    // Чтение количества столов
    if (!std::getline(file, line)) {
        std:: cout << "Failed to read number of tables";
    }
    
    lineNumber++;
    try {
        tableCount = std::stoi(line);
        if (tableCount <= 0) {
            errorLine = line;
            std:: cout << "Invalid number of tables on line " << std::to_string(lineNumber);
        }
    } catch (...) {
        errorLine = line;
        std:: cout << "Invalid number of tables on line " << std::to_string(lineNumber);
    }
    
    // Чтение времени работы
    if (!std::getline(file, line)) {
        std:: cout << "Failed to read opening/closing time";
    }
    
    lineNumber++;
    std::istringstream timeStream(line);
    std::string openTimeStr, closeTimeStr;
    if (!(timeStream >> openTimeStr >> closeTimeStr)) {
        errorLine = line;
        std:: cout << "Invalid time format on line " << std::to_string(lineNumber);
    }

    try {
        openTime = Time::fromString(openTimeStr);
        closeTime = Time::fromString(closeTimeStr);
    } catch (...) {
        errorLine = line;
        std:: cout << "Invalid time format on line " << std::to_string(lineNumber);
    }

    if (openTime >= closeTime) {
        errorLine = line;
        std:: cout << "Opening time must be earlier than closing time on line " << std::to_string(lineNumber);
    }

    // Чтение стоимости часа
    if (!std::getline(file, line)) {
        std:: cout << "Failed to read hourly rate";
    }
    lineNumber++;
    try {
        hourlyRate = std::stoi(line);
        if (hourlyRate <= 0) {
            errorLine = line;
            std:: cout << "Hourly rate must be positive on line " << std::to_string(lineNumber);
        }
    } catch (...) {
        errorLine = line;
        std:: cout << "Invalid hourly rate on line " << std::to_string(lineNumber);
    }

    // Чтение событий
    while (std::getline(file, line)) {
        lineNumber++;
        if (line.empty()) continue;
        //std:: cout << "Hello";
        std::istringstream eventStream(line);
        std::string timeStr;
        int eventId;
        if (!(eventStream >> timeStr >> eventId)) {
            errorLine = line;
            std:: cout << "Invalid event format on line " << std::to_string(lineNumber);
        }

        try {
            Time eventTime = Time::fromString(timeStr);

            Event event;
            event.time = eventTime;
            event.id = eventId;

            std::string token;
            while (eventStream >> token) {
                event.data.push_back(token);
            }

            // Проверка формата данных события
            if (eventId == 1) { // Клиент пришел
                if (event.data.size() != 1 || !isValidName(event.data[0])) {
                    errorLine = line;
                    std:: cout << "Invalid client arrival event on line " << std::to_string(lineNumber);
                }
            
            } else if (eventId == 2) { // Клиент сел за стол
                if (event.data.size() != 2 || !isValidName(event.data[0])) {
                    errorLine = line;
                    std:: cout << "Invalid client sit down event on line " << std::to_string(lineNumber);
                }
                // Номер стола проверяется позже, так как мы не знаем общее количество столов на этом этапе
            } else if (eventId == 3) { // Клиент ожидает
                if (event.data.size() != 1 || !isValidName(event.data[0])) {
                    errorLine = line;
                    std:: cout << "Invalid client wait event on line " << std::to_string(lineNumber);
                }
            } else if (eventId == 4) { // Клиент ушел
                if (event.data.size() != 1 || !isValidName(event.data[0])) {
                    errorLine = line;
                    std:: cout << "Invalid client leave event on line " << std::to_string(lineNumber);
                }
            } else if (eventId == 11 || eventId == 12) { // Исходящие события
                // Эти события генерируются программой, поэтому они не должны встречаться во входном файле
                errorLine = line;
                std:: cout << "Outgoing event found in input on line " << std::to_string(lineNumber);
            } else {
                errorLine = line;
                std:: cout << "Unknown event type on line " << std::to_string(lineNumber);
            }

            events.push_back(event);
        } catch (const std::exception& e) {
            errorLine = line;
            std:: cout << "Error parsing event on line " << std::to_string(lineNumber) + ": " << e.what();
        }
    }
    //std::cout << "Reading input file..." << std::endl;
}

int main() {

    std::cout << "Welcome to the Club System!" << std::endl;
    /*
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
*/
    std::string filename = "test.txt";
    int tableCount = 0;
    Time openTime, closeTime;
    int hourlyRate = 0;
    std::vector<Event> events;
    std::string errorLine;
    //std::cout << "Reading input file..." << std::endl;
    try {
        parseInputFile(filename, tableCount, openTime, closeTime, hourlyRate, events, errorLine);
    } catch (const std::exception& e) {
        std::cerr << errorLine << std::endl;
        return 1;
    }
    //std::cout << "Input file read successfully." << std::endl;
    ClubSystem club(openTime, closeTime, hourlyRate, tableCount);
    club.processEvents(events);
    //std::cout << "Processing completed successfully." << std::endl;
    club.printResults();

    return 0;
}
