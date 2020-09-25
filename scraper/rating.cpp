#include "rating.hpp"

#include <QtMath>

static float eloProb(float r1, float r2)
{
    return 1.0f / (1.0f + qPow(10, (r2 - r1) / 400));
}

void EloRating::adjust(float k, float result, const EloRating &o)
{
    const float pa = eloProb(m_rating, o.m_rating);
    m_rating += k * (result - pa);
}

void EloRating::adjust(float k, const EloRating &partner, float result, const EloRating &o1, const EloRating &o2)
{
    const float r1 = 0.5f * (m_rating + partner.m_rating);
    const float r2 = 0.5f * (o1.m_rating + o2.m_rating);
    const float pa = eloProb(r1, r2);
    m_rating += k * (result - pa);
}
