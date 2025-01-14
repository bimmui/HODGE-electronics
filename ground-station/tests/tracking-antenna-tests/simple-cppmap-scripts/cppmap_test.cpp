#include <iostream>
#include "../cppmap3d.hh"

int main()
{
    double target_lat2 = 0.740270493924, target_longg2 = -1.240228753387, target_alt2 = 100;
    double target_lat = 0.7402751382449, target_longg = -1.240227212261, target_alt = 100;
    double lat = 0.7402761749704, longg = -1.240227756804, alt = 90;
    double ref_lat = 0, ref_longg = 0, ref_alt = 0;

    double az, el, range;
    cppmap3d::geodetic2aer(lat, longg, alt, ref_lat, ref_longg, ref_alt, az, el, range);

    std::cout << "Azimuthal: " << az << std::endl;
    std::cout << "Elevation: " << el << std::endl;
    std::cout << "Range: " << range << std::endl;

    std::cout << "----------------------------------------------" << std::endl;

    cppmap3d::geodetic2aer(target_lat, target_longg, target_alt, lat, longg, alt, az, el, range);

    std::cout << "Neighbor #1: " << std::endl;
    std::cout << "Azimuthal: " << az << std::endl;
    std::cout << "Elevation: " << el << std::endl;
    std::cout << "Range: " << range << std::endl;

    std::cout << "----------------------------------------------" << std::endl;

    cppmap3d::geodetic2aer(target_lat2, target_longg2, target_alt2, lat, longg, alt, az, el, range);

    std::cout << "Neighbor #2: " << std::endl;
    std::cout << "Azimuthal: " << az << std::endl;
    std::cout << "Elevation: " << el << std::endl;
    std::cout << "Range: " << range << std::endl;
}