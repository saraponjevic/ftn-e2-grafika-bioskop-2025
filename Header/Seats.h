#pragma once
#include <vector>

enum class SeatStatus {
    Free,
    Reserved,
    Bought
};

struct Seat {
    float x, y;
    SeatStatus status;
};

extern std::vector<Seat> seats;

void initSeats();
void handleSeatClick(float ndcX, float ndcY);
void buySeats(int N);
float seatScaleForRow(int row);

