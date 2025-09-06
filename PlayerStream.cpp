#include "PlayerStream.hpp"

VectorPlayerStream::VectorPlayerStream(const std::vector<Player> &players) {
    players_ = players;
    currIndex = 0;
}

Player VectorPlayerStream::nextPlayer() {
    if (currIndex >= players_.size()) {
        throw std::runtime_error("Out of Bounds.");
    }
    
    return players_[currIndex++];
}

size_t VectorPlayerStream::remaining() const {
    return players_.size() - currIndex;
}