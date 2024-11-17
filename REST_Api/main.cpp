#include "/home/pascal/Übungen/Übung/REST_API_Übung/my_crow_api/Crow/include/crow.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <cstdlib>  // Für system() Befehl
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>
#include <csignal>
#include <vector>

// Sinuskurven-Daten
std::atomic<bool> stop_thread(false);
std::atomic<bool> active(true);
std::thread sinus_thread;

// Sinuskurven-Thread-Funktion
void shared_sinus_message_sender(std::vector<crow::websocket::connection*>& connections, 
                                 double frequency, 
                                 double value_min, 
                                 double value_max, 
                                 std::atomic<bool>& stop_thread, 
                                 std::mutex& conn_mutex) {
    double time = 0.0;
    while (active && !stop_thread) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Kurze Pause zwischen den Nachrichten

        // Sinuswert berechnen
        double amplitude = (value_max - value_min) / 2.0;
        double offset = (value_max + value_min) / 2.0;
        double value = amplitude * std::sin(frequency * time) + offset;
        time += 0.01;

        // JSON-Nachricht erstellen
        crow::json::wvalue message;
        message["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        message["value"] = value;

        // Nachricht an alle Verbindungen senden
        std::lock_guard<std::mutex> lock(conn_mutex);
        for (auto conn_ptr : connections) {
            try {
                conn_ptr->send_text(message.dump());
            } catch (const std::exception& e) {
                std::cerr << "Fehler beim Senden an Client: " << e.what() << std::endl;
                active = false;
            }
        }
    }
}

// Struktur zum Speichern des Status von Datenservern
struct DataServerStatus {
    bool is_running;
    std::string info;
};

auto aggregator_status = std::make_shared<bool>(false);
auto data_servers_status = std::make_shared<std::unordered_map<int, DataServerStatus>>();

int main() {
    crow::App<> app;

    // Endpunkt zum Abrufen des Status
    CROW_ROUTE(app, "/status")([aggregator_status, data_servers_status]() {
        crow::json::wvalue status;
        status["aggregator_running"] = *aggregator_status;

        crow::json::wvalue::object servers_json;
        for (const auto& [id, server_status] : *data_servers_status) {
            crow::json::wvalue server;
            server["is_running"] = server_status.is_running;
            server["info"] = server_status.info;
            servers_json[std::to_string(id)] = std::move(server);
        }
        status["data_servers"] = std::move(servers_json);

        return crow::response{status};
    });

    // Endpunkt zum Starten des Aggregators
    CROW_ROUTE(app, "/start_aggregator").methods("POST"_method)(
        [aggregator_status](const crow::request& req) {
            if (*aggregator_status) {
                return crow::response{400, "Aggregator is already running"};
            }

            *aggregator_status = true;
            int result = system("/home/pascal/WebSocket_Übung/Aggregator/build/server > /home/pascal/aggregator.log 2>&1 &");
            if (result == -1) {
                return crow::response{500, "Failed to start aggregator"};
            }

            return crow::response{200, "Aggregator started successfully"};
        });

    // Endpunkt zum Starten eines neuen Datenservers
    CROW_ROUTE(app, "/start_data_server").methods("POST"_method)(
        [data_servers_status](const crow::request& req) {
            auto payload = crow::json::load(req.body);
            if (!payload || !payload.has("id") || !payload.has("info")) {
                return crow::response{400, "Invalid JSON payload"};
            }

            int server_id = payload["id"].i();
            std::string server_info = payload["info"].s();

            if (data_servers_status->count(server_id) > 0) {
                return crow::response{400, "Data server with this ID already exists"};
            }

            (*data_servers_status)[server_id] = {true, server_info};

            // Starten des Sinuskurven-Threads, wenn der Server gestartet wird
            static std::mutex conn_mutex;
            static std::vector<crow::websocket::connection*> connections;

            if (!sinus_thread.joinable()) {
                sinus_thread = std::thread(shared_sinus_message_sender, std::ref(connections), 1.0, 48.0, 52.0, std::ref(stop_thread), std::ref(conn_mutex));
                std::cout << "Sinuskurven-Thread gestartet!" << std::endl;
            }

            int result = system("/home/pascal/WebSocket_Übung/Datenserver/build/server > /home/pascal/data_server.log 2>&1 &");
            if (result == -1) {
                return crow::response{500, "Failed to start data server"};
            }

            return crow::response{200, "Data server started successfully"};
        });

    // Endpunkt zum Stoppen eines Datenservers
    CROW_ROUTE(app, "/stop_data_server/<int>").methods("POST"_method)(
        [data_servers_status](int server_id) {
            if (data_servers_status->count(server_id) == 0) {
                return crow::response{404, "Data server not found"};
            }

            data_servers_status->at(server_id).is_running = false;
            return crow::response{200, "Data server stopped successfully"};
        });

    // Endpunkt zum Stoppen des Aggregators
    CROW_ROUTE(app, "/stop_aggregator").methods("POST"_method)(
        [aggregator_status](const crow::request& req) {
            if (!*aggregator_status) {
                return crow::response{400, "Aggregator is not running"};
            }

            *aggregator_status = false;
            return crow::response{200, "Aggregator stopped successfully"};
        });

    app.port(18080).multithreaded().run();
}

