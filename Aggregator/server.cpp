#include <crow.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <iostream>
#include <string>
#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <unordered_set> // Hinzugefügt für std::unordered_set
#include <vector>
#include <chrono>
#include <uuid/uuid.h>
#include <json.hpp>

using json = nlohmann::json;
using namespace std::chrono_literals;

// Ein einfacher Ringbuffer
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity = 1000)
        : size(capacity), index(0), buffer(capacity) {}

    void push(const json& value) {
        std::lock_guard<std::mutex> lock(mutex);
        buffer[index] = value;
        index = (index + 1) % size;
    }

    std::vector<json> get_all() const {
        std::lock_guard<std::mutex> lock(mutex);
        return buffer;
    }

private:
    size_t size;
    size_t index;
    mutable std::mutex mutex;
    std::vector<json> buffer;
};

// Der Hauptserver
class WebSocketServer {
public:
    WebSocketServer() {
        // Initialisiere Crow
        crow::SimpleApp app;

        // REST-API zum Starten des Servers
        CROW_ROUTE(app, "/start")
        ([this]() {
            std::lock_guard<std::mutex> lock(mutex);
            if (!is_running) {
                is_running = true;
                return crow::response(200, "Server gestartet.");
            } else {
                return crow::response(400, "Server läuft bereits.");
            }
        });

        // WebSocket-Route für eingehende Datenserver
        CROW_WEBSOCKET_ROUTE(app, "/in")
            .onopen([this](crow::websocket::connection& conn) {
                auto uuid = generate_uuid();
                std::lock_guard<std::mutex> lock(mutex);
                ring_buffers[uuid] = std::make_shared<RingBuffer>();
                connections[&conn] = uuid;
                std::cout << "Datenserver verbunden. UUID: " << uuid << std::endl;
            })
            .onmessage([this](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
                std::lock_guard<std::mutex> lock(mutex);
                auto it = connections.find(&conn);
                if (it != connections.end()) {
                    auto& uuid = it->second;

                    // Aktuellen Zeitstempel in Nanosekunden holen
                    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
                    auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

                    // Eingehende Nachricht parsen
                    json parsed_message = json::parse(message);

                    // JSON-Objekt erstellen
                    json full_message = {
                        {"timestamp", timestamp},
                        {"value", parsed_message["value"]},
                        {"uuid", uuid}
                    };

                    // Nachricht speichern
                    ring_buffers[uuid]->push(full_message);

                    // Ausgabe in der Konsole
                    std::cout << "Empfangene Nachricht: " << full_message.dump() << std::endl;
                }
            })
            .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
                std::lock_guard<std::mutex> lock(mutex);
                auto it = connections.find(&conn);
                if (it != connections.end()) {
                    std::cout << "Datenserver getrennt. UUID: " << it->second << std::endl;
                    ring_buffers.erase(it->second);
                    connections.erase(it);
                }
            });

        // WebSocket-Route für ausgehende Clients
        CROW_WEBSOCKET_ROUTE(app, "/out")
            .onopen([this](crow::websocket::connection& conn) {
                std::lock_guard<std::mutex> lock(mutex);
                client_connections.insert(&conn);
                std::cout << "Client verbunden." << std::endl;
            })
            .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
                std::lock_guard<std::mutex> lock(mutex);
                client_connections.erase(&conn);
                std::cout << "Client getrennt." << std::endl;
            });

        // Thread für die Verteilung von Daten an Clients
        std::thread([this]() {
            while (true) {
                distribute_data();
                std::this_thread::sleep_for(10ms); // 10ms Verzögerung
            }
        }).detach();

        // Starte die Crow-App
        app.port(8080).run();
    }

private:
    std::mutex mutex;
    std::atomic<bool> is_running = false;

    // UUID-Generator
    static std::string generate_uuid() {
        uuid_t uuid;
        char uuid_str[37];
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);
        return std::string(uuid_str);
    }

    // Verteilungslogik
    void distribute_data() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [uuid, buffer] : ring_buffers) {
            auto data = buffer->get_all();
            for (const auto& message : data) {
                for (auto& client : client_connections) {
                    try {
                        client->send_text(message.dump());
                    } catch (const std::exception& e) {
                        std::cerr << "Fehler beim Senden an Client: " << e.what() << std::endl;
                    }
                }
            }
        }
    }

    // Verbindungen und Ringbuffer
    std::unordered_map<std::string, std::shared_ptr<RingBuffer>> ring_buffers;
    std::unordered_map<crow::websocket::connection*, std::string> connections;
    std::unordered_set<crow::websocket::connection*> client_connections; // Korrekte Deklaration
};

int main() {
    WebSocketServer server;
    return 0;
}

