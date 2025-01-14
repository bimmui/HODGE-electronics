#include <iostream>
#include "../cppmap3d.hh"

int main()
{
    double target_lat = ..., target_longg = ..., target_alt = 90;
    double lat = ..., longg = ..., alt = 90;
    double ref_lat = 0, ref_longg = 0, ref_alt = 0;

    double az, el, range;
    cppmap3d::geodetic2aer(lat, longg, alt, ref_lat, ref_longg, ref_alt, az, el, range);

    std::cout << "Azimuthal: " << az << std::endl;
    std::cout << "Elevation: " << el << std::endl;
    std::cout << "Range: " << range << std::endl;

    std::cout << "----------------------------------------------" << std::endl;

    cppmap3d::geodetic2aer(target_lat, target_longg, target_alt, lat, longg, alt, az, el, range);

    std::cout << "Azimuthal: " << az << std::endl;
    std::cout << "Elevation: " << el << std::endl;
    std::cout << "Range: " << range << std::endl;
}