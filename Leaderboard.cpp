#include "Leaderboard.hpp"

/**
 * @brief Constructor for RankingResult with top players, cutoffs, and elapsed time.
 *
 * @param top Vector of top-ranked Player objects, in sorted order.
 * @param cutoffs Map of player count thresholds to minimum level cutoffs.
 *   NOTE: This is only ever non-empty for Online::rankIncoming().
 *         This parameter & the corresponding member should be empty
 *         for all Offline algorithms.
 * @param elapsed Time taken to calculate the ranking, in seconds.
 */
RankingResult::RankingResult(const std::vector<Player> &top, const std::unordered_map<size_t, size_t> &cutoffs, double elapsed)
    : top_{top}, cutoffs_{cutoffs}, elapsed_{elapsed} {
}

/**
 * @brief Uses an early-stopping version of heapsort to
 *        select and sort the top 10% of players in-place
 *        (excluding the returned RankingResult vector)
 *
 * @param players A reference to the vector of Player objects to be ranked
 * @return A Ranking Result object whose
 * - top_ vector -> Contains the top 10% of players from the input in sorted order (ascending)
 * - cutoffs_    -> Is empty
 * - elapsed_    -> Contains the duration (ms) of the selection/sorting operation
 *
 * @post The order of the parameter vector is modified.
 */
RankingResult Offline::heapRank(std::vector<Player> &players) {
    const auto t1 = std::chrono::high_resolution_clock::now();

    RankingResult result;
    int topTen = std::floor(0.1 * players.size());
    std::make_heap(players.begin(), players.end()); // max-heap
    for (int i = 0; i < topTen; ++i) {
        std::pop_heap(players.begin(), players.end() - i);
    }
    result.top_.insert(result.top_.begin(), players.end() - topTen, players.end());
    std::sort(result.top_.begin(), result.top_.end());

    const auto t2 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> time = t2 - t1;
    result.elapsed_ = time.count();
    return result;
}

/**
 * @brief Uses a mixture of quickselect/quicksort to
 *        select and sort the top 10% of players with O(log N) memory
 *        (excluding the returned RankingResult vector)
 *
 * @param players A reference to the vector of Player objects to be ranked
 * @return A Ranking Result object whose
 * - top_ vector -> Contains the top 10% of players from the input in sorted order (ascending)
 * - cutoffs_    -> Is empty
 * - elapsed_    -> Contains the duration (ms) of the selection/sorting operation
 *
 * @post The order of the parameter vector is modified.
 */
RankingResult Offline::quickSelectRank(std::vector<Player> &players) {
    const auto t1 = std::chrono::high_resolution_clock::now();

    RankingResult result;
    int topTen = players.size() - (std::floor(0.1 * players.size()));
    quickSelect(players, 0, players.size() - 1, topTen);
    quickSort(players, topTen, players.size() - 1);
    result.top_.insert(result.top_.end(), players.begin() + topTen, players.end());

    const auto t2 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> time = t2 - t1;
    result.elapsed_ = time.count();
    return result;
}

/**
 * @brief A helper method that replaces the minimum element
 * in a min-heap with a target value & preserves the heap
 * by percolating the new value down to its correct position.
 *
 * Performs in O(log N) time.
 *
 * @pre The range [first, last) is a min-heap.
 *
 * @param first An iterator to a vector of Player objects
 *      denoting the beginning of a min-heap
 *      NOTE: Unlike the textbook, this is *not* an empty slot
 *      used to store temporary values. It is the root of the heap.
 *
 * @param last An iterator to a vector of Player objects
 *      denoting one past the end of a min-heap
 *      (i.e. it is not considering a valid index of the heap)
 *
 * @param target A reference to a Player object to be inserted into the heap
 * @post
 * - The vector slice denoted from [first,last) is a min-heap
 *   into which `target` has been inserted.
 * - The contents of `target` is not guaranteed to match its original state
 *   (ie. you may move it).
 */
