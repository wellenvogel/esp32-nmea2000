// Arduino Moving Average Library
// https://github.com/JChristensen/movingAvg
// Copyright (C) 2018 by Jack Christensen and licensed under
// GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html

#include <movingAvg.h>

// initialize - allocate the interval array
void movingAvg::begin()
{
    m_readings = new int[m_interval];
}

// add a new reading and return the new moving average
int movingAvg::reading(int newReading)
{
    // add each new data point to the sum until the m_readings array is filled
    if (m_nbrReadings < m_interval) {
        ++m_nbrReadings;
        m_sum += newReading;
    }
    // once the array is filled, subtract the oldest data point and add the new one
    else {
        m_sum = m_sum - m_readings[m_next] + newReading;
    }

    m_readings[m_next] = newReading;
    if (++m_next >= m_interval) m_next = 0;
    return (m_sum + m_nbrReadings / 2) / m_nbrReadings;
}

// just return the current moving average
int movingAvg::getAvg()
{
    return (m_sum + m_nbrReadings / 2) / m_nbrReadings;
}

// return the average for a subset of the data, the most recent nPoints readings.
// for invalid values of nPoints, return zero.
int movingAvg::getAvg(int nPoints)
{
    if (nPoints < 1 || nPoints > m_interval || nPoints > m_nbrReadings) {
        return 0;
    }
    else {
        long sum{0};
        int i = m_next;
        for (int n=0; n<nPoints; ++n) {
            if (i == 0) {
                i = m_interval - 1;
            }
            else {
                --i;
            }
            sum += m_readings[i];
        }
        return (sum + nPoints / 2) / nPoints;
    }
}

// start the moving average over again
void movingAvg::reset()
{
    m_nbrReadings = 0;
    m_sum = 0;
    m_next = 0;
}
