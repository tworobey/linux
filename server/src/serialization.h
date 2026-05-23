#pragma once

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "model.h"
#include "model_serialization.h"

namespace serialization {

namespace fs = std::filesystem;

inline void SaveGameState(const model::Game& game, const fs::path& path) {
    // Сохраняем во временный файл, потом атомарно переименовываем
    fs::path tmp_path = path;
    tmp_path += ".tmp";

    {
        std::ofstream ofs(tmp_path, std::ios::binary);
        if (!ofs) {
            throw std::runtime_error("Cannot open file for writing: " + tmp_path.string());
        }
        boost::archive::text_oarchive oa(ofs);
        GameStateRepr repr(game);
        oa << repr;
    }

    fs::rename(tmp_path, path);
}

inline void LoadGameState(model::Game& game, const fs::path& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Cannot open file for reading: " + path.string());
    }
    boost::archive::text_iarchive ia(ifs);
    GameStateRepr repr;
    ia >> repr;
    repr.Restore(game);
}

} // namespace serialization