void Online::replaceMin(PlayerIt first, PlayerIt last, Player &target) {
    auto size = std::distance(first, last);

    *first = std::move(target);
    int index = 0;

    while (true) {
        int child = index * 2 + 1;
        int smallerChild = index;

        if (child < size && *std::next(first, child) < *std::next(first, smallerChild)) {
            smallerChild = child; // Left Child
        }

        if (child + 1 < size && *std::next(first, child + 1) < *std::next(first, smallerChild)) {
            smallerChild = child + 1; // Right Child
        }

        if (smallerChild != index) {
            std::swap(*std::next(first, index), *std::next(first, smallerChild));
            index = smallerChild;
        } else {
            break;
        }
    }
}

/**
 * @brief Exhausts a stream of Players (ie. until there are none left) such that we:
 * 1) Maintain a running collection of the <reporting_interval> highest leveled players
 * 2) Record the Player level after reading every <reporting_interval> players
 *    representing the minimum level required to be in the leaderboard at that point.
 *
 * @note You should use NOT use a priority-queue.
 *       Instead, use a vector, the STL heap operations, & `replaceMin()`
 *
 * @param stream A stream providing Player objects
 * @param reporting_interval The frequency at which to record cutoff levels
 * @return A RankingResult in which:
 * - top_       -> Contains the top <reporting_interval> Players read in the stream in
 *                 sorted (least to greatest) order
 * - cutoffs_   -> Maps player count milestones to minimum level required at that point
 *                 including the minimum level after ALL players have been read, regardless
 *                 of being a multiple of the reporting interval
 * - elapsed_   -> Contains the duration (ms) of the selection/sorting operation
 *                 excluding fetching the next player in the stream
 *
 * @post All elements of the stream are read until there are none remaining.
 *
 * @example Suppose we have:
 * 1) A stream with 132 players
 * 2) A reporting interval of 50
 *
 * Then our resulting RankingResult might contain something like:
 * top_ = { Player("RECLUSE", 994), Player("WYLDER", 1002), ..., Player("DUCHESS", 1399) }, with length 50
 * cutoffs_ = { 50: 239, 100: 992, 132: 994 } (see RankingResult explanation)
 * elapsed_ = 0.003 (Your runtime will vary based on hardware)
 */
RankingResult Online::rankIncoming(PlayerStream &stream, const size_t &reporting_interval) {
    const auto t1 = std::chrono::high_resolution_clock::now();
    RankingResult result;
    int count = 0;

    std::vector<Player> topReportPlayers;
    while (stream.remaining() > 0) {
        Player currPlayer = stream.nextPlayer();
        count++;
        if (topReportPlayers.size() < reporting_interval) {
            topReportPlayers.push_back(currPlayer);
            std::make_heap(topReportPlayers.begin(), topReportPlayers.end(), std::greater<>());

        } else if (currPlayer > topReportPlayers[0]) {
            replaceMin(topReportPlayers.begin(), topReportPlayers.end(), currPlayer);
        }

        if (count % reporting_interval == 0 && topReportPlayers.empty() == false) {
            result.cutoffs_[count] = topReportPlayers[0].level_;
        }
    }

    std::sort(topReportPlayers.begin(), topReportPlayers.end());
    result.top_ = topReportPlayers;

    const auto t2 = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> time = t2 - t1;
    result.elapsed_ = time.count();
    return result;
}

// Helper Functions For quickSelectRank
int Offline::partition(std::vector<Player> &players, int &low, int &high) {
    int lower = low;
    Player x = players[high];
    for (int i = lower; i < high; i++) {
        if (players[i].level_ <= x.level_) {
            std::swap(players[lower], players[i]);
            lower++;
        }
    }
    std::swap(players[lower], players[high]);
    return lower;
}

void Offline::quickSort(std::vector<Player> &players, int low, int high) {
    if (low < high) {
        int pivot = partition(players, low, high);
        quickSort(players, low, pivot - 1);
        quickSort(players, pivot + 1, high);
    }
}

void Offline::quickSelect(std::vector<Player> &players, int low, int high, int k) {
    if (low < high) {
        int pivot = partition(players, low, high);

        if (pivot == k) {
            return;
        } else if (pivot < k) {
            quickSelect(players, pivot + 1, high, k);
        } else {
            quickSelect(players, low, pivot - 1, k);
        }
    }
}
