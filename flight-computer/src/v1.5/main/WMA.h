// DISCLIMER: this was ai generated

#ifndef WEIGHTED_MOVING_AVERAGE_H
#define WEIGHTED_MOVING_AVERAGE_H

// Adjust this if you need a different maximum possible window size.
#define WMA_MAX_SIZE 32

/**
 * @brief Weighted Moving Average that:
 *   - Uses a ring buffer of size 'windowSize' (<= WMA_MAX_SIZE).
 *   - The newest sample always has the highest weight.
 *   - Returns a valid average even if the window isn't fully filled yet.
 */
class WeightedMovingAverage
{
public:
    /**
     * @param windowSize How many samples to store (<= WMA_MAX_SIZE).
     *
     * If windowSize exceeds WMA_MAX_SIZE, we clamp it to WMA_MAX_SIZE.
     */
    WeightedMovingAverage(unsigned int windowSize)
        : m_windowSize(windowSize),
          m_head(0),
          m_count(0)
    {
        if (m_windowSize > WMA_MAX_SIZE)
        {
            m_windowSize = WMA_MAX_SIZE;
        }

        // Initialize the ring buffer to 0.0
        for (unsigned int i = 0; i < WMA_MAX_SIZE; ++i)
        {
            m_buffer[i] = 0.0;
        }
    }

    /**
     * @brief Add a new sample to the ring buffer.
     *        Overwrites the oldest sample when the buffer is full.
     */
    void addSample(double value)
    {
        m_buffer[m_head] = value;
        m_head = (m_head + 1U) % m_windowSize; // move head forward in a circular manner

        if (m_count < m_windowSize)
        {
            m_count++;
        }
    }

    /**
     * @brief Get the current weighted average of all samples in the buffer so far.
     *
     * Weights:
     *   - If m_count = k, the newest sample is weight k, the next-newest is weight k-1, etc.
     *   - So if only 3 samples were added, the newest has weight=3, oldest has weight=1, etc.
     *
     * @return The weighted average (0.0 if no samples exist).
     */
    double getAverage() const
    {
        if (m_count == 0)
        {
            return 0.0; // No samples yet
        }

        double weightedSum = 0.0;
        double totalWeight = 0.0;

        // We have m_count valid samples in the buffer.
        // The newest sample is at (m_head - 1).
        // Assign it weight = m_count.
        // The next-newest gets weight = m_count - 1, etc.

        for (unsigned int i = 0; i < m_count; ++i)
        {
            // i=0 => newest sample
            // i=1 => second-newest, etc.
            unsigned int weight = (m_count - i);
            unsigned int sampleIndex = (m_head + m_windowSize - 1U - i) % m_windowSize;

            weightedSum += m_buffer[sampleIndex] * weight;
            totalWeight += weight;
        }

        return weightedSum / totalWeight;
    }

private:
    unsigned int m_windowSize;     ///< Number of samples to store (up to WMA_MAX_SIZE)
    unsigned int m_head;           ///< Ring buffer "write" index
    unsigned int m_count;          ///< How many samples have been stored so far (<= m_windowSize)
    double m_buffer[WMA_MAX_SIZE]; ///< Circular buffer for the samples
};

#endif // WEIGHTED_MOVING_AVERAGE_H
