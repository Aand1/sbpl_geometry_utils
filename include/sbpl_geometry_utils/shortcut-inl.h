#ifndef sbpl_shortcutting_inl_h
#define sbpl_shortcutting_inl_h

#include <sbpl_geometry_utils/shortcut.h>

namespace sbpl
{

namespace shortcut
{

template <
    typename PathContainer,
    typename CostsContainer,
    typename PathGeneratorsContainer,
    typename ShortcutPathContainer,
    typename CostCompare>
bool ShortcutPath(
    const PathContainer& orig_path,
    const CostsContainer& orig_path_costs,
    const PathGeneratorsContainer& path_generators,
    ShortcutPathContainer& shortcut_path,
    unsigned window,
    unsigned granularity,
    const CostCompare& leq)
{
    typedef typename PathContainer::value_type PointType;
    typedef typename CostsContainer::value_type CostType;
    typedef typename PathGeneratorsContainer::value_type PathGeneratorType;

    // assert one cost per point transition
    if (orig_path.size() != orig_path_costs.size() + 1) {
        return false;
    }

    // nothing to do with a trivial path
    if (orig_path.size() < 2) {
        return true;
    }

    /// gather the accumulated original costs per point
    std::vector<CostType> accum_point_costs(orig_path.size());
    accum_point_costs[0] = 0.0;
    for (int i = 1; i < (int)accum_point_costs.size(); ++i) {
        accum_point_costs[i] = accum_point_costs[i - 1] + orig_path_costs[i - 1];
    }

    std::vector<PointType> result_traj;

    typename PathContainer::const_iterator curr_start = orig_path.begin();
    typename PathContainer::const_iterator last_end = orig_path.begin();
    typename PathContainer::const_iterator curr_end = orig_path.begin(); ++curr_end;

    // maintain indices into the original path
    unsigned start_index = 0;
    unsigned end_index = 1;

    bool cost_improved;
    typename PathGeneratorType::PathContainer best_path;
    typename PathGeneratorType::Cost best_cost;

    cost_improved = false;
    best_path.push_back(*curr_start);
    best_path.push_back(*curr_end);
    best_cost = accum_point_costs[end_index] - accum_point_costs[start_index];

    while (curr_end != orig_path.end()) {
        cost_improved = false;
        CostType orig_cost = best_cost;

        // look for a better path using the trajectories generated by the path generators
        for (auto it = path_generators.begin(); it != path_generators.end(); ++it) {
            // find the path between these two points with the lowest cost
            typename PathGeneratorType::PathContainer path;
            typename PathGeneratorType::Cost cost;
            if (it->generate_path(*curr_start, *curr_end, path, cost) &&
                leq(cost, best_cost + accum_point_costs[end_index] - accum_point_costs[end_index - 1]))
            {
                cost_improved = true;
                best_cost = cost;
                best_path.swap(path);
            }
        }

        if (cost_improved) {
            // advance curr_end to attempt further cost improvement
            typename PathContainer::difference_type dist_to_end = std::distance(curr_end, orig_path.end());

            if (dist_to_end == 1) {
                curr_end = orig_path.end();
                last_end = --orig_path.end();
                end_index = orig_path.size();
            }
            else if (dist_to_end < granularity) {
                std::advance(curr_end, dist_to_end - 1);
                std::advance(last_end, dist_to_end - 1);
                end_index += dist_to_end - 1;
            }
            else {
                std::advance(curr_end, granularity);
                std::advance(last_end, granularity);
                end_index += granularity;
            }
        }
        else {
            if (!result_traj.empty()) {
                result_traj.pop_back();
            }
            result_traj.insert(result_traj.end(), best_path.begin(), best_path.end()); // insert path from start to end
            curr_start = last_end;
            start_index = end_index - 1;

            // reinitialize best_path
            cost_improved = false;
            best_path.clear();
            best_path.push_back(*curr_start);
            best_path.push_back(*curr_end);
            best_cost = accum_point_costs[end_index] - accum_point_costs[start_index];
        }
    }

    if (!result_traj.empty()) {
        result_traj.pop_back();
    }
    result_traj.insert(result_traj.end(), best_path.begin(), best_path.end());

    shortcut_path.assign(result_traj.begin(), result_traj.end());
    return true;
}

} // namespace shortcut

} // namespace sbpl

#endif

