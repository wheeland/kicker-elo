#pragma once

#include <QVector>

class EloRating
{
public:
    EloRating() : m_rating(1000.0f) {}

    float abs() const { return m_rating; }

    void adjust(float k, float result, const EloRating &o);
    void adjust(float k, const EloRating &partner, float result, const EloRating &o1, const EloRating &o2);

    bool operator<=(const EloRating &other) const { return m_rating < other.m_rating; }

private:
    float m_rating;
};
