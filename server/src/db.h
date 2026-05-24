#pragma once

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <memory>

namespace db {

struct RetiredPlayer {
    std::string name;
    int score = 0;
    double play_time = 0.0; // в секундах
};

class Database {
public:
    explicit Database(const std::string& db_url)
        : conn_(db_url) {
        Init();
    }

    void SaveRetiredPlayer(const RetiredPlayer& player) {
        pqxx::work txn(conn_);
        txn.exec_params(
            "INSERT INTO retired_players (id, name, score, play_time_ms) "
            "VALUES (gen_random_uuid(), $1, $2, $3)",
            player.name,
            player.score,
            static_cast<int64_t>(player.play_time * 1000)
        );
        txn.commit();
    }

    std::vector<RetiredPlayer> GetRecords(int start, int max_items) const {
        pqxx::read_transaction txn(conn_);
        auto result = txn.exec_params(
            "SELECT name, score, play_time_ms FROM retired_players "
            "ORDER BY score DESC, play_time_ms ASC, name ASC "
            "LIMIT $1 OFFSET $2",
            max_items, start
        );

        std::vector<RetiredPlayer> records;
        for (const auto& row : result) {
            RetiredPlayer p;
            p.name = row[0].as<std::string>();
            p.score = row[1].as<int>();
            p.play_time = row[2].as<int64_t>() / 1000.0;
            records.push_back(p);
        }
        return records;
    }

private:
    void Init() {
        pqxx::work txn(conn_);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
                name VARCHAR(100) NOT NULL,
                score INTEGER NOT NULL,
                play_time_ms BIGINT NOT NULL
            )
        )");
        txn.exec(R"(
            CREATE INDEX IF NOT EXISTS retired_players_score_time_name
            ON retired_players (score DESC, play_time_ms ASC, name ASC)
        )");
        txn.commit();
    }

    mutable pqxx::connection conn_;
};

} // namespace db
