#pragma once

#include <iostream>
#include <array>
#include <algorithm>
#include <cmath>

namespace walk_detector
{

    struct raw_data
    {
        raw_data() = default;
        raw_data(long long t, int nx, int ny, float ax, float ay, float az) : 
            time(t), x(nx), y(ny), accx(ax), accy(ay), accz(az)
        {}
        long long time;
        int x, y;
        float accx, accy, accz;

        int squared_scale() const
        {
            if(sq_scale > 0) return sq_scale;
            sq_scale = x*x+y*y;
            return sq_scale;
        }

        float acc_shake_fator()
        {
            return std::abs(accx) + std::abs(accy) + std::abs(accz);
        }
        mutable int sq_scale = -1;

    };

    struct walk_speed_vector
    {
        float x, y;

        float squaredLenght()
        {
            return x*x+y*y;
        }

        float length()
        {
            return sqrt(squaredLenght());
        }

        void normalize()
        {
            auto l = length();
            if(l > 0)
            {
                x /= l;
                y /= l;
            }
        }

        walk_speed_vector operator*(float s)
        {
            auto newVect = *this;
            newVect.x *= s;
            newVect.y *= s;
            return newVect;
        }

        walk_speed_vector operator*=(float s)
        {
            *this = *this * s;
            return *this;
        }
    };

    template <size_t buffer_size>
        class analyser 
        {

            float lastSpeedUpdate;
            float currentSpeedUpdate;
            float speedSmoothingValue; 

            std::array<raw_data, buffer_size> buffer;

            std::tuple<int, int> get_min_max_buffer()
            {
                static int min, max;
                min = buffer[0].squared_scale();
                max = buffer[0].squared_scale();
                for(const auto& sample : buffer)
                {
                    const auto scale = sample.squared_scale();
                    if(scale < min) min = scale;
                    if(scale > max) max = scale;
                }
                return std::tie(min, max);
            }

            int get_buffer_amplitude()
            {
                auto result = get_min_max_buffer();
                return sqrt(std::get<1>(result)) - sqrt(std::get<0>(result));
            }

            int get_buffer_mean_distance() const
            {
                
                int n = 0;
                for(const auto& sample : buffer)
                {
                    n += sample.squared_scale();

                }

                n/=int(buffer.size());
                return n;
            }

            int compute_freq_estimate() const
            {
                int zero_cross = 0;
                const int mean = get_buffer_mean_distance();
                bool positive = buffer[0].squared_scale() > mean ? true : false;
                for(const auto& sample : buffer)
                {
                    if(positive && sample.squared_scale() < mean)
                    {
                        positive = false;
                        zero_cross++;
                    }
                    if(!positive && sample.squared_scale() > mean)
                    {
                        positive = true;
                        zero_cross++;
                    }
                }
                return zero_cross;
            }

            walk_speed_vector estimation;
            size_t initial;
            
            float detectionThreshold;
            float map(float x, float in_min, float in_max, float out_min, float out_max)
            {
                return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
            }

            void compute()
            {

                auto& latest = buffer[0];
                auto angle = atan2(latest.y, latest.x);

                walk_speed_vector vect;
                vect.x = latest.x;
                vect.y = latest.y;

                float lsquared = vect.squaredLenght();
                std::cout << lsquared << '\n';
                if(lsquared > detectionThreshold)
                {
                    //std::cerr << "PLATFORM TILTED!!\n";

                    const auto s = sin(angle);
                    const auto c = cos(angle);

                    static const float nx = 1, ny = 0;
                    estimation.x = nx * c - ny * s;
                    estimation.y = nx * s + ny * c;
                    
                    //TODO calculate some actual speed!

                    const auto accShake = latest.acc_shake_fator();
                    const auto buffAmplitude = get_buffer_amplitude();
                    const auto freqEstimate = compute_freq_estimate();
                    /*std::cout << "freq estimate = " << compute_freq_estimate() << '\n';
                    std::cout << "accShake = " << accShake  << '\n';
                    std::cout << "buffAmplitude = " << buffAmplitude  << '\n';
                    */
                    
                    if(accShake > 0.25f)
                        std::cout << "Accelerometer shake detected\n";
                    else std::cout << '\n';

                    if(buffAmplitude > 6)
                        std::cout << "Buffer amplitude high detected\n";
                    else std::cout << '\n';
                    
                    if(freqEstimate > 3)
                        std::cout << "High Freq Estimate detected\n";
                    else std::cout << '\n';

                    float some_speed = 1;
                    
                    const int max_th = 300;
                    if(lsquared < max_th)
                    {
                        some_speed = map(lsquared, 15, max_th, 1, 3.5f);
                    }
                    else
                    {
                        some_speed = 3.5f;
                    }
                    estimation *= some_speed;

                }
                else
                {
                    std::cout << "PLATFORM NOT TILTED\n";
                    estimation.x = 0.F;
                    estimation.y = 0.F;
                }

            }

            public:

            analyser()
            {
                initial = 0;
                detectionThreshold =  15;
            }

            walk_speed_vector get_estimated_walk()
            {
                if(initial < buffer.size())
                    return {0.f, 0.f};
                compute();
                return estimation;
            }
            void push_data(const raw_data& data_point)
            {
                initial++;
                std::rotate(buffer.rbegin(), buffer.rbegin()+1, buffer.rend());
                buffer[0] = data_point;
            }
        };

}
