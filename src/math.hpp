#include <algorithm>
#include <stdexcept>
#include <vector>

class P2Quantile {
   public:
    explicit P2Quantile(double prob) : p(prob) {
        if (p < 0.0 || p > 1.0) {
            throw std::invalid_argument("p must be in (0,1)");
        }
    }

    void add(double x) {
        if (count < 5) {
            // Fill initial buffer
            buffer.push_back(x);
            ++count;
            if (count == 5) {
                std::sort(buffer.begin(), buffer.end());
                for (int i = 0; i < 5; ++i)
                    q[i] = buffer[i];
                n[0] = 0;
                n[1] = 1;
                n[2] = 2;
                n[3] = 3;
                n[4] = 4;
                np[0] = 0;
                np[1] = 2 * p;
                np[2] = 4 * p;
                np[3] = 2 + 2 * p;
                np[4] = 4;
            }
            return;
        }

        // Step 1: Locate k such that q_k <= x < q_{k+1}
        int k;
        if (x < q[0]) {
            q[0] = x;
            k = 0;
        } else if (x >= q[4]) {
            q[4] = x;
            k = 3;
        } else {
            for (k = 0; k < 4 && x >= q[k + 1]; ++k)
                ;
        }

        // Step 2: Increment positions of markers above k
        for (int i = k + 1; i < 5; ++i)
            n[i]++;

        // Step 3: Desired positions
        np[1] += p / 2.0;
        np[2] += p;
        np[3] += (1 + p) / 2.0;
        np[4] += 1;

        // Step 4: Adjust heights of markers 2-4
        for (int i = 1; i < 4; ++i) {
            double d = np[i] - n[i];
            if ((d >= 1 && n[i + 1] - n[i] > 1) || (d <= -1 && n[i - 1] - n[i] < -1)) {
                int sign = (d >= 0) ? 1 : -1;
                double qn = parabolic(i, sign);
                if (q[i - 1] < qn && qn < q[i + 1]) {
                    q[i] = qn;
                } else {
                    q[i] = linear(i, sign);
                }
                n[i] += sign;
            }
        }
    }

    double get() const {
        if (count < 5) {
            if (count == 0)
                throw std::runtime_error("No data");
            auto tmp = buffer;
            std::sort(tmp.begin(), tmp.end());
            size_t idx = static_cast<size_t>(p * (tmp.size() - 1));
            return tmp[idx];
        }
        return q[2];
    }

   private:
    double p;
    size_t count = 0;
    std::vector<double> buffer;
    double q[5] = {};
    double n[5] = {};
    double np[5] = {};

    double parabolic(int i, int d) const {
        return q[i] + d / (n[i + 1] - n[i - 1]) *
                          ((n[i] - n[i - 1] + d) * (q[i + 1] - q[i]) / (n[i + 1] - n[i]) +
                           (n[i + 1] - n[i] - d) * (q[i] - q[i - 1]) / (n[i] - n[i - 1]));
    }

    double linear(int i, int d) const { return q[i] + d * (q[i + d] - q[i]) / (n[i + d] - n[i]); }
};
