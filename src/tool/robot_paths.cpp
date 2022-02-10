/* creates paths for robot that are navigatable
 * considers the robot size and avoids gaps adjacent to occlusions
 */

#include <cell_world/paths.h>
#include <cell_world/map.h>

#include <utility>
using namespace std;

//namespace cell_world {
//    Paths::Paths(const Graph &g) :
//            cells(g.cells),
//            moves(cells.size() * cells.size(), Coordinates{0, 0}) {
//    }
//
//    Paths::Paths(const Graph &g, Move_list m) :
//            cells(g.cells),
//            moves(std::move(m)){
//        _compute_steps();
//    }
//
//    Paths::Paths(const Graph &g, const Path_builder &pb):
//            cells(g.cells),
//            moves(pb.moves),
//            steps(pb.steps){
//
//    }
//
//    Move Paths::get_move(const Cell &s, const Cell &d) const {
//        int si = _index(s,d);
//        if (si == Not_found) return Move{0, 0};
//        return moves[si];
//    }
//
//    bool Paths::operator==(const Paths &p) const {
//        if (moves.size() != p.moves.size()) return false;
//        for (unsigned int i = 0; i < moves.size(); i++) {
//            if (moves[i] != p.moves[i]) return false;
//        }
//        return true;
//    }
//
//    int Paths::_index(const Cell &s, const Cell &d) const {
//        int s_ind = cells.find(s);
//        if (s_ind == Not_found) return Not_found;
//        int d_ind = cells.find(d);
//        if (d_ind == Not_found) return Not_found;
//        return s_ind * (int) cells.size() + d_ind;
//    }
//
//    bool Paths::set_move(const Cell &s, const Cell &d, const Move &move) {
//        int m_index = _index(s,d);
//        if (m_index==Not_found) return false;
//        moves [m_index] = move;
//        return true;
//    }
//
//    Paths Paths::get_euclidean(const Graph &g) {
//        Paths paths(g);
//        for (const Cell &s:g.cells){
//            Coordinates s_coordinates = s.coordinates;
//            for (const Cell &d:g.cells){
//                if (s==d) {
//                    paths.set_move(s,d,{0,0});
//                } else {
//                    Location d_location = d.location;
//                    Cell_group s_connections = g[s];
//                    if (s_connections.empty()) {
//                        paths.set_move(s, d, {0, 0});
//                    } else {
//                        double min_distance = s_connections[0].location.dist(d_location);
//                        int best_option = 0;
//                        for (unsigned int i = 1; i < s_connections.size(); i++) {
//                            double distance = s_connections[i].location.dist(d_location);
//                            if (min_distance > distance) {
//                                best_option = i;
//                                min_distance = distance;
//                            }
//                        }
//                        Move best_move = s_connections[best_option].coordinates - s_coordinates;
//                        paths.set_move(s, d, best_move);
//                    }
//                }
//            }
//        }
//        paths._compute_steps();
//        return paths;
//    }
//
//    Paths Paths::get_manhattan(const Graph &g) {
//        Paths paths(g);
//        for (const Cell &s:g.cells) {
//            Coordinates s_coordinates = s.coordinates;
//            for (const Cell &d:g.cells) {
//                if (s == d) {
//                    paths.set_move(s, d, {0, 0});
//                } else {
//                    auto sp = g.get_shortest_path(s,d);
//                    if (sp.empty()){
//                        paths.set_move(s, d, {0, 0});
//                    } else {
//                        Move move = sp[sp.size()-2].coordinates - s_coordinates;
//                        paths.set_move(s, d, move);
//                    }
//                }
//            }
//        }
//        paths._compute_steps();
//        return paths;
//    }
//
//    Paths Paths::get_astar(const Graph &g) {
//        Paths paths(g);
//        Cell_group free_cells = g.cells.free_cells();
//        for (const Cell &s:free_cells) {
//            Coordinates s_coordinates = s.coordinates;
//            for (const Cell &d:free_cells) {
//                if (s == d) {
//                    paths.set_move(s, d, {0, 0});
//                } else {
//                    auto sp = g.get_shortest_path(s,d);
//                    if (sp.empty()){
//                        paths.set_move(s, d, {0, 0});
//                    } else {
//                        unsigned int path_len = sp.size() - 1;
//                        double min_distance = -1;
//                        for (const Cell&a:g[s]){
//                            if (g.get_shortest_path(a,d).size()==path_len){
//                                double a_distance = a.location.dist(d.location);
//                                if (min_distance == -1 || a_distance < min_distance){
//                                    min_distance = a_distance;
//                                    Move move = a.coordinates - s_coordinates;
//                                    paths.set_move(s, d, move);
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }
//        paths._compute_steps();
//        return paths;
//    }
//
//
//    Cell_group cell_world::Paths::get_path(const Cell &source, const Cell &destination) {
//        Map map (cells);
//        Cell_group trajectory;
//        Cell current = source;
//        trajectory.add(source);
//        while (!trajectory.contains(destination)){
//            const Cell &next = map[current.coordinates + get_move(current,destination)];
//            if (trajectory.contains(next)) return Cell_group();
//            trajectory.add(next);
//            current = next;
//        }
//        return trajectory;
//    }
//
//    void Paths::_compute_steps() {
//        steps.clear();
//        for (auto &s: cells){
//            for (auto &d: cells){
//                int pl = get_path(s,d).size();
//                steps.push_back(pl-1);
//            }
//        }
//    }
//
//    int Paths::get_steps(const Cell &s, const Cell &d) const {
//        return steps[_index(s,d)];
//    }
//
//    Move_list Paths::get_moves(const Cell &source, const Cell &destination) {
//        Move_list moves;
//        Map map (cells);
//        Cell_group trajectory;
//        Cell current = source;
//        trajectory.add(source);
//        while (!trajectory.contains(destination)){
//            auto move = get_move(current,destination);
//            moves.push_back(move);
//            const Cell &next = map[current.coordinates + move];
//            if (trajectory.contains(next)) return Move_list();
//            trajectory.add(next);
//            current = next;
//        }
//        return moves;
//    }
//}
int main(){
    cout << "paths" << endl;
    return 0;
}