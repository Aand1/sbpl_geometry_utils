#include <cmath>
#include <iostream>
#include <sbpl_geometry_utils/interpolation.h>
#include <sbpl_geometry_utils/utils.h>

namespace sbpl
{
    namespace interp
    {

namespace
{

// @brief Normalize all angles in \angles into the range [min_limits, max_limits)
// @return true if min_limits and max_limits are valid (have the same size as angles and
//         (max_limits[i] > min_limits[i]) and \angles fall within those limits; false otherwise
bool NormalizeAnglesIntoRange(std::vector<double>& angles,
                              const std::vector<double>& min_limits,
                              const std::vector<double>& max_limits)
{
    unsigned dim = angles.size();
    if (min_limits.size() != dim || max_limits.size() != dim) {
        return false;
    }

    for (unsigned i = 0; i < dim; i++) {
        if (min_limits[i] > max_limits[i]) {
            return false;
        }
    }

    for (unsigned i = 0; i < dim; i++) {
        double min_angle_norm = utils::NormalizeAngle(min_limits[i], 0.0, 2.0 * M_PI);
        angles[i] = utils::NormalizeAngle(angles[i], min_limits[i], min_angle_norm);
        if (angles[i] < min_limits[i] || angles[i] > max_limits[i]) {
            return false;
        }
    }

    return true;
}

} // end empty namespace

bool InterpolatePath(const std::vector<double>& start, const std::vector<double>& end,
                     const std::vector<double>& min_limits, const std::vector<double>& max_limits,
                     const std::vector<double>& inc,
                     std::vector<std::vector<double> >& path)
{
    std::vector<bool> continuous_joints(start.size(), false);
    const double eps = 1e-6;
    return InterpolatePath(start, end, min_limits, max_limits, inc, continuous_joints, eps, path);
}

bool InterpolatePath(const std::vector<double>& start, const std::vector<double>& end,
                     const std::vector<double>& min_limits, const std::vector<double>& max_limits,
                     const std::vector<double>& inc, double eps,
                     std::vector<std::vector<double> >& path)
{
    std::vector<bool> continuous_joints(start.size(), false);
    return InterpolatePath(start, end, min_limits, max_limits, inc, continuous_joints, eps, path);
}

bool InterpolatePath(const std::vector<double>& start, const std::vector<double>& end,
                     const std::vector<double>& min_limits, const std::vector<double>& max_limits,
                     const std::vector<double>& inc, const std::vector<bool>& continuous_joints,
                     std::vector<std::vector<double> >& path)
{
    const double eps = 1e-6;
    return InterpolatePath(start, end, min_limits, max_limits, inc, continuous_joints, eps, path);
}

bool InterpolatePath(const std::vector<double>& start, const std::vector<double>& end,
                     const std::vector<double>& min_limits, const std::vector<double>& max_limits,
                     const std::vector<double>& inc, const std::vector<bool>& continuous_joints,
                     double eps, std::vector<std::vector<double> >& path)
{
    path.clear();

    // make copies so i can normalize things
    std::vector<double> start_norm = start;
    std::vector<double> end_norm = end;

    // check that all inputs have the same size
    unsigned dim = start_norm.size();
    if (dim != end_norm.size() || dim != inc.size() || dim != min_limits.size() || dim != max_limits.size() ||
        dim != continuous_joints.size())
    {
        return false;
    }

    // check that min_limits and max_limits make sense, and normalize angles
    if (!NormalizeAnglesIntoRange(start_norm, min_limits, max_limits)) {
        return false;
    }
    if (!NormalizeAnglesIntoRange(end_norm, min_limits, max_limits)) {
        return false;
    }

    // determine the directions in which we should interpolate the angles
    std::vector<int> travel_dirs(dim, 0);
    for (unsigned i = 0; i < dim; i++) {
        double angle_diff = utils::ShortestAngleDiff(end_norm[i], start_norm[i]);
        if (continuous_joints[i]) {
            travel_dirs[i] = utils::Sign(angle_diff);
        }
        else {
            if (start_norm[i] + angle_diff > max_limits[i] || start_norm[i] + angle_diff < min_limits[i]) {
                travel_dirs[i] = -utils::Sign(angle_diff);
            }
            else {
                travel_dirs[i] = utils::Sign(angle_diff);
            }
        }
    }

    int max_iterations = 0;
    for (int i = 0; i < dim; i++) {
        double angle_diff = utils::ShortestAngleDiff(end_norm[i], start_norm[i]);

        double angle_dist;
        if (start_norm[i] + angle_diff > max_limits[i] || start_norm[i] + angle_diff < min_limits[i]) {
            angle_dist = 2 * M_PI - fabs(angle_diff);
        }
        else {
            angle_dist = fabs(angle_diff);
        }

        int max_iters_for_angle = 0;
        if (fabs(end_norm[i] - start_norm[i]) < eps) {
            max_iters_for_angle = 1;
        }
        else if (fabs(end_norm[i] - start_norm[i]) <= inc[i]) {
            max_iters_for_angle = 1;
        }
        else {
            max_iters_for_angle = (int)ceil(angle_dist / inc[i]);
        }

        max_iterations = std::max(max_iterations, max_iters_for_angle);
    }

    // interpolate between angles
    std::vector<double> curr_cfg = start_norm;
    path.push_back(curr_cfg);
    for (int c = 0; c < max_iterations; c++) {
        // inch all the joint angles towards the end
        for (unsigned i = 0; i < dim; i++) {
            double anglediff = utils::ShortestAngleDiff(curr_cfg[i], end_norm[i]);

            if (fabs(anglediff) < inc[i]) {
                // add the last little bit to reach the end for this joint angle
                curr_cfg[i] += anglediff;
            }
            else {
                // add increment in the direction of the shortest legal angle
                curr_cfg[i] += travel_dirs[i] * inc[i];
            }

            // wrap into the [min_limits[i], max_limits[i]] range to keep continuous joints in desired range
            if (curr_cfg[i] > max_limits[i]) {
                curr_cfg[i] -= 2 * M_PI;
            }
            if (curr_cfg[i] < min_limits[i]) {
                curr_cfg[i] += 2 * M_PI;
            }
        }

        // add this waypoint to the path
        path.push_back(curr_cfg);
    }

    return true;
}

    } // end namespace interp
} // end namespace sbpl